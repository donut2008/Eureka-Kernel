t tagged_addr *cpu_pa;
	struct tagged_addr *gpu_pa;
	u64 page_off, page_count;
	u64 i;
	u64 offset;

	kbase_os_mem_map_lock(kctx);
	kbase_gpu_vm_lock(kctx);

	/* find the region where the virtual address is contained */
	reg = kbase_region_tracker_find_region_enclosing_address(kctx,
			sset->mem_handle.basep.handle);
	if (kbase_is_region_invalid_or_free(reg)) {
		dev_warn(kctx->kbdev->dev, "Can't find a valid region at VA 0x%016llX",
				sset->mem_handle.basep.handle);
		err = -EINVAL;
		goto out_unlock;
	}

	/*
	 * Handle imported memory before checking for KBASE_REG_CPU_CACHED. The
	 * CPU mapping cacheability is defined by the owner of the imported
	 * memory, and not by kbase, therefore we must assume that any imported
	 * memory may be cached.
	 */
	if (kbase_mem_is_imported(reg->gpu_alloc->type)) {
		err = kbase_mem_do_sync_imported(kctx, reg, sync_fn);
		goto out_unlock;
	}

	if (!(reg->flags & KBASE_REG_CPU_CACHED))
		goto out_unlock;

	start = (uintptr_t)sset->user_addr;
	size = (size_t)sset->size;

	map = kbasep_find_enclosing_cpu_mapping(kctx, start, size, &offset);
	if (!map) {
		dev_warn(kctx->kbdev->dev, "Can't find CPU mapping 0x%016lX for VA 0x%016llX",
				start, sset->mem_handle.basep.handle);
		err = -EINVAL;
		goto out_unlock;
	}

	page_off = offset >> PAGE_SHIFT;
	offset &= ~PAGE_MASK;
	page_count = (size + offset + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
	cpu_pa = kbase_get_cpu_phy_pages(reg);
	gpu_pa = kbase_get_gpu_phy_pages(reg);

	if (page_off > reg->nr_pages ||
			page_off + page_count > reg->nr_pages) {
		/* Sync overflows the region */
		err = -EINVAL;
		goto out_unlock;
	}

	/* Sync first page */
	if (as_phys_addr_t(cpu_pa[page_off])) {
		size_t sz = MIN(((size_t) PAGE_SIZE - offset), size);

		kbase_sync_single(kctx, cpu_pa[page_off], gpu_pa[page_off],
				offset, sz, sync_fn);
	}

	/* Sync middle pages (if any) */
	for (i = 1; page_count > 2 && i < page_count - 1; i++) {
		/* we grow upwards, so bail on first non-present page */
		if (!as_phys_addr_t(cpu_pa[page_off + i]))
			break;

		kbase_sync_single(kctx, cpu_pa[page_off + i],
				gpu_pa[page_off + i], 0, PAGE_SIZE, sync_fn);
	}

	/* Sync last page (if any) */
	if (page_count > 1 &&
	    as_phys_addr_t(cpu_pa[page_off + page_count - 1])) {
		size_t sz = ((start + size - 1) & ~PAGE_MASK) + 1;

		kbase_sync_single(kctx, cpu_pa[page_off + page_count - 1],
				gpu_pa[page_off + page_count - 1], 0, sz,
				sync_fn);
	}

out_unlock:
	kbase_gpu_vm_unlock(kctx);
	kbase_os_mem_map_unlock(kctx);
	return err;
}

int kbase_sync_now(struct kbase_context *kctx, struct basep_syncset *sset)
{
	int err = -EINVAL;

	KBASE_DEBUG_ASSERT(kctx != NULL);
	KBASE_DEBUG_ASSERT(sset != NULL);

	if (sset->mem_handle.basep.handle & ~PAGE_MASK) {
		dev_warn(kctx->kbdev->dev,
				"mem_handle: passed parameter is invalid");
		return -EINVAL;
	}

	switch (sset->type) {
	case BASE_SYNCSET_OP_MSYNC:
		err = kbase_do_syncset(kctx, sset, KBASE_SYNC_TO_DEVICE);
		break;

	case BASE_SYNCSET_OP_CSYNC:
		err = kbase_do_syncset(kctx, sset, KBASE_SYNC_TO_CPU);
		break;

	default:
		dev_warn(kctx->kbdev->dev, "Unknown msync op %d\n", sset->type);
		break;
	}

	return err;
}

KBASE_EXPORT_TEST_API(kbase_sync_now);

/* vm lock must be held */
int kbase_mem_free_region(struct kbase_context *kctx, struct kbase_va_region *reg)
{
	int err;

	KBASE_DEBUG_ASSERT(NULL != kctx);
	KBASE_DEBUG_ASSERT(NULL != reg);
	dev_dbg(kctx->kbdev->dev, "%s %p in kctx %p\n",
		__func__, (void *)reg, (void *)kctx);
	lockdep_assert_held(&kctx->reg_lock);

	if (reg->flags & KBASE_REG_NO_USER_FREE) {
		dev_warn(kctx->kbdev->dev, "Attempt to free GPU memory whose freeing by user space is forbidden!\n");
		return -EINVAL;
	}

	/*
	 * Unlink the physical allocation before unmaking it evictable so
	 * that the allocation isn't grown back to its last backed size
	 * as we're going to unmap it anyway.
	 */
	reg->cpu_alloc->reg = NULL;
	if (reg->cpu_alloc != reg->gpu_alloc)
		reg->gpu_alloc->reg = NULL;

	/*
	 * If a region has been made evictable then we must unmake it
	 * before trying to free it.
	 * If the memory hasn't been reclaimed it will be unmapped and freed
	 * below, if it has been reclaimed then the operations below are no-ops.
	 */
	if (reg->flags & KBASE_REG_DONT_NEED) {
		KBASE_DEBUG_ASSERT(reg->cpu_alloc->type ==
				   KBASE_MEM_TYPE_NATIVE);
		kbase_mem_evictable_unmake(reg->gpu_alloc);
	}

	err = kbase_gpu_munmap(kctx, reg);
	if (err) {
		dev_warn(kctx->kbdev->dev, "Could not unmap from the GPU...\n");
		goto out;
	}

	/* This will also free the physical pages */
	kbase_free_alloced_region(reg);

 out:
	return err;
}

KBASE_EXPORT_TEST_API(kbase_mem_free_region);

/**
 * @brief Free the region from the GPU and unregister it.
 *
 * This function implements the free operation on a memory segment.
 * It will loudly fail if called with outstanding mappings.
 */
int kbase_mem_free(struct kbase_context *kctx, u64 gpu_addr)
{
	int err = 0;
	struct kbase_va_region *reg;

	KBASE_DEBUG_ASSERT(kctx != NULL);
	dev_dbg(kctx->kbdev->dev, "%s 0x%llx in kctx %p\n",
		__func__, gpu_addr, (void *)kctx);

	if ((gpu_addr & ~PAGE_MASK) && (gpu_addr >= PAGE_SIZE)) {
		dev_warn(kctx->kbdev->dev, "kbase_mem_free: gpu_addr parameter is invalid");
		return -EINVAL;
	}

	if (0 == gpu_addr) {
		dev_warn(kctx->kbdev->dev, "gpu_addr 0 is reserved for the ringbuffer and it's an error to try to free it using kbase_mem_free\n");
		return -EINVAL;
	}
	kbase_gpu_vm_lock(kctx);

	if (gpu_addr >= BASE_MEM_COOKIE_BASE &&
	    gpu_addr < BASE_MEM_FIRST_FREE_ADDRESS) {
		int cookie = PFN_DOWN(gpu_addr - BASE_MEM_COOKIE_BASE);

		reg = kctx->pending_regions[cookie];
		if (!reg) {
			err = -EINVAL;
			goto out_unlock;
		}

		/* ask to unlink the cookie as we'll free it */

		kctx->pending_regions[cookie] = NULL;
		bitmap_set(kctx->cookies, cookie, 1);

		kbase_free_alloced_region(reg);
	} else {
		/* A real GPU va */
		/* Validate the region */
		reg = kbase_region_tracker_find_region_base_address(kctx, gpu_addr);
		if (kbase_is_region_invalid_or_free(reg)) {
			dev_warn(kctx->kbdev->dev, "kbase_mem_free called with nonexistent gpu_addr 0x%llX",
					gpu_addr);
			err = -EINVAL;
			goto out_unlock;
		}

		if ((reg->flags & KBASE_REG_ZONE_MASK) == KBASE_REG_ZONE_SAME_VA) {
			/* SAME_VA must be freed through munmap */
			dev_warn(kctx->kbdev->dev, "%s called on SAME_VA memory 0x%llX", __func__,
					gpu_addr);
			err = -EINVAL;
			goto out_unlock;
		}
		err = kbase_mem_free_region(kctx, reg);
	}

 out_unlock:
	kbase_gpu_vm_unlock(kctx);
	return err;
}

KBASE_EXPORT_TEST_API(kbase_mem_free);

int kbase_update_region_flags(struct kbase_context *kctx,
		struct kbase_va_region *reg, unsigned long flags)
{
	KBASE_DEBUG_ASSERT(NULL != reg);
	KBASE_DEBUG_ASSERT((flags & ~((1ul << BASE_MEM_FLAGS_NR_BITS) - 1)) == 0);

	reg->flags |= kbase_cache_enabled(flags, reg->nr_pages);
	/* all memory is now growable */
	reg->flags |= KBASE_REG_GROWABLE;

	if (flags & BASE_MEM_GROW_ON_GPF)
		reg->flags |= KBASE_REG_PF_GROW;

	if (flags & BASE_MEM_PROT_CPU_WR)
		reg->flags |= KBASE_REG_CPU_WR;

	if (flags & BASE_MEM_PROT_CPU_RD)
		reg->flags |= KBASE_REG_CPU_RD;

	if (flags & BASE_MEM_PROT_GPU_WR)
		reg->flags |= KBASE_REG_GPU_WR;

	if (flags & BASE_MEM_PROT_GPU_RD)
		reg->flags |= KBASE_REG_GPU_RD;

	if (0 == (flags & BASE_MEM_PROT_GPU_EX))
		reg->flags |= KBASE_REG_GPU_NX;

	if (!kbase_device_is_cpu_coherent(kctx->kbdev)) {
		if (flags & BASE_MEM_COHERENT_SYSTEM_REQUIRED &&
				!(flags & BASE_MEM_UNCACHED_GPU))
			return -EINVAL;
	} else if (flags & (BASE_MEM_COHERENT_SYSTEM |
			BASE_MEM_COHERENT_SYSTEM_REQUIRED)) {
		reg->flags |= KBASE_REG_SHARE_BOTH;
	}

	if (!(reg->flags & KBASE_REG_SHARE_BOTH) &&
			flags & BASE_MEM_COHERENT_LOCAL) {
		reg->flags |= KBASE_REG_SHARE_IN;
	}

	if (flags & BASE_MEM_TILER_ALIGN_TOP)
		reg->flags |= KBASE_REG_TILER_ALIGN_TOP;


	/* Set up default MEMATTR usage */
	if (!(reg->flags & KBASE_REG_GPU_CACHED)) {
		if (kctx->kbdev->mmu_mode->flags &
				KBASE_MMU_MODE_HAS_NON_CACHEABLE) {
			/* Override shareability, and MEMATTR for uncached */
			reg->flags &= ~(KBASE_REG_SHARE_IN | KBASE_REG_SHARE_BOTH);
			reg->flags |= KBASE_REG_MEMATTR_INDEX(AS_MEMATTR_INDEX_NON_CACHEABLE);
		} else {
			dev_warn(kctx->kbdev->dev,
				"Can't allocate GPU uncached memory due to MMU in Legacy Mode\n");
			return -EINVAL;
		}
	} else if (kctx->kbdev->system_coherency == COHERENCY_ACE &&
		(reg->flags & KBASE_REG_SHARE_BOTH)) {
		reg->flags |=
			KBASE_REG_MEMATTR_INDEX(AS_MEMATTR_INDEX_DEFAULT_ACE);
	} else {
		reg->flags |=
			KBASE_REG_MEMATTR_INDEX(AS_MEMATTR_INDEX_DEFAULT);
	}

	if (flags & BASEP_MEM_PERMANENT_KERNEL_MAPPING)
		reg->flags |= KBASE_REG_PERMANENT_KERNEL_MAPPING;

	if (flags & BASEP_MEM_NO_USER_FREE)
		reg->flags |= KBASE_REG_NO_USER_FREE;

	if (flags & BASE_MEM_GPU_VA_SAME_4GB_PAGE)
		reg->flags |= KBASE_REG_GPU_VA_SAME_4GB_PAGE;

	return 0;
}

int kbase_alloc_phy_pages_helper(struct kbase_mem_phy_alloc *alloc,
		size_t nr_pages_requested)
{
	int new_page_count __maybe_unused;
	size_t nr_left = nr_pages_requested;
	int res;
	struct kbase_context *kctx;
	struct kbase_device *kbdev;
	struct tagged_addr *tp;

	if (WARN_ON(alloc->type != KBASE_MEM_TYPE_NATIVE) ||
	    WARN_ON(alloc->imported.native.kctx == NULL) ||
	    WARN_ON(alloc->group_id >= MEMORY_GROUP_MANAGER_NR_GROUPS)) {
		return -EINVAL;
	}

	if (alloc->reg) {
		if (nr_pages_requested > alloc->reg->nr_pages - alloc->nents)
			goto invalid_request;
	}

	kctx = alloc->imported.native.kctx;
	kbdev = kctx->kbdev;

	if (nr_pages_requested == 0)
		goto done; /*nothing to do*/

	new_page_count = atomic_add_return(
		nr_pages_requested, &kctx->used_pages);
	atomic_add(nr_pages_requested,
		&kctx->kbdev->memdev.used_pages);

	/* Increase mm counters before we allocate pages so that this
	 * allocation is visible to the OOM killer */
	kbase_process_page_usage_inc(kctx, nr_pages_requested);

	tp = alloc->pages + alloc->nents;

#ifdef CONFIG_MALI_2MB_ALLOC
	/* Check if we have enough pages requested so we can allocate a large
	 * page (512 * 4KB = 2MB )
	 */
	if (nr_left >= (SZ_2M / SZ_4K)) {
		int nr_lp = nr_left / (SZ_2M / SZ_4K);

		res = kbase_mem_pool_alloc_pages(
			&kctx->mem_pools.large[alloc->group_id],
			 nr_lp * (SZ_2M / SZ_4K),
			 tp,
			 true);

		if (res > 0) {
			nr_left -= res;
			tp += res;
		}

		if (nr_left) {
			struct kbase_sub_alloc *sa, *temp_sa;

			spin_lock(&kctx->mem_partials_lock);

			list_for_each_entry_safe(sa, temp_sa,
						 &kctx->mem_partials, link) {
				int pidx = 0;

				while (nr_left) {
					pidx = find_next_zero_bit(sa->sub_pages,
								  SZ_2M / SZ_4K,
								  pidx);
					bitmap_set(sa->sub_pages, pidx, 1);
					*tp++ = as_tagged_tag(page_to_phys(sa->page +
									   pidx),
							      FROM_PARTIAL);
					nr_left--;

					if (bitmap_full(sa->sub_pages, SZ_2M / SZ_4K)) {
						/* unlink from partial list when full */
						list_del_init(&sa->link);
						break;
					}
				}
			}
			spin_unlock(&kctx->mem_partials_lock);
		}

		/* only if we actually have a chunk left <512. If more it indicates
		 * that we couldn't allocate a 2MB above, so no point to retry here.
		 */
		if (nr_left > 0 && nr_left < (SZ_2M / SZ_4K)) {
			/* create a new partial and suballocate the rest from it */
			struct page *np = NULL;

			do {
				int err;

				np = kbase_mem_pool_alloc(
					&kctx->mem_pools.large[
						alloc->group_id]);
				if (np)
					break;

				err = kbase_mem_pool_grow(
					&kctx->mem_pools.large[alloc->group_id],
					1);
				if (err)
					break;
			} while (1);

			if (np) {
				int i;
				struct kbase_sub_alloc *sa;
				struct page *p;

				sa = kmalloc(sizeof(*sa), GFP_KERNEL);
				if (!sa) {
					kbase_mem_pool_free(
						&kctx->mem_pools.large[
							alloc->group_id],
						np,
						false);
					goto no_new_partial;
				}

				/* store pointers back to the control struct */
				np->lru.next = (void *)sa;
				for (p = np; p < np + SZ_2M / SZ_4K; p++)
					p->lru.prev = (void *)np;
				INIT_LIST_HEAD(&sa->link);
				bitmap_zero(sa->sub_pages, SZ_2M / SZ_4K);
				sa->page = np;

				for (i = 0; i < nr_left; i++)
					*tp++ = as_tagged_tag(page_to_phys(np + i), FROM_PARTIAL);

				bitmap_set(sa->sub_pages, 0, nr_left);
				nr_left = 0;

				/* expose for later use */
				spin_lock(&kctx->mem_partials_lock);
				list_add(&sa->link, &kctx->mem_partials);
				spin_unlock(&kctx->mem_partials_lock);
			}
		}
	}
no_new_partial:
#endif

	if (nr_left) {
		res = kbase_mem_pool_alloc_pages(
			&kctx->mem_pools.small[alloc->group_id],
			nr_left, tp, false);
		if (res <= 0)
			goto alloc_failed;
	}

	KBASE_TLSTREAM_AUX_PAGESALLOC(
			kbdev,
			kctx->id,
			(u64)new_page_count);

	alloc->nents += nr_pages_requested;

	kbase_trace_gpu_mem_usage_inc(kctx->kbdev, kctx, nr_pages_requested);

done:
	return 0;

alloc_failed:
	/* rollback needed if got one or more 2MB but failed later */
	if (nr_left != nr_pages_requested) {
		size_t nr_pages_to_free = nr_pages_requested - nr_left;

		alloc->nents += nr_pages_to_free;

		kbase_process_page_usage_inc(kctx, nr_pages_to_free);
		atomic_add(nr_pages_to_free, &kctx->used_pages);
		atomic_add(nr_pages_to_free,
			&kctx->kbdev->memdev.used_pages);

		kbase_free_phy_pages_helper(alloc, nr_pages_to_free);
	}

	kbase_process_page_usage_dec(kctx, nr_pages_requested);
	atomic_sub(nr_pages_requested, &kctx->used_pages);
	atomic_sub(nr_pages_requested,
		&kctx->kbdev->memdev.used_pages);

invalid_request:
	return -ENOMEM;
}

struct tagged_addr *kbase_alloc_phy_pages_helper_locked(
		struct kbase_mem_phy_alloc *alloc, struct kbase_mem_pool *pool,
		size_t nr_pages_requested,
		struct kbase_sub_alloc **prealloc_sa)
{
	int new_page_count __maybe_unused;
	size_t nr_left = nr_pages_requested;
	int res;
	struct kbase_context *kctx;
	struct kbase_device *kbdev;
	struct tagged_addr *tp;
	struct tagged_addr *new_pages = NULL;

	KBASE_DEBUG_ASSERT(alloc->type == KBASE_MEM_TYPE_NATIVE);
	KBASE_DEBUG_ASSERT(alloc->imported.native.kctx);

	lockdep_assert_held(&pool->pool_lock);

#if !defined(CONFIG_MALI_2MB_ALLOC)
	WARN_ON(pool->order);
#endif

	if (alloc->reg) {
		if (nr_pages_requested > alloc->reg->nr_pages - alloc->nents)
			goto invalid_request;
	}

	kctx = alloc->imported.native.kctx;
	kbdev = kctx->kbdev;

	lockdep_assert_held(&kctx->mem_partials_lock);

	if (nr_pages_requested == 0)
		goto done; /*nothing to do*/

	new_page_count = atomic_add_return(
		nr_pages_requested, &kctx->used_pages);
	atomic_add(nr_pages_requested,
		&kctx->kbdev->memdev.used_pages);

	/* Increase mm counters before we allocate pages so that this
	 * allocation is visible to the OOM killer
	 */
	kbase_process_page_usage_inc(kctx, nr_pages_requested);

	tp = alloc->pages + alloc->nents;
	new_pages = tp;

#ifdef CONFIG_MALI_2MB_ALLOC
	if (pool->order) {
		int nr_lp = nr_left / (SZ_2M / SZ_4K);

		res = kbase_mem_pool_alloc_pages_locked(pool,
						 nr_lp * (SZ_2M / SZ_4K),
						 tp);

		if (res > 0) {
			nr_left -= res;
			tp += res;
		}

		if (nr_left) {
			struct kbase_sub_alloc *sa, *temp_sa;

			list_for_each_entry_safe(sa, temp_sa,
						 &kctx->mem_partials, link) {
				int pidx = 0;

				while (nr_left) {
					pidx = find_next_zero_bit(sa->sub_pages,
								  SZ_2M / SZ_4K,
								  pidx);
					bitmap_set(sa->sub_pages, pidx, 1);
					*tp++ = as_tagged_tag(page_to_phys(
							sa->page + pidx),
							FROM_PARTIAL);
					nr_left--;

					if (bitmap_full(sa->sub_pages,
							SZ_2M / SZ_4K)) {
						/* unlink from partial list when
						 * full
						 */
						list_del_init(&sa->link);
						break;
					}
				}
			}
		}

		/* only if we actually have a chunk left <512. If more it
		 * indicates that we couldn't allocate a 2MB above, so no point
		 * to retry here.
		 */
		if (nr_left > 0 && nr_left < (SZ_2M / SZ_4K)) {
			/* create a new partial and suballocate the rest from it
			 */
			struct page *np = NULL;

			np = kbase_mem_pool_alloc_locked(pool);

			if (np) {
				int i;
				struct kbase_sub_alloc *const sa = *prealloc_sa;
				struct page *p;

				/* store pointers back to the control struct */
				np->lru.next = (void *)sa;
				for (p = np; p < np + SZ_2M / SZ_4K; p++)
					p->lru.prev = (void *)np;
				INIT_LIST_HEAD(&sa->link);
				bitmap_zero(sa->sub_pages, SZ_2M / SZ_4K);
				sa->page = np;

				for (i = 0; i < nr_left; i++)
					*tp++ = as_tagged_tag(
							page_to_phys(np + i),
							FROM_PARTIAL);

				bitmap_set(sa->sub_pages, 0, nr_left);
				nr_left = 0;
				/* Indicate to user that we'll free this memory
				 * later.
				 */
				*prealloc_sa = NULL;

				/* expose for later use */
				list_add(&sa->link, &kctx->mem_partials);
			}
		}
		if (nr_left)
			goto alloc_failed;
	} else {
#endif
		res = kbase_mem_pool_alloc_pages_locked(pool,
						 nr_left,
						 tp);
		if (res <= 0)
			goto alloc_failed;
#ifdef CONFIG_MALI_2MB_ALLOC
	}
#endif

	KBASE_TLSTREAM_AUX_PAGESALLOC(
			kbdev,
			kctx->id,
			(u64)new_page_count);

	alloc->nents += nr_pages_requested;

	kbase_trace_gpu_mem_usage_inc(kctx->kbdev, kctx, nr_pages_requested);

done:
	return new_pages;

alloc_failed:
	/* rollback needed if got one or more 2MB but failed later */
	if (nr_left != nr_pages_requested) {
		size_t nr_pages_to_free = nr_pages_requested - nr_left;

		struct tagged_addr *start_free = alloc->pages + alloc->nents;

#ifdef CONFIG_MALI_2MB_ALLOC
		if (pool->order) {
			while (nr_pages_to_free) {
				if (is_huge_head(*start_free)) {
					kbase_mem_pool_free_pages_locked(
						pool, 512,
						start_free,
						false, /* not dirty */
						true); /* return to pool */
					nr_pages_to_free -= 512;
					start_free += 512;
				} else if (is_partial(*start_free)) {
					free_partial_locked(kctx, pool,
							*start_free);
					nr_pages_to_free--;
					start_free++;
				}
			}
		} else {
#endif
			kbase_mem_pool_free_pages_locked(pool,
					nr_pages_to_free,
					start_free,
					false, /* not dirty */
					true); /* return to pool */
#ifdef CONFIG_MALI_2MB_ALLOC
		}
#endif
	}

	kbase_process_page_usage_dec(kctx, nr_pages_requested);
	atomic_sub(nr_pages_requested, &kctx->used_pages);
	atomic_sub(nr_pages_requested, &kctx->kbdev->memdev.used_pages);

invalid_request:
	return NULL;
}

static void free_partial(struct kbase_context *kctx, int group_id, struct
		tagged_addr tp)
{
	struct page *p, *head_page;
	struct kbase_sub_alloc *sa;

	p = as_page(tp);
	head_page = (struct page *)p->lru.prev;
	sa = (struct kbase_sub_alloc *)head_page->lru.next;
	spin_lock(&kctx->mem_partials_lock);
	clear_bit(p - head_page, sa->sub_pages);
	if (bitmap_empty(sa->sub_pages, SZ_2M / SZ_4K)) {
		list_del(&sa->link);
		kbase_mem_pool_free(
			&kctx->mem_pools.large[group_id],
			head_page,
			true);
		kfree(sa);
	} else if (bitmap_weight(sa->sub_pages, SZ_2M / SZ_4K) ==
		   SZ_2M / SZ_4K - 1) {
		/* expose the partial again */
		list_add(&sa->link, &kctx->mem_partials);
	}
	spin_unlock(&kctx->mem_partials_lock);
}

int kbase_free_phy_pages_helper(
	struct kbase_mem_phy_alloc *alloc,
	size_t nr_pages_to_free)
{
	struct kbase_context *kctx = alloc->imported.native.kctx;
	struct kbase_device *kbdev = kctx->kbdev;
	bool syncback;
	bool reclaimed = (alloc->evicted != 0);
	struct tagged_addr *start_free;
	int new_page_count __maybe_unused;
	size_t freed = 0;

	if (WARN_ON(alloc->type != KBASE_MEM_TYPE_NATIVE) ||
	    WARN_ON(alloc->imported.native.kctx == NULL) ||
	    WARN_ON(alloc->nents < nr_pages_to_free) ||
	    WARN_ON(alloc->group_id >= MEMORY_GROUP_MANAGER_NR_GROUPS)) {
		return -EINVAL;
	}

	/* early out if nothing to do */
	if (0 == nr_pages_to_free)
		return 0;

	start_free = alloc->pages + alloc->nents - nr_pages_to_free;

	syncback = alloc->properties & KBASE_MEM_PHY_ALLOC_ACCESSED_CACHED;

	/* pad start_free to a valid start location */
	while (nr_pages_to_free && is_huge(*start_free) &&
	       !is_huge_head(*start_free)) {
		nr_pages_to_free--;
		start_free++;
	}

	while (nr_pages_to_free) {
		if (is_huge_head(*start_free)) {
			/* This is a 2MB entry, so free all the 512 pages that
			 * it points to
			 */
			kbase_mem_pool_free_pages(
				&kctx->mem_pools.large[alloc->group_id],
				512,
				start_free,
				syncback,
				reclaimed);
			nr_pages_to_free -= 512;
			start_free += 512;
			freed += 512;
		} else if (is_partial(*start_free)) {
			free_partial(kctx, alloc->group_id, *start_free);
			nr_pages_to_free--;
			start_free++;
			freed++;
		} else {
			struct tagged_addr *local_end_free;

			local_end_free = start_free;
			while (nr_pages_to_free &&
				!is_huge(*local_end_free) &&
				!is_partial(*local_end_free)) {
				local_end_free++;
				nr_pages_to_free--;
			}
			kbase_mem_pool_free_pages(
				&kctx->mem_pools.small[alloc->group_id],
				local_end_free - start_free,
				start_free,
				syncback,
				reclaimed);
			freed += local_end_free - start_free;
			start_free += local_end_free - start_free;
		}
	}

	alloc->nents -= freed;

	/*
	 * If the allocation was not evicted (i.e. evicted == 0) then
	 * the page accounting needs to be done.
	 */
	if (!reclaimed) {
		kbase_process_page_usage_dec(kctx, freed);
		new_page_count = atomic_sub_return(freed,
			&kctx->used_pages);
		atomic_sub(freed,
			&kctx->kbdev->memdev.used_pages);

		KBASE_TLSTREAM_AUX_PAGESALLOC(
			kbdev,
			kctx->id,
			(u64)new_page_count);

		kbase_trace_gpu_mem_usage_dec(kctx->kbdev, kctx, freed);
	}

	return 0;
}

static void free_partial_locked(struct kbase_context *kctx,
		struct kbase_mem_pool *pool, struct tagged_addr tp)
{
	struct page *p, *head_page;
	struct kbase_sub_alloc *sa;

	lockdep_assert_held(&pool->pool_lock);
	lockdep_assert_held(&kctx->mem_partials_lock);

	p = as_page(tp);
	head_page = (struct page *)p->lru.prev;
	sa = (struct kbase_sub_alloc *)head_page->lru.next;
	clear_bit(p - head_page, sa->sub_pages);
	if (bitmap_empty(sa->sub_pages, SZ_2M / SZ_4K)) {
		list_del(&sa->link);
		kbase_mem_pool_free_locked(pool, head_page, true);
		kfree(sa);
	} else if (bitmap_weight(sa->sub_pages, SZ_2M / SZ_4K) ==
		   SZ_2M / SZ_4K - 1) {
		/* expose the partial again */
		list_add(&sa->link, &kctx->mem_partials);
	}
}

void kbase_free_phy_pages_helper_locked(struct kbase_mem_phy_alloc *alloc,
		struct kbase_mem_pool *pool, struct tagged_addr *pages,
		size_t nr_pages_to_free)
{
	struct kbase_context *kctx = alloc->imported.native.kctx;
	struct kbase_device *kbdev = kctx->kbdev;
	bool syncback;
	bool reclaimed = (alloc->evicted != 0);
	struct tagged_addr *start_free;
	size_t freed = 0;

	KBASE_DEBUG_ASSERT(alloc->type == KBASE_MEM_TYPE_NATIVE);
	KBASE_DEBUG_ASSERT(alloc->imported.native.kctx);
	KBASE_DEBUG_ASSERT(alloc->nents >= nr_pages_to_free);

	lockdep_assert_held(&pool->pool_lock);
	lockdep_assert_held(&kctx->mem_partials_lock);

	/* early out if nothing to do */
	if (!nr_pages_to_free)
		return;

	start_free = pages;

	syncback = alloc->properties & KBASE_MEM_PHY_ALLOC_ACCESSED_CACHED;

	/* pad start_free to a valid start location */
	while (nr_pages_to_free && is_huge(*start_free) &&
	       !is_huge_head(*start_free)) {
		nr_pages_to_free--;
		start_free++;
	}

	while (nr_pages_to_free) {
		if (is_huge_head(*start_free)) {
			/* This is a 2MB entry, so free all the 512 pages that
			 * it points to
			 */
			WARN_ON(!pool->order);
			kbase_mem_pool_free_pages_locked(pool,
					512,
					start_free,
					syncback,
					reclaimed);
			nr_pages_to_free -= 512;
			start_free += 512;
			freed += 512;
		} else if (is_partial(*start_free)) {
			WARN_ON(!pool->order);
			free_partial_locked(kctx, pool, *start_free);
			nr_pages_to_free--;
			start_free++;
			freed++;
		} else {
			struct tagged_addr *local_end_free;

			WARN_ON(pool->order);
			local_end_free = start_free;
			while (nr_pages_to_free &&
			       !is_huge(*local_end_free) &&
			       !is_partial(*local_end_free)) {
				local_end_free++;
				nr_pages_to_free--;
			}
			kbase_mem_pool_free_pages_locked(pool,
					local_end_free - start_free,
					start_free,
					syncback,
					reclaimed);
			freed += local_end_free - start_free;
			start_free += local_end_free - start_free;
		}
	}

	alloc->nents -= freed;

	/*
	 * If the allocation was not evicted (i.e. evicted == 0) then
	 * the page accounting needs to be done.
	 */
	if (!reclaimed) {
		int new_page_count;

		kbase_process_page_usage_dec(kctx, freed);
		new_page_count = atomic_sub_return(freed,
			&kctx->used_pages);
		atomic_sub(freed,
			&kctx->kbdev->memdev.used_pages);

		KBASE_TLSTREAM_AUX_PAGESALLOC(
				kbdev,
				kctx->id,
				(u64)new_page_count);

		kbase_trace_gpu_mem_usage_dec(kctx->kbdev, kctx, freed);
	}
}


void kbase_mem_kref_free(struct kref *kref)
{
	struct kbase_mem_phy_alloc *alloc;

	alloc = container_of(kref, struct kbase_mem_phy_alloc, kref);

	switch (alloc->type) {
	case KBASE_MEM_TYPE_NATIVE: {

		if (!WARN_ON(!alloc->imported.native.kctx)) {
			if (alloc->permanent_map)
				kbase_phy_alloc_mapping_term(
						alloc->imported.native.kctx,
						alloc);

			/*
			 * The physical allocation must have been removed from
			 * the eviction list before trying to free it.
			 */
			mutex_lock(
				&alloc->imported.native.kctx->jit_evict_lock);
			WARN_ON(!list_empty(&alloc->evict_node));
			mutex_unlock(
				&alloc->imported.native.kctx->jit_evict_lock);

			kbase_process_page_usage_dec(
					alloc->imported.native.kctx,
					alloc->imported.native.nr_struct_pages);
		}
		kbase_free_phy_pages_helper(alloc, alloc->nents);
		break;
	}
	case KBASE_MEM_TYPE_ALIAS: {
		/* just call put on the underlying phy allocs */
		size_t i;
		struct kbase_aliased *aliased;

		aliased = alloc->imported.alias.aliased;
		if (aliased) {
			for (i = 0; i < alloc->imported.alias.nents; i++)
				if (aliased[i].alloc) {
					kbase_mem_phy_alloc_gpu_unmapped(aliased[i].alloc);
					kbase_mem_phy_alloc_put(aliased[i].alloc);
				}
			vfree(aliased);
		}
		break;
	}
	case KBASE_MEM_TYPE_RAW:
		/* raw pages, external cleanup */
		break;
	case KBASE_MEM_TYPE_IMPORTED_UMM:
		if (!IS_ENABLED(CONFIG_MALI_DMA_BUF_MAP_ON_DEMAND)) {
			WARN_ONCE(alloc->imported.umm.current_mapping_usage_count != 1,
					"WARNING: expected excatly 1 mapping, got %d",
					alloc->imported.umm.current_mapping_usage_count);
			dma_buf_unmap_attachment(
					alloc->imported.umm.dma_attachment,
					alloc->imported.umm.sgt,
					DMA_BIDIRECTIONAL);
			kbase_remove_dma_buf_usage(alloc->imported.umm.kctx,
						   alloc);
		}
		dma_buf_detach(alloc->imported.umm.dma_buf,
			       alloc->imported.umm.dma_attachment);
		dma_buf_put(alloc->imported.umm.dma_buf);
		break;
	case KBASE_MEM_TYPE_IMPORTED_USER_BUF:
		if (alloc->imported.user_buf.mm)
			mmdrop(alloc->imported.user_buf.mm);
		if (alloc->properties & KBASE_MEM_PHY_ALLOC_LARGE)
			vfree(alloc->imported.user_buf.pages);
		else
			kfree(alloc->imported.user_buf.pages);
		break;
	default:
		WARN(1, "Unexecpted free of type %d\n", alloc->type);
		break;
	}

	/* Free based on allocation type */
	if (alloc->properties & KBASE_MEM_PHY_ALLOC_LARGE)
		vfree(alloc);
	else
		kfree(alloc);
}

KBASE_EXPORT_TEST_API(kbase_mem_kref_free);

int kbase_alloc_phy_pages(struct kbase_va_region *reg, size_t vsize, size_t size)
{
	KBASE_DEBUG_ASSERT(NULL != reg);
	KBASE_DEBUG_ASSERT(vsize > 0);

	/* validate user provided arguments */
	if (size > vsize || vsize > reg->nr_pages)
		goto out_term;

	/* Prevent vsize*sizeof from wrapping around.
	 * For instance, if vsize is 2**29+1, we'll allocate 1 byte and the alloc won't fail.
	 */
	if ((size_t) vsize > ((size_t) -1 / sizeof(*reg->cpu_alloc->pages)))
		goto out_term;

	KBASE_DEBUG_ASSERT(0 != vsize);

	if (kbase_alloc_phy_pages_helper(reg->cpu_alloc, size) != 0)
		goto out_term;

	reg->cpu_alloc->reg = reg;
	if (reg->cpu_alloc != reg->gpu_alloc) {
		if (kbase_alloc_phy_pages_helper(reg->gpu_alloc, size) != 0)
			goto out_rollback;
		reg->gpu_alloc->reg = reg;
	}

	return 0;

out_rollback:
	kbase_free_phy_pages_helper(reg->cpu_alloc, size);
out_term:
	return -1;
}

KBASE_EXPORT_TEST_API(kbase_alloc_phy_pages);

bool kbase_check_alloc_flags(unsigned long flags)
{
	/* Only known input flags should be set. */
	if (flags & ~BASE_MEM_FLAGS_INPUT_MASK)
		return false;

	/* At least one flag should be set */
	if (flags == 0)
		return false;

	/* Either the GPU or CPU must be reading from the allocated memory */
	if ((flags & (BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_GPU_RD)) == 0)
		return false;

	/* Either the GPU or CPU must be writing to the allocated memory */
	if ((flags & (BASE_MEM_PROT_CPU_WR | BASE_MEM_PROT_GPU_WR)) == 0)
		return false;

	/* GPU executable memory cannot:
	 * - Be written by the GPU
	 * - Be grown on GPU page fault
	 */
	if ((flags & BASE_MEM_PROT_GPU_EX) && (flags &
			(BASE_MEM_PROT_GPU_WR | BASE_MEM_GROW_ON_GPF)))
		return false;

	/* GPU executable memory also cannot have the top of its initial
	 * commit aligned to 'extent'
	 */
	if ((flags & BASE_MEM_PROT_GPU_EX) && (flags &
			BASE_MEM_TILER_ALIGN_TOP))
		return false;

	/* To have an allocation lie within a 4GB chunk is required only for
	 * TLS memory, which will never be used to contain executable code.
	 */
	if ((flags & BASE_MEM_GPU_VA_SAME_4GB_PAGE) && (flags &
			BASE_MEM_PROT_GPU_EX))
		return false;

	/* TLS memory should also not be used for tiler heap */
	if ((flags & BASE_MEM_GPU_VA_SAME_4GB_PAGE) && (flags &
			BASE_MEM_TILER_ALIGN_TOP))
		return false;

	/* GPU should have at least read or write access otherwise there is no
	   reason for allocating. */
	if ((flags & (BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR)) == 0)
		return false;

	/* BASE_MEM_IMPORT_SHARED is only valid for imported memory */
	if ((flags & BASE_MEM_IMPORT_SHARED) == BASE_MEM_IMPORT_SHARED)
		return false;

	/* BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP is only valid for imported
	 * memory */
	if ((flags & BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP) ==
			BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP)
		return false;

	/* Should not combine BASE_MEM_COHERENT_LOCAL with
	 * BASE_MEM_COHERENT_SYSTEM */
	if ((flags & (BASE_MEM_COHERENT_LOCAL | BASE_MEM_COHERENT_SYSTEM)) ==
			(BASE_MEM_COHERENT_LOCAL | BASE_MEM_COHERENT_SYSTEM))
		return false;

	return true;
}

bool kbase_check_import_flags(unsigned long flags)
{
	/* Only known input flags should be set. */
	if (flags & ~BASE_MEM_FLAGS_INPUT_MASK)
		return false;

	/* At least one flag should be set */
	if (flags == 0)
		return false;

	/* Imported memory cannot be GPU executable */
	if (flags & BASE_MEM_PROT_GPU_EX)
		return false;

	/* Imported memory cannot grow on page fault */
	if (flags & BASE_MEM_GROW_ON_GPF)
		return false;

	/* Imported memory cannot be aligned to the end of its initial commit */
	if (flags & BASE_MEM_TILER_ALIGN_TOP)
		return false;

	/* GPU should have at least read or write access otherwise there is no
	   reason for importing. */
	if ((flags & (BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR)) == 0)
		return false;

	/* Protected memory cannot be read by the CPU */
	if ((flags & BASE_MEM_PROTECTED) && (flags & BASE_MEM_PROT_CPU_RD))
		return false;

	return true;
}

int kbase_check_alloc_sizes(struct kbase_context *kctx, unsigned long flags,
		u64 va_pages, u64 commit_pages, u64 large_extent)
{
	struct device *dev = kctx->kbdev->dev;
	int gpu_pc_bits = kctx->kbdev->gpu_props.props.core_props.log2_program_counter_size;
	u64 gpu_pc_pages_max = 1ULL << gpu_pc_bits >> PAGE_SHIFT;
	struct kbase_va_region test_reg;

	/* kbase_va_region's extent member can be of variable size, so check against that type */
	test_reg.extent = large_extent;

#define KBASE_MSG_PRE "GPU allocation attempted with "

	if (0 == va_pages) {
		dev_warn(dev, KBASE_MSG_PRE "0 va_pages!");
		return -EINVAL;
	}

	if (va_pages > KBASE_MEM_ALLOC_MAX_SIZE) {
		dev_warn(dev, KBASE_MSG_PRE "va_pages==%lld larger than KBASE_MEM_ALLOC_MAX_SIZE!",
				(unsigned long long)va_pages);
		return -ENOMEM;
	}

	/* Note: commit_pages is checked against va_pages during
	 * kbase_alloc_phy_pages() */

	/* Limit GPU executable allocs to GPU PC size */
	if ((flags & BASE_MEM_PROT_GPU_EX) && (va_pages > gpu_pc_pages_max)) {
		dev_warn(dev, KBASE_MSG_PRE "BASE_MEM_PROT_GPU_EX and va_pages==%lld larger than GPU PC range %lld",
				(unsigned long long)va_pages,
				(unsigned long long)gpu_pc_pages_max);

		return -EINVAL;
	}

	if ((flags & BASE_MEM_GROW_ON_GPF) && (test_reg.extent == 0)) {
		dev_warn(dev, KBASE_MSG_PRE "BASE_MEM_GROW_ON_GPF but extent == 0\n");
		return -EINVAL;
	}

	if ((flags & BASE_MEM_TILER_ALIGN_TOP) && (test_reg.extent == 0)) {
		dev_warn(dev, KBASE_MSG_PRE "BASE_MEM_TILER_ALIGN_TOP but extent == 0\n");
		return -EINVAL;
	}

	if (!(flags & (BASE_MEM_GROW_ON_GPF | BASE_MEM_TILER_ALIGN_TOP)) &&
			test_reg.extent != 0) {
		dev_warn(dev, KBASE_MSG_PRE "neither BASE_MEM_GROW_ON_GPF nor BASE_MEM_TILER_ALIGN_TOP set but extent != 0\n");
		return -EINVAL;
	}

	/* BASE_MEM_TILER_ALIGN_TOP memory has a number of restrictions */
	if (flags & BASE_MEM_TILER_ALIGN_TOP) {
#define KBASE_MSG_PRE_FLAG KBASE_MSG_PRE "BASE_MEM_TILER_ALIGN_TOP and "
		unsigned long small_extent;

		if (large_extent > BASE_MEM_TILER_ALIGN_TOP_EXTENT_MAX_PAGES) {
			dev_warn(dev, KBASE_MSG_PRE_FLAG "extent==%lld pages exceeds limit %lld",
					(unsigned long long)large_extent,
					BASE_MEM_TILER_ALIGN_TOP_EXTENT_MAX_PAGES);
			return -EINVAL;
		}
		/* For use with is_power_of_2, which takes unsigned long, so
		 * must ensure e.g. on 32-bit kernel it'll fit in that type */
		small_extent = (unsigned long)large_extent;

		if (!is_power_of_2(small_extent)) {
			dev_warn(dev, KBASE_MSG_PRE_FLAG "extent==%ld not a non-zero power of 2",
					small_extent);
			return -EINVAL;
		}

		if (commit_pages > large_extent) {
			dev_warn(dev, KBASE_MSG_PRE_FLAG "commit_pages==%ld exceeds extent==%ld",
					(unsigned long)commit_pages,
					(unsigned long)large_extent);
			return -EINVAL;
		}
#undef KBASE_MSG_PRE_FLAG
	}

	if ((flags & BASE_MEM_GPU_VA_SAME_4GB_PAGE) &&
	    (va_pages > (BASE_MEM_PFN_MASK_4GB + 1))) {
		dev_warn(dev, KBASE_MSG_PRE "BASE_MEM_GPU_VA_SAME_4GB_PAGE and va_pages==%lld greater than that needed for 4GB space",
				(unsigned long long)va_pages);
		return -EINVAL;
	}

	return 0;
#undef KBASE_MSG_PRE
}

/**
 * @brief Acquire the per-context region list lock
 */
void kbase_gpu_vm_lock(struct kbase_context *kctx)
{
	KBASE_DEBUG_ASSERT(kctx != NULL);
	mutex_lock(&kctx->reg_lock);
}

KBASE_EXPORT_TEST_API(kbase_gpu_vm_lock);

/**
 * @brief Release the per-context region list lock
 */
void kbase_gpu_vm_unlock(struct kbase_context *kctx)
{
	KBASE_DEBUG_ASSERT(kctx != NULL);
	mutex_unlock(&kctx->reg_lock);
}

KBASE_EXPORT_TEST_API(kbase_gpu_vm_unlock);

#if 0
struct kbase_jit_debugfs_data {
	int (*func)(struct kbase_jit_debugfs_data *);
	struct mutex lock;
	struct kbase_context *kctx;
	u64 active_value;
	u64 pool_value;
	u64 destroy_value;
	char buffer[50];
};

static int kbase_jit_debugfs_common_open(struct inode *inode,
		struct file *file, int (*func)(struct kbase_jit_debugfs_data *))
{
	struct kbase_jit_debugfs_data *data;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->func = func;
	mutex_init(&data->lock);
	data->kctx = (struct kbase_context *) inode->i_private;

	file->private_data = data;

	return nonseekable_open(inode, file);
}

static ssize_t kbase_jit_debugfs_common_read(struct file *file,
		char __user *buf, size_t len, loff_t *ppos)
{
	struct kbase_jit_debugfs_data *data;
	size_t size;
	int ret;

	data = (struct kbase_jit_debugfs_data *) file->private_data;
	mutex_lock(&data->lock);

	if (*ppos) {
		size = strnlen(data->buffer, sizeof(data->buffer));
	} else {
		if (!data->func) {
			ret = -EACCES;
			goto out_unlock;
		}

		if (data->func(data)) {
			ret = -EACCES;
			goto out_unlock;
		}

		size = scnprintf(data->buffer, sizeof(data->buffer),
				"%llu,%llu,%llu", data->active_value,
				data->pool_value, data->destroy_value);
	}

	ret = simple_read_from_buffer(buf, len, ppos, data->buffer, size);

out_unlock:
	mutex_unlock(&data->lock);
	return ret;
}

static int kbase_jit_debugfs_common_release(struct inode *inode,
		struct file *file)
{
	kfree(file->private_data);
	return 0;
}

#define KBASE_JIT_DEBUGFS_DECLARE(__fops, __func) \
static int __fops ## _open(struct inode *inode, struct file *file) \
{ \
	return kbase_jit_debugfs_common_open(inode, file, __func); \
} \
static const struct file_operations __fops = { \
	.owner = THIS_MODULE, \
	.open = __fops ## _open, \
	.release = kbase_jit_debugfs_common_release, \
	.read = kbase_jit_debugfs_common_read, \
	.write = NULL, \
	.llseek = generic_file_llseek, \
}

static int kbase_jit_debugfs_count_get(struct kbase_jit_debugfs_data *data)
{
	struct kbase_context *kctx = data->kctx;
	struct list_head *tmp;

	mutex_lock(&kctx->jit_evict_lock);
	list_for_each(tmp, &kctx->jit_active_head) {
		data->active_value++;
	}

	list_for_each(tmp, &kctx->jit_pool_head) {
		data->pool_value++;
	}

	list_for_each(tmp, &kctx->jit_destroy_head) {
		data->destroy_value++;
	}
	mutex_unlock(&kctx->jit_evict_lock);

	return 0;
}
KBASE_JIT_DEBUGFS_DECLARE(kbase_jit_debugfs_count_fops,
		kbase_jit_debugfs_count_get);

static int kbase_jit_debugfs_vm_get(struct kbase_jit_debugfs_data *data)
{
	struct kbase_context *kctx = data->kctx;
	struct kbase_va_region *reg;

	mutex_lock(&kctx->jit_evict_lock);
	list_for_each_entry(reg, &kctx->jit_active_head, jit_node) {
		data->active_value += reg->nr_pages;
	}

	list_for_each_entry(reg, &kctx->jit_pool_head, jit_node) {
		data->pool_value += reg->nr_pages;
	}

	list_for_each_entry(reg, &kctx->jit_destroy_head, jit_node) {
		data->destroy_value += reg->nr_pages;
	}
	mutex_unlock(&kctx->jit_evict_lock);

	return 0;
}
KBASE_JIT_DEBUGFS_DECLARE(kbase_jit_debugfs_vm_fops,
		kbase_jit_debugfs_vm_get);

static int kbase_jit_debugfs_phys_get(struct kbase_jit_debugfs_data *data)
{
	struct kbase_context *kctx = data->kctx;
	struct kbase_va_region *reg;

	mutex_lock(&kctx->jit_evict_lock);
	list_for_each_entry(reg, &kctx->jit_active_head, jit_node) {
		data->active_value += reg->gpu_alloc->nents;
	}

	list_for_each_entry(reg, &kctx->jit_pool_head, jit_node) {
		data->pool_value += reg->gpu_alloc->nents;
	}

	list_for_each_entry(reg, &kctx->jit_destroy_head, jit_node) {
		data->destroy_value += reg->gpu_alloc->nents;
	}
	mutex_unlock(&kctx->jit_evict_lock);

	return 0;
}
KBASE_JIT_DEBUGFS_DECLARE(kbase_jit_debugfs_phys_fops,
		kbase_jit_debugfs_phys_get);

#if MALI_JIT_PRESSURE_LIMIT_BASE
static int kbase_jit_debugfs_used_get(struct kbase_jit_debugfs_data *data)
{
	struct kbase_context *kctx = data->kctx;
	struct kbase_va_region *reg;

	mutex_lock(&kctx->jctx.lock);
	mutex_lock(&kctx->jit_evict_lock);
	list_for_each_entry(reg, &kctx->jit_active_head, jit_node) {
		data->active_value += reg->used_pages;
	}
	mutex_unlock(&kctx->jit_evict_lock);
	mutex_unlock(&kctx->jctx.lock);

	return 0;
}

KBASE_JIT_DEBUGFS_DECLARE(kbase_jit_debugfs_used_fops,
		kbase_jit_debugfs_used_get);

static int kbase_mem_jit_trim_pages_from_region(struct kbase_context *kctx,
		struct kbase_va_region *reg, size_t pages_needed,
		size_t *freed, bool shrink);

static int kbase_jit_debugfs_trim_get(struct kbase_jit_debugfs_data *data)
{
	struct kbase_context *kctx = data->kctx;
	struct kbase_va_region *reg;

	mutex_lock(&kctx->jctx.lock);
	kbase_gpu_vm_lock(kctx);
	mutex_lock(&kctx->jit_evict_lock);
	list_for_each_entry(reg, &kctx->jit_active_head, jit_node) {
		int err;
		size_t freed = 0u;

		err = kbase_mem_jit_trim_pages_from_region(kctx, reg,
				SIZE_MAX, &freed, false);

		if (err) {
			/* Failed to calculate, try the next region */
			continue;
		}

		data->active_value += freed;
	}
	mutex_unlock(&kctx->jit_evict_lock);
	kbase_gpu_vm_unlock(kctx);
	mutex_unlock(&kctx->jctx.lock);

	return 0;
}

KBASE_JIT_DEBUGFS_DECLARE(kbase_jit_debugfs_trim_fops,
		kbase_jit_debugfs_trim_get);
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */

void kbase_jit_debugfs_init(struct kbase_context *kctx)
{
	/* prevent unprivileged use of debug file system
         * in old kernel version
         */
#if (KERNEL_VERSION(4, 7, 0) <= LINUX_VERSION_CODE)
	/* only for newer kernel version debug file system is safe */
	const mode_t mode = 0444;
#else
	const mode_t mode = 0400;
#endif

	/* Caller already ensures this, but we keep the pattern for
	 * maintenance safety.
	 */
	if (WARN_ON(!kctx) ||
		WARN_ON(IS_ERR_OR_NULL(kctx->kctx_dentry)))
		return;



	/* Debugfs entry for getting the number of JIT allocations. */
	debugfs_create_file("mem_jit_count", mode, kctx->kctx_dentry,
			kctx, &kbase_jit_debugfs_count_fops);

	/*
	 * Debugfs entry for getting the total number of virtual pages
	 * used by JIT allocations.
	 */
	debugfs_create_file("mem_jit_vm", mode, kctx->kctx_dentry,
			kctx, &kbase_jit_debugfs_vm_fops);

	/*
	 * Debugfs entry for getting the number of physical pages used
	 * by JIT allocations.
	 */
	debugfs_create_file("mem_jit_phys", mode, kctx->kctx_dentry,
			kctx, &kbase_jit_debugfs_phys_fops);
#if MALI_JIT_PRESSURE_LIMIT_BASE
	/*
	 * Debugfs entry for getting the number of pages used
	 * by JIT allocations for estimating the physical pressure
	 * limit.
	 */
	debugfs_create_file("mem_jit_used", mode, kctx->kctx_dentry,
			kctx, &kbase_jit_debugfs_used_fops);

	/*
	 * Debugfs entry for getting the number of pages that could
	 * be trimmed to free space for more JIT allocations.
	 */
	debugfs_create_file("mem_jit_trim", mode, kctx->kctx_dentry,
			kctx, &kbase_jit_debugfs_trim_fops);
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */
}
#endif /* CONFIG_DEBUG_FS */

/**
 * kbase_jit_destroy_worker - Deferred worker which frees JIT allocations
 * @work: Work item
 *
 * This function does the work of freeing JIT allocations whose physical
 * backing has been released.
 */
static void kbase_jit_destroy_worker(struct work_struct *work)
{
	struct kbase_context *kctx;
	struct kbase_va_region *reg;

	kctx = container_of(work, struct kbase_context, jit_work);
	do {
		mutex_lock(&kctx->jit_evict_lock);
		if (list_empty(&kctx->jit_destroy_head)) {
			mutex_unlock(&kctx->jit_evict_lock);
			break;
		}

		reg = list_first_entry(&kctx->jit_destroy_head,
				struct kbase_va_region, jit_node);

		list_del(&reg->jit_node);
		mutex_unlock(&kctx->jit_evict_lock);

		kbase_gpu_vm_lock(kctx);
		reg->flags &= ~KBASE_REG_NO_USER_FREE;
		kbase_mem_free_region(kctx, reg);
		kbase_gpu_vm_unlock(kctx);
	} while (1);
}

int kbase_jit_init(struct kbase_context *kctx)
{
	mutex_lock(&kctx->jit_evict_lock);
	INIT_LIST_HEAD(&kctx->jit_active_head);
	INIT_LIST_HEAD(&kctx->jit_pool_head);
	INIT_LIST_HEAD(&kctx->jit_destroy_head);
	INIT_WORK(&kctx->jit_work, kbase_jit_destroy_worker);

	INIT_LIST_HEAD(&kctx->jctx.jit_atoms_head);
	INIT_LIST_HEAD(&kctx->jctx.jit_pending_alloc);
	mutex_unlock(&kctx->jit_evict_lock);

	kctx->jit_max_allocations = 0;
	kctx->jit_current_allocations = 0;
	kctx->trim_level = 0;

	return 0;
}

/* Check if the allocation from JIT pool is of the same size as the new JIT
 * allocation and also, if BASE_JIT_ALLOC_MEM_TILER_ALIGN_TOP is set, meets
 * the alignment requirements.
 */
static bool meet_size_and_tiler_align_top_requirements(
	const struct kbase_va_region *walker,
	const struct base_jit_alloc_info *info)
{
	bool meet_reqs = true;

	if (walker->nr_pages != info->va_pages)
		meet_reqs = false;

	if (meet_reqs && (info->flags & BASE_JIT_ALLOC_MEM_TILER_ALIGN_TOP)) {
		size_t align = info->extent;
		size_t align_mask = align - 1;

		if ((walker->start_pfn + info->commit_pages) & align_mask)
			meet_reqs = false;
	}

	return meet_reqs;
}

#if MALI_JIT_PRESSURE_LIMIT_BASE
/* Function will guarantee *@freed will not exceed @pages_needed
 */
static int kbase_mem_jit_trim_pages_from_region(struct kbase_context *kctx,
		struct kbase_va_region *reg, size_t pages_needed,
		size_t *freed, bool shrink)
{
	int err = 0;
	size_t available_pages = 0u;
	const size_t old_pages = kbase_reg_current_backed_size(reg);
	size_t new_pages = old_pages;
	size_t to_free = 0u;
	size_t max_allowed_pages = old_pages;

	lockdep_assert_held(&kctx->jctx.lock);
	lockdep_assert_held(&kctx->reg_lock);

	/* Is this a JIT allocation that has been reported on? */
	if (reg->used_pages == reg->nr_pages)
		goto out;

	if (!(reg->flags & KBASE_REG_HEAP_INFO_IS_SIZE)) {
		/* For address based memory usage calculation, the GPU
		 * allocates objects of up to size 's', but aligns every object
		 * to alignment 'a', with a < s.
		 *
		 * It also doesn't have to write to all bytes in an object of
		 * size 's'.
		 *
		 * Hence, we can observe the GPU's address for the end of used
		 * memory being up to (s - a) bytes into the first unallocated
		 * page.
		 *
		 * We allow for this and only warn when it exceeds this bound
		 * (rounded up to page sized units). Note, this is allowed to
		 * exceed reg->nr_pages.
		 */
		max_allowed_pages += PFN_UP(
			KBASE_GPU_ALLOCATED_OBJECT_MAX_BYTES -
			KBASE_GPU_ALLOCATED_OBJECT_ALIGN_BYTES);
	} else if (reg->flags & KBASE_REG_TILER_ALIGN_TOP) {
		/* The GPU could report being ready to write to the next
		 * 'extent' sized chunk, but didn't actually write to it, so we
		 * can report up to 'extent' size pages more than the backed
		 * size.
		 *
		 * Note, this is allowed to exceed reg->nr_pages.
		 */
		max_allowed_pages += reg->extent;

		/* Also note that in these GPUs, the GPU may make a large (>1
		 * page) initial allocation but not actually write out to all
		 * of it. Hence it might report that a much higher amount of
		 * memory was used than actually was written to. This does not
		 * result in a real warning because on growing this memory we
		 * round up the size of the allocation up to an 'extent' sized
		 * chunk, hence automatically bringing the backed size up to
		 * the reported size.
		 */
	}

	if (old_pages < reg->used_pages) {
		/* Prevent overflow on available_pages, but only report the
		 * problem if it's in a scenario where used_pages should have
		 * been consistent with the backed size
		 *
		 * Note: In case of a size-based report, this legitimately
		 * happens in common use-cases: we allow for up to this size of
		 * memory being used, but depending on the content it doesn't
		 * have to use all of it.
		 *
		 * Hence, we're much more quiet about that in the size-based
		 * report case - it's not indicating a real problem, it's just
		 * for information
		 */
		if (max_allowed_pages < reg->used_pages) {
			if (!(reg->flags & KBASE_REG_HEAP_INFO_IS_SIZE))
				dev_warn(kctx->kbdev->dev,
						"%s: current backed pages %zu < reported used pages %zu (allowed to be up to %zu) on JIT 0x%llx vapages %zu\n",
						__func__,
						old_pages, reg->used_pages,
						max_allowed_pages,
						reg->start_pfn << PAGE_SHIFT,
						reg->nr_pages);
			else
				dev_dbg(kctx->kbdev->dev,
						"%s: no need to trim, current backed pages %zu < reported used pages %zu on size-report for JIT 0x%llx vapages %zu\n",
						__func__,
						old_pages, reg->used_pages,
						reg->start_pfn << PAGE_SHIFT,
						reg->nr_pages);
			}
		/* In any case, no error condition to report here, caller can
		 * try other regions
		 */

		goto out;
	}
	available_pages = old_pages - reg->used_pages;
	to_free = min(available_pages, pages_needed);

	if (shrink) {
		new_pages -= to_free;

		err = kbase_mem_shrink(kctx, reg, new_pages);
	}
out:
	trace_mali_jit_trim_from_region(reg, to_free, old_pages,
			available_pages, new_pages);
	*freed = to_free;
	return err;
}


/**
 * kbase_mem_jit_trim_pages - Trim JIT regions until sufficient pages have been
 * freed
 * @kctx: Pointer to the kbase context whose active JIT allocations will be
 * checked.
 * @pages_needed: The maximum number of pages to trim.
 *
 * This functions checks all active JIT allocations in @kctx for unused pages
 * at the end, and trim the backed memory regions of those allocations down to
 * the used portion and free the unused pages into the page pool.
 *
 * Specifying @pages_needed allows us to stop early when there's enough
 * physical memory freed to sufficiently bring down the total JIT physical page
 * usage (e.g. to below the pressure limit)
 *
 * Return: Total number of successfully freed pages
 */
static size_t kbase_mem_jit_trim_pages(struct kbase_context *kctx,
		size_t pages_needed)
{
	struct kbase_va_region *reg, *tmp;
	size_t total_freed = 0;

	lockdep_assert_held(&kctx->jctx.lock);
	lockdep_assert_held(&kctx->reg_lock);
	lockdep_assert_held(&kctx->jit_evict_lock);

	list_for_each_entry_safe(reg, tmp, &kctx->jit_active_head, jit_node) {
		int err;
		size_t freed = 0u;

		err = kbase_mem_jit_trim_pages_from_region(kctx, reg,
				pages_needed, &freed, true);

		if (err) {
			/* Failed to trim, try the next region */
			continue;
		}

		total_freed += freed;
		WARN_ON(freed > pages_needed);
		pages_needed -= freed;
		if (!pages_needed)
			break;
	}

	trace_mali_jit_trim(total_freed);

	return total_freed;
}
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */

static int kbase_jit_grow(struct kbase_context *kctx,
			  const struct base_jit_alloc_info *info,
			  struct kbase_va_region *reg,
			  struct kbase_sub_alloc **prealloc_sas)
{
	size_t delta;
	size_t pages_required;
	size_t old_size;
	struct kbase_mem_pool *pool;
	int ret = -ENOMEM;
	struct tagged_addr *gpu_pages;

	if (info->commit_pages > reg->nr_pages) {
		/* Attempted to grow larger than maximum size */
		return -EINVAL;
	}

	lockdep_assert_held(&kctx->reg_lock);

	/* Make the physical backing no longer reclaimable */
	if (!kbase_mem_evictable_unmake(reg->gpu_alloc))
		goto update_failed;

	if (reg->gpu_alloc->nents >= info->commit_pages)
		goto done;

	/* Grow the backing */
	old_size = reg->gpu_alloc->nents;

	/* Allocate some more pages */
	delta = info->commit_pages - reg->gpu_alloc->nents;
	pages_required = delta;

#ifdef CONFIG_MALI_2MB_ALLOC
	if (pages_required >= (SZ_2M / SZ_4K)) {
		pool = &kctx->mem_pools.large[kctx->jit_group_id];
		/* Round up to number of 2 MB pages required */
		pages_required += ((SZ_2M / SZ_4K) - 1);
		pages_required /= (SZ_2M / SZ_4K);
	} else {
#endif
		pool = &kctx->mem_pools.small[kctx->jit_group_id];
#ifdef CONFIG_MALI_2MB_ALLOC
	}
#endif

	if (reg->cpu_alloc != reg->gpu_alloc)
		pages_required *= 2;

	spin_lock(&kctx->mem_partials_lock);
	kbase_mem_pool_lock(pool);

	/* As we can not allocate memory from the kernel with the vm_lock held,
	 * grow the pool to the required size with the lock dropped. We hold the
	 * pool lock to prevent another thread from allocating from the pool
	 * between the grow and allocation.
	 */
	while (kbase_mem_pool_size(pool) < pages_required) {
		int pool_delta = pages_required - kbase_mem_pool_size(pool);
		int ret;

		kbase_mem_pool_unlock(pool);
		spin_unlock(&kctx->mem_partials_lock);

		kbase_gpu_vm_unlock(kctx);
		ret = kbase_mem_pool_grow(pool, pool_delta);
		kbase_gpu_vm_lock(kctx);

		if (ret)
			goto update_failed;

		spin_lock(&kctx->mem_partials_lock);
		kbase_mem_pool_lock(pool);
	}

	gpu_pages = kbase_alloc_phy_pages_helper_locked(reg->gpu_alloc, pool,
			delta, &prealloc_sas[0]);
	if (!gpu_pages) {
		kbase_mem_pool_unlock(pool);
		spin_unlock(&kctx->mem_partials_lock);
		goto update_failed;
	}

	if (reg->cpu_alloc != reg->gpu_alloc) {
		struct tagged_addr *cpu_pages;

		cpu_pages = kbase_alloc_phy_pages_helper_locked(reg->cpu_alloc,
				pool, delta, &prealloc_sas[1]);
		if (!cpu_pages) {
			kbase_free_phy_pages_helper_locked(reg->gpu_alloc,
					pool, gpu_pages, delta);
			kbase_mem_pool_unlock(pool);
			spin_unlock(&kctx->mem_partials_lock);
			goto update_failed;
		}
	}
	kbase_mem_pool_unlock(pool);
	spin_unlock(&kctx->mem_partials_lock);

	ret = kbase_mem_grow_gpu_mapping(kctx, reg, info->commit_pages,
			old_size);
	/*
	 * The grow failed so put the allocation back in the
	 * pool and return failure.
	 */
	if (ret)
		goto update_failed;

done:
	ret = 0;

	/* Update attributes of JIT allocation taken from the pool */
	reg->initial_commit = info->commit_pages;
	reg->extent = info->extent;

update_failed:
	return ret;
}

static void trace_jit_stats(struct kbase_context *kctx,
		u32 bin_id, u32 max_allocations)
{
	const u32 alloc_count =
		kctx->jit_current_allocations_per_bin[bin_id];
	struct kbase_device *kbdev = kctx->kbdev;

	struct kbase_va_region *walker;
	u32 va_pages = 0;
	u32 ph_pages = 0;

	mutex_lock(&kctx->jit_evict_lock);
	list_for_each_entry(walker, &kctx->jit_active_head, jit_node) {
		if (walker->jit_bin_id != bin_id)
			continue;

		va_pages += walker->nr_pages;
		ph_pages += walker->gpu_alloc->nents;
	}
	mutex_unlock(&kctx->jit_evict_lock);

	KBASE_TLSTREAM_AUX_JIT_STATS(kbdev, kctx->id, bin_id,
		max_allocations, alloc_count, va_pages, ph_pages);
}

#if MALI_JIT_PRESSURE_LIMIT_BASE
/**
 * get_jit_phys_backing() - calculate the physical backing of all JIT
 * allocations
 *
 * @kctx: Pointer to the kbase context whose active JIT allocations will be
 * checked
 *
 * Return: number of pages that are committed by JIT allocations
 */
static size_t get_jit_phys_backing(struct kbase_context *kctx)
{
	struct kbase_va_region *walker;
	size_t backing = 0;

	lockdep_assert_held(&kctx->jit_evict_lock);

	list_for_each_entry(walker, &kctx->jit_active_head, jit_node) {
		backing += kbase_reg_current_backed_size(walker);
	}

	return backing;
}

void kbase_jit_trim_necessary_pages(struct kbase_context *kctx,
				    size_t needed_pages)
{
	size_t jit_backing = 0;
	size_t pages_to_trim = 0;

	lockdep_assert_held(&kctx->jctx.lock);
	lockdep_assert_held(&kctx->reg_lock);
	lockdep_assert_held(&kctx->jit_evict_lock);

	jit_backing = get_jit_phys_backing(kctx);

	/* It is possible that this is the case - if this is the first
	 * allocation after "ignore_pressure_limit" allocation.
	 */
	if (jit_backing > kctx->jit_phys_pages_limit) {
		pages_to_trim += (jit_backing - kctx->jit_phys_pages_limit) +
				 needed_pages;
	} else {
		size_t backed_diff = kctx->jit_phys_pages_limit - jit_backing;

		if (needed_pages > backed_diff)
			pages_to_trim += needed_pages - backed_diff;
	}

	if (pages_to_trim) {
		size_t trimmed_pages =
			kbase_mem_jit_trim_pages(kctx, pages_to_trim);

		/* This should never happen - we already asserted that
		 * we are not violating JIT pressure limit in earlier
		 * checks, which means that in-flight JIT allocations
		 * must have enough unused pages to satisfy the new
		 * allocation
		 */
		WARN_ON(trimmed_pages < pages_to_trim);
	}
}
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */

/**
 * jit_allow_allocate() - check whether basic conditions are satisfied to allow
 * a new JIT allocation
 *
 * @kctx: Pointer to the kbase context
 * @info: Pointer to JIT allocation information for the new allocation
 * @ignore_pressure_limit: Flag to indicate whether JIT pressure limit check
 * should be ignored
 *
 * Return: true if allocation can be executed, false otherwise
 */
static bool jit_allow_allocate(struct kbase_context *kctx,
		const struct base_jit_alloc_info *info,
		bool ignore_pressure_limit)
{
	lockdep_assert_held(&kctx->jctx.lock);

#if MALI_JIT_PRESSURE_LIMIT_BASE
	if (!ignore_pressure_limit &&
			((kctx->jit_phys_pages_limit <= kctx->jit_current_phys_pressure) ||
			(info->va_pages > (kctx->jit_phys_pages_limit - kctx->jit_current_phys_pressure)))) {
		dev_dbg(kctx->kbdev->dev,
			"Max JIT page allocations limit reached: active pages %llu, max pages %llu\n",
			kctx->jit_current_phys_pressure + info->va_pages,
			kctx->jit_phys_pages_limit);
		return false;
	}
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */

	if (kctx->jit_current_allocations >= kctx->jit_max_allocations) {
		/* Too many current allocations */
		dev_dbg(kctx->kbdev->dev,
			"Max JIT allocations limit reached: active allocations %d, max allocations %d\n",
			kctx->jit_current_allocations,
			kctx->jit_max_allocations);
		return false;
	}

	if (info->max_allocations > 0 &&
			kctx->jit_current_allocations_per_bin[info->bin_id] >=
			info->max_allocations) {
		/* Too many current allocations in this bin */
		dev_dbg(kctx->kbdev->dev,
			"Per bin limit of max JIT allocations reached: bin_id %d, active allocations %d, max allocations %d\n",
			info->bin_id,
			kctx->jit_current_allocations_per_bin[info->bin_id],
			info->max_allocations);
		return false;
	}

	return true;
}

static struct kbase_va_region *
find_reasonable_region(const struct base_jit_alloc_info *info,
		       struct list_head *pool_head, bool ignore_usage_id)
{
	struct kbase_va_region *closest_reg = NULL;
	struct kbase_va_region *walker;
	size_t current_diff = SIZE_MAX;

	list_for_each_entry(walker, pool_head, jit_node) {
		if ((ignore_usage_id ||
		     walker->jit_usage_id == info->usage_id) &&
		    walker->jit_bin_id == info->bin_id &&
		    meet_size_and_tiler_align_top_requirements(walker, info)) {
			size_t min_size, max_size, diff;

			/*
			 * The JIT allocations VA requirements have been met,
			 * it's suitable but other allocations might be a
			 * better fit.
			 */
			min_size = min_t(size_t, walker->gpu_alloc->nents,
					 info->commit_pages);
			max_size = max_t(size_t, walker->gpu_alloc->nents,
					 info->commit_pages);
			diff = max_size - min_size;

			if (current_diff > diff) {
				current_diff = diff;
				closest_reg = walker;
			}

			/* The allocation is an exact match */
			if (current_diff == 0)
				break;
		}
	}

	return closest_reg;
}

struct kbase_va_region *kbase_jit_allocate(struct kbase_context *kctx,
		const struct base_jit_alloc_info *info,
		bool ignore_pressure_limit)
{
	struct kbase_va_region *reg = NULL;
	struct kbase_sub_alloc *prealloc_sas[2] = { NULL, NULL };
	int i;

	lockdep_assert_held(&kctx->jctx.lock);

	if (!jit_allow_allocate(kctx, info, ignore_pressure_limit))
		return NULL;

#ifdef CONFIG_MALI_2MB_ALLOC
	/* Preallocate memory for the sub-allocation structs */
	for (i = 0; i != ARRAY_SIZE(prealloc_sas); ++i) {
		prealloc_sas[i] = kmalloc(sizeof(*prealloc_sas[i]), GFP_KERNEL);
		if (!prealloc_sas[i])
			goto end;
	}
#endif

	kbase_gpu_vm_lock(kctx);
	mutex_lock(&kctx->jit_evict_lock);

	/*
	 * Scan the pool for an existing allocation which meets our
	 * requirements and remove it.
	 */
	if (info->usage_id != 0)
		/* First scan for an allocation with the same usage ID */
		reg = find_reasonable_region(info, &kctx->jit_pool_head, false);

	if (!reg)
		/* No allocation with the same usage ID, or usage IDs not in
		 * use. Search for an allocation we can reuse.
		 */
		reg = find_reasonable_region(info, &kctx->jit_pool_head, true);

	if (reg) {
#if MALI_JIT_PRESSURE_LIMIT_BASE
		size_t needed_pages = 0;
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */
		int ret;

		/*
		 * Remove the found region from the pool and add it to the
		 * active list.
		 */
		list_move(&reg->jit_node, &kctx->jit_active_head);

		WARN_ON(reg->gpu_alloc->evicted);

		/*
		 * Remove the allocation from the eviction list as it's no
		 * longer eligible for eviction. This must be done before
		 * dropping the jit_evict_lock
		 */
		list_del_init(&reg->gpu_alloc->evict_node);

#if MALI_JIT_PRESSURE_LIMIT_BASE
		if (!ignore_pressure_limit) {
			if (info->commit_pages > reg->gpu_alloc->nents)
				needed_pages = info->commit_pages -
					       reg->gpu_alloc->nents;

			/* Update early the recycled JIT region's estimate of
			 * used_pages to ensure it doesn't get trimmed
			 * undesirably. This is needed as the recycled JIT
			 * region has been added to the active list but the
			 * number of used pages for it would be zero, so it
			 * could get trimmed instead of other allocations only
			 * to be regrown later resulting in a breach of the JIT
			 * physical pressure limit.
			 * Also that trimming would disturb the accounting of
			 * physical pages, i.e. the VM stats, as the number of
			 * backing pages would have changed when the call to
			 * kbase_mem_evictable_unmark_reclaim is made.
			 *
			 * The second call to update pressure at the end of
			 * this function would effectively be a nop.
			 */
			kbase_jit_report_update_pressure(
				kctx, reg, info->va_pages,
				KBASE_JIT_REPORT_ON_ALLOC_OR_FREE);

			kbase_jit_request_phys_increase_locked(kctx,
							       needed_pages);
		}
#endif
		mutex_unlock(&kctx->jit_evict_lock);

		/* kbase_jit_grow() can release & reacquire 'kctx->reg_lock',
		 * so any state protected by that lock might need to be
		 * re-evaluated if more code is added here in future.
		 */
		ret = kbase_jit_grow(kctx, info, reg, prealloc_sas);

#if MALI_JIT_PRESSURE_LIMIT_BASE
		if (!ignore_pressure_limit)
			kbase_jit_done_phys_increase(kctx, needed_pages);
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */

		kbase_gpu_vm_unlock(kctx);

		if (ret < 0) {
			/*
			 * An update to an allocation from the pool failed,
			 * chances are slim a new allocation would fair any
			 * better so return the allocation to the pool and
			 * return the function with failure.
			 */
			dev_dbg(kctx->kbdev->dev,
				"JIT allocation resize failed: va_pages 0x%llx, commit_pages 0x%llx\n",
				info->va_pages, info->commit_pages);
#if MALI_JIT_PRESSURE_LIMIT_BASE
			/* Undo the early change made to the recycled JIT
			 * region's estimate of used_pages.
			 */
			if (!ignore_pressure_limit) {
				kbase_jit_report_update_pressure(
					kctx, reg, 0,
					KBASE_JIT_REPORT_ON_ALLOC_OR_FREE);
			}
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */
			mutex_lock(&kctx->jit_evict_lock);
			list_move(&reg->jit_node, &kctx->jit_pool_head);
			mutex_unlock(&kctx->jit_evict_lock);
			reg = NULL;
			goto end;
		}
	} else {
		/* No suitable JIT allocation was found so create a new one */
		u64 flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_GPU_RD |
				BASE_MEM_PROT_GPU_WR | BASE_MEM_GROW_ON_GPF |
				BASE_MEM_COHERENT_LOCAL |
				BASEP_MEM_NO_USER_FREE;
		u64 gpu_addr;

		if (info->flags & BASE_JIT_ALLOC_MEM_TILER_ALIGN_TOP)
			flags |= BASE_MEM_TILER_ALIGN_TOP;

		flags |= base_mem_group_id_set(kctx->jit_group_id);
#if MALI_JIT_PRESSURE_LIMIT_BASE
		if (!ignore_pressure_limit) {
			flags |= BASEP_MEM_PERFORM_JIT_TRIM;
			/* The corresponding call to 'done_phys_increase' would
			 * be made inside the kbase_mem_alloc().
			 */
			kbase_jit_request_phys_increase_locked(
				kctx, info->commit_pages);
		}
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */

		mutex_unlock(&kctx->jit_evict_lock);
		kbase_gpu_vm_unlock(kctx);

		reg = kbase_mem_alloc(kctx, info->va_pages, info->commit_pages,
				info->extent, &flags, &gpu_addr);
		if (!reg) {
			/* Most likely not enough GPU virtual space left for
			 * the new JIT allocation.
			 */
			dev_dbg(kctx->kbdev->dev,
				"Failed to allocate JIT memory: va_pages 0x%llx, commit_pages 0x%llx\n",
				info->va_pages, info->commit_pages);
			goto end;
		}

		if (!ignore_pressure_limit) {
			/* Due to enforcing of pressure limit, kbase_mem_alloc
			 * was instructed to perform the trimming which in turn
			 * would have ensured that the new JIT allocation is
			 * already in the jit_active_head list, so nothing to
			 * do here.
			 */
			WARN_ON(list_empty(&reg->jit_node));
		} else {
			mutex_lock(&kctx->jit_evict_lock);
			list_add(&reg->jit_node, &kctx->jit_active_head);
			mutex_unlock(&kctx->jit_evict_lock);
		}
	}

	trace_mali_jit_alloc(reg, info->id);

	kctx->jit_current_allocations++;
	kctx->jit_current_allocations_per_bin[info->bin_id]++;

	trace_jit_stats(kctx, info->bin_id, info->max_allocations);

	reg->jit_usage_id = info->usage_id;
	reg->jit_bin_id = info->bin_id;
	reg->flags |= KBASE_REG_ACTIVE_JIT_ALLOC;
#if MALI_JIT_PRESSURE_LIMIT_BASE
	if (info->flags & BASE_JIT_ALLOC_HEAP_INFO_IS_SIZE)
		reg->flags = reg->flags | KBASE_REG_HEAP_INFO_IS_SIZE;
	reg->heap_info_gpu_addr = info->heap_info_gpu_addr;
	kbase_jit_report_update_pressure(kctx, reg, info->va_pages,
			KBASE_JIT_REPORT_ON_ALLOC_OR_FREE);
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */

end:
	for (i = 0; i != ARRAY_SIZE(prealloc_sas); ++i)
		kfree(prealloc_sas[i]);

	return reg;
}

void kbase_jit_free(struct kbase_context *kctx, struct kbase_va_region *reg)
{
	u64 old_pages;

	/* JIT id not immediately available here, so use 0u */
	trace_mali_jit_free(reg, 0u);

	/* Get current size of JIT region */
	old_pages = kbase_reg_current_backed_size(reg);
	if (reg->initial_commit < old_pages) {
		/* Free trim_level % of region, but don't go below initial
		 * commit size
		 */
		u64 new_size = MAX(reg->initial_commit,
			div_u64(old_pages * (100 - kctx->trim_level), 100));
		u64 delta = old_pages - new_size;

		if (delta)
			kbase_mem_shrink(kctx, reg, old_pages - delta);
	}

#if MALI_JIT_PRESSURE_LIMIT_BASE
	reg->heap_info_gpu_addr = 0;
	kbase_jit_report_update_pressure(kctx, reg, 0,
			KBASE_JIT_REPORT_ON_ALLOC_OR_FREE);
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */

	kctx->jit_current_allocations--;
	kctx->jit_current_allocations_per_bin[reg->jit_bin_id]--;

	trace_jit_stats(kctx, reg->jit_bin_id, UINT_MAX);

	kbase_mem_evictable_mark_reclaim(reg->gpu_alloc);

	kbase_gpu_vm_lock(kctx);
	reg->flags |= KBASE_REG_DONT_NEED;
	reg->flags &= ~KBASE_REG_ACTIVE_JIT_ALLOC;
	kbase_mem_shrink_cpu_mapping(kctx, reg, 0, reg->gpu_alloc->nents);
	kbase_gpu_vm_unlock(kctx);

	/*
	 * Add the allocation to the eviction list and the jit pool, after this
	 * point the shrink can reclaim it, or it may be reused.
	 */
	mutex_lock(&kctx->jit_evict_lock);

	/* This allocation can't already be on a list. */
	WARN_ON(!list_empty(&reg->gpu_alloc->evict_node));
	list_add(&reg->gpu_alloc->evict_node, &kctx->evict_list);

	list_move(&reg->jit_node, &kctx->jit_pool_head);

	mutex_unlock(&kctx->jit_evict_lock);
}

void kbase_jit_backing_lost(struct kbase_va_region *reg)
{
	struct kbase_context *kctx = kbase_reg_flags_to_kctx(reg);

	if (WARN_ON(!kctx))
		return;

	lockdep_assert_held(&kctx->jit_evict_lock);

	/*
	 * JIT allocations will always be on a list, if the region
	 * is not on a list then it's not a JIT allocation.
	 */
	if (list_empty(&reg->jit_node))
		return;

	/*
	 * Freeing the allocation requires locks we might not be able
	 * to take now, so move the allocation to the free list and kick
	 * the worker which will do the freeing.
	 */
	list_move(&reg->jit_node, &kctx->jit_destroy_head);

	schedule_work(&kctx->jit_work);
}

bool kbase_jit_evict(struct kbase_context *kctx)
{
	struct kbase_va_region *reg = NULL;

	lockdep_assert_held(&kctx->reg_lock);

	/* Free the oldest allocation from the pool */
	mutex_lock(&kctx->jit_evict_lock);
	if (!list_empty(&kctx->jit_pool_head)) {
		reg = list_entry(kctx->jit_pool_head.prev,
				struct kbase_va_region, jit_node);
		list_del(&reg->jit_node);
		list_del_init(&reg->gpu_alloc->evict_node);
	}
	mutex_unlock(&kctx->jit_evict_lock);

	if (reg) {
		reg->flags &= ~KBASE_REG_NO_USER_FREE;
		kbase_mem_free_region(kctx, reg);
	}

	return (reg != NULL);
}

void kbase_jit_term(struct kbase_context *kctx)
{
	struct kbase_va_region *walker;

	/* Free all allocations for this context */

	kbase_gpu_vm_lock(kctx);
	mutex_lock(&kctx->jit_evict_lock);
	/* Free all allocations from the pool */
	while (!list_empty(&kctx->jit_pool_head)) {
		walker = list_first_entry(&kctx->jit_pool_head,
				struct kbase_va_region, jit_node);
		list_del(&walker->jit_node);
		list_del_init(&walker->gpu_alloc->evict_node);
		mutex_unlock(&kctx->jit_evict_lock);
		walker->flags &= ~KBASE_REG_NO_USER_FREE;
		kbase_mem_free_region(kctx, walker);
		mutex_lock(&kctx->jit_evict_lock);
	}

	/* Free all allocations from active list */
	while (!list_empty(&kctx->jit_active_head)) {
		walker = list_first_entry(&kctx->jit_active_head,
				struct kbase_va_region, jit_node);
		list_del(&walker->jit_node);
		list_del_init(&walker->gpu_alloc->evict_node);
		mutex_unlock(&kctx->jit_evict_lock);
		walker->flags &= ~KBASE_REG_NO_USER_FREE;
		kbase_mem_free_region(kctx, walker);
		mutex_lock(&kctx->jit_evict_lock);
	}
#if MALI_JIT_PRESSURE_LIMIT_BASE
	WARN_ON(kctx->jit_phys_pages_to_be_allocated);
#endif
	mutex_unlock(&kctx->jit_evict_lock);
	kbase_gpu_vm_unlock(kctx);

	/*
	 * Flush the freeing of allocations whose backing has been freed
	 * (i.e. everything in jit_destroy_head).
	 */
	cancel_work_sync(&kctx->jit_work);
}

#if MALI_JIT_PRESSURE_LIMIT_BASE
void kbase_trace_jit_report_gpu_mem_trace_enabled(struct kbase_context *kctx,
		struct kbase_va_region *reg, unsigned int flags)
{
	/* Offset to the location used for a JIT report within the GPU memory
	 *
	 * This constants only used for this debugging function - not useful
	 * anywhere else in kbase
	 */
	const u64 jit_report_gpu_mem_offset = sizeof(u64)*2;

	u64 addr_start;
	struct kbase_vmap_struct mapping;
	u64 *ptr;

	if (reg->heap_info_gpu_addr == 0ull)
		goto out;

	/* Nothing else to trace in the case the memory just contains the
	 * size. Other tracepoints already record the relevant area of memory.
	 */
	if (reg->flags & KBASE_REG_HEAP_INFO_IS_SIZE)
		goto out;

	addr_start = reg->heap_info_gpu_addr - jit_report_gpu_mem_offset;

	ptr = kbase_vmap(kctx, addr_start, KBASE_JIT_REPORT_GPU_MEM_SIZE,
			&mapping);
	if (!ptr) {
		dev_warn(kctx->kbdev->dev,
				"%s: JIT start=0x%llx unable to map memory near end pointer %llx\n",
				__func__, reg->start_pfn << PAGE_SHIFT,
				addr_start);
		goto out;
	}

	trace_mali_jit_report_gpu_mem(addr_start, reg->start_pfn << PAGE_SHIFT,
				ptr, flags);

	kbase_vunmap(kctx, &mapping);
out:
	return;
}
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */

#if MALI_JIT_PRESSURE_LIMIT_BASE
void kbase_jit_report_update_pressure(struct kbase_context *kctx,
		struct kbase_va_region *reg, u64 new_used_pages,
		unsigned int flags)
{
	u64 diff;

	lockdep_assert_held(&kctx->jctx.lock);

	trace_mali_jit_report_pressure(reg, new_used_pages,
		kctx->jit_current_phys_pressure + new_used_pages -
			reg->used_pages,
		flags);

	if (WARN_ON(new_used_pages > reg->nr_pages))
		return;

	if (reg->used_pages > new_used_pages) {
		/* We reduced the number of used pages */
		diff = reg->used_pages - new_used_pages;

		if (!WARN_ON(diff > kctx->jit_current_phys_pressure))
			kctx->jit_current_phys_pressure -= diff;

		reg->used_pages = new_used_pages;
	} else {
		/* We increased the number of used pages */
		diff = new_used_pages - reg->used_pages;

		if (!WARN_ON(diff > U64_MAX - kctx->jit_current_phys_pressure))
			kctx->jit_current_phys_pressure += diff;

		reg->used_pages = new_used_pages;
	}

}
#endif /* MALI_JIT_PRESSURE_LIMIT_BASE */

int kbase_jd_user_buf_pin_pages(struct kbase_context *kctx,
		struct kbase_va_region *reg)
{
	struct kbase_mem_phy_alloc *alloc = reg->gpu_alloc;
	struct page **pages = alloc->imported.user_buf.pages;
	unsigned long address = alloc->imported.user_buf.address;
	struct mm_struct *mm = alloc->imported.user_buf.mm;
	long pinned_pages;
	long i;

	if (WARN_ON(alloc->type != KBASE_MEM_TYPE_IMPORTED_USER_BUF))
		return -EINVAL;

	if (alloc->nents) {
		if (WARN_ON(alloc->nents != alloc->imported.user_buf.nr_pages))
			return -EINVAL;
		else
			return 0;
	}

	if (WARN_ON(reg->gpu_alloc->imported.user_buf.mm != current->mm))
		return -EINVAL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)
	pinned_pages = get_user_pages(NULL, mm,
			address,
			alloc->imported.user_buf.nr_pages,
#if KERNEL_VERSION(4, 4, 168) <= LINUX_VERSION_CODE && \
KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE
			reg->flags & KBASE_REG_GPU_WR ? FOLL_WRITE : 0,
			pages, NULL);
#else
			reg->flags & KBASE_REG_GPU_WR,
			0, pages, NULL);
#endif
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
	pinned_pages = get_user_pages_remote(NULL, mm,
			address,
			alloc->imported.user_buf.nr_pages,
			reg->flags & KBASE_REG_GPU_WR,
			0, pages, NULL);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
	pinned_pages = get_user_pages_remote(NULL, mm,
			address,
			alloc->imported.user_buf.nr_pages,
			reg->flags & KBASE_REG_GPU_WR ? FOLL_WRITE : 0,
			pages, NULL);
#else
	pinned_pages = get_user_pages_remote(NULL, mm,
			address,
			alloc->imported.user_buf.nr_pages,
			reg->flags & KBASE_REG_GPU_WR ? FOLL_WRITE : 0,
			pages, NULL, NULL);
#endif

	if (pinned_pages <= 0)
		return pinned_pages;

	if (pinned_pages != alloc->imported.user_buf.nr_pages) {
		for (i = 0; i < pinned_pages; i++)
			put_page(pages[i]);
		return -ENOMEM;
	}

	alloc->nents = pinned_pages;

	return 0;
}

static int kbase_jd_user_buf_map(struct kbase_context *kctx,
		struct kbase_va_region *reg)
{
	long pinned_pages;
	struct kbase_mem_phy_alloc *alloc;
	struct page **pages;
	struct tagged_addr *pa;
	long i;
	unsigned long address;
	struct device *dev;
	unsigned long offset;
	unsigned long local_size;
	unsigned long gwt_mask = ~0;
	int err = kbase_jd_user_buf_pin_pages(kctx, reg);

	if (err)
		return err;

	alloc = reg->gpu_alloc;
	pa = kbase_get_gpu_phy_pages(reg);
	address = alloc->imported.user_buf.address;
	pinned_pages = alloc->nents;
	pages = alloc->imported.user_buf.pages;
	dev = kctx->kbdev->dev;
	offset = address & ~PAGE_MASK;
	local_size = alloc->imported.user_buf.size;

	for (i = 0; i < pinned_pages; i++) {
		dma_addr_t dma_addr;
		unsigned long min;

		min = MIN(PAGE_SIZE - offset, local_size);
		dma_addr = dma_map_page(dev, pages[i],
				offset, min,
				DMA_BIDIRECTIONAL);
		if (dma_mapping_error(dev, dma_addr))
			goto unwind;

		alloc->imported.user_buf.dma_addrs[i] = dma_addr;
		pa[i] = as_tagged(page_to_phys(pages[i]));

		local_size -= min;
		offset = 0;
	}

#ifdef CONFIG_MALI_CINSTR_GWT
	if (kctx->gwt_enabled)
		gwt_mask = ~KBASE_REG_GPU_WR;
#endif

	err = kbase_mmu_insert_pages(kctx->kbdev, &kctx->mmu, reg->start_pfn,
			pa, kbase_reg_current_backed_size(reg),
			reg->flags & gwt_mask, kctx->as_nr,
			alloc->group_id);
	if (err == 0)
		return 0;

	/* fall down */
unwind:
	alloc->nents = 0;
	while (i--) {
		dma_unmap_page(kctx->kbdev->dev,
				alloc->imported.user_buf.dma_addrs[i],
				PAGE_SIZE, DMA_BIDIRECTIONAL);
	}

	while (++i < pinned_pages) {
		put_page(pages[i]);
		pages[i] = NULL;
	}

	return err;
}

/* This function would also perform the work of unpinning pages on Job Manager
 * GPUs, which implies that a call to kbase_jd_user_buf_pin_pages() will NOT
 * have a corresponding call to kbase_jd_user_buf_unpin_pages().
 */
static void kbase_jd_user_buf_unmap(struct kbase_context *kctx,
		struct kbase_mem_phy_alloc *alloc, bool writeable)
{
	long i;
	struct page **pages;
	unsigned long size = alloc->imported.user_buf.size;

	KBASE_DEBUG_ASSERT(alloc->type == KBASE_MEM_TYPE_IMPORTED_USER_BUF);
	pages = alloc->imported.user_buf.pages;
	for (i = 0; i < alloc->imported.user_buf.nr_pages; i++) {
		unsigned long local_size;
		dma_addr_t dma_addr = alloc->imported.user_buf.dma_addrs[i];

		local_size = MIN(size, PAGE_SIZE - (dma_addr & ~PAGE_MASK));
		dma_unmap_page(kctx->kbdev->dev, dma_addr, local_size,
				DMA_BIDIRECTIONAL);
		if (writeable)
			set_page_dirty_lock(pages[i]);
		put_page(pages[i]);
		pages[i] = NULL;

		size -= local_size;
	}
	alloc->nents = 0;
}

int kbase_mem_copy_to_pinned_user_pages(struct page **dest_pages,
		void *src_page, size_t *to_copy, unsigned int nr_pages,
		unsigned int *target_page_nr, size_t offset)
{
	void *target_page = kmap(dest_pages[*target_page_nr]);
	size_t chunk = PAGE_SIZE-offset;

	if (!target_page) {
		pr_err("%s: kmap failure", __func__);
		return -ENOMEM;
	}

	chunk = min(chunk, *to_copy);

	memcpy(target_page + offset, src_page, chunk);
	*to_copy -= chunk;

	kunmap(dest_pages[*target_page_nr]);

	*target_page_nr += 1;
	if (*target_page_nr >= nr_pages || *to_copy == 0)
		return 0;

	target_page = kmap(dest_pages[*target_page_nr]);
	if (!target_page) {
		pr_err("%s: kmap failure", __func__);
		return -ENOMEM;
	}

	KBASE_DEBUG_ASSERT(target_page);

	chunk = min(offset, *to_copy);
	memcpy(target_page, src_page + PAGE_SIZE-offset, chunk);
	*to_copy -= chunk;

	kunmap(dest_pages[*target_page_nr]);

	return 0;
}

struct kbase_mem_phy_alloc *kbase_map_external_resource(
		struct kbase_context *kctx, struct kbase_va_region *reg,
		struct mm_struct *locked_mm)
{
	int err;

	lockdep_assert_held(&kctx->reg_lock);

	/* decide what needs to happen for this resource */
	switch (reg->gpu_alloc->type) {
	case KBASE_MEM_TYPE_IMPORTED_USER_BUF: {
		if ((reg->gpu_alloc->imported.user_buf.mm != locked_mm) &&
		    (!reg->gpu_alloc->nents))
			goto exit;

		reg->gpu_alloc->imported.user_buf.current_mapping_usage_count++;
		if (1 == reg->gpu_alloc->imported.user_buf.current_mapping_usage_count) {
			err = kbase_jd_user_buf_map(kctx, reg);
			if (err) {
				reg->gpu_alloc->imported.user_buf.current_mapping_usage_count--;
				goto exit;
			}
		}
	}
	break;
	case KBASE_MEM_TYPE_IMPORTED_UMM: {
		err = kbase_mem_umm_map(kctx, reg);
		if (err)
			goto exit;
		break;
	}
	default:
		goto exit;
	}

	return kbase_mem_phy_alloc_get(reg->gpu_alloc);
exit:
	return NULL;
}

void kbase_unmap_external_resource(struct kbase_context *kctx,
		struct kbase_va_region *reg, struct kbase_mem_phy_alloc *all