eada_lock);

	if (!re)
		return -1;

	spin_lock(&re->lock);
	/*
	 * just take the full list from the extent. afterwards we
	 * don't need the lock anymore
	 */
	list_replace_init(&re->extctl, &list);
	for_dev = re->scheduled_for;
	re->scheduled_for = NULL;
	spin_unlock(&re->lock);

	if (err == 0) {
		nritems = level ? btrfs_header_nritems(eb) : 0;
		generation = btrfs_header_generation(eb);
		/*
		 * FIXME: currently we just set nritems to 0 if this is a leaf,
		 * effectively ignoring the content. In a next step we could
		 * trigger more readahead depending from the content, e.g.
		 * fetch the checksums for the extents in the leaf.
		 */
	} else {
		/*
		 * this is the error case, the extent buffer has not been
		 * read correctly. We won't access anything from it and
		 * just cleanup our data structures. Effectively this will
		 * cut the branch below this node from read ahead.
		 */
		nritems = 0;
		generation = 0;
	}

	for (i = 0; i < nritems; i++) {
		struct reada_extctl *rec;
		u64 n_gen;
		struct btrfs_key key;
		struct btrfs_key next_key;

		btrfs_node_key_to_cpu(eb, &key, i);
		if (i + 1 < nritems)
			btrfs_node_key_to_cpu(eb, &next_key, i + 1);
		else
			next_key = re->top;
		bytenr = btrfs_node_blockptr(eb, i);
		n_gen = btrfs_node_ptr_generation(eb, i);

		list_for_each_entry(rec, &list, list) {
			struct reada_control *rc = rec->rc;

			/*
			 * if the generation doesn't match, just ignore this
			 * extctl. This will probably cut off a branch from
			 * prefetch. Alternatively one could start a new (sub-)
			 * prefetch for this branch, starting again from root.
			 * FIXME: move the generation check out of this loop
			 */
#ifdef DEBUG
			if (rec->generation != generation) {
				btrfs_debug(root->fs_info,
					   "generation mismatch for (%llu,%d,%llu) %llu != %llu",
				       key.objectid, key.type, key.offset,
				       rec->generation, generation);
			}
#endif
			if (rec->generation == generation &&
			    btrfs_comp_cpu_keys(&key, &rc->key_end) < 0 &&
			    btrfs_comp_cpu_keys(&next_key, &rc->key_start) > 0)
				reada_add_block(rc, bytenr, &next_key,
						level - 1, n_gen);
		}
	}
	/*
	 * free extctl records
	 */
	while (!list_empty(&list)) {
		struct reada_control *rc;
		struct reada_extctl *rec;

		rec = list_first_entry(&list, struct reada_extctl, list);
		list_del(&rec->list);
		rc = rec->rc;
		kfree(rec);

		kref_get(&rc->refcnt);
		if (atomic_dec_and_test(&rc->elems)) {
			kref_put(&rc->refcnt, reada_control_release);
			wake_up(&rc->wait);
		}
		kref_put(&rc->refcnt, reada_control_release);

		reada_extent_put(fs_info, re);	/* one ref for each entry */
	}
	reada_extent_put(fs_info, re);	/* our ref */
	if (for_dev)
		atomic_dec(&for_dev->reada_in_flight);

	return 0;
}

/*
 * start is passed separately in case eb in NULL, which may be the case with
 * failed I/O
 */
int btree_readahead_hook(struct btrfs_root *root, struct extent_buffer *eb,
			 u64 start, int err)
{
	int ret;

	ret = __readahead_hook(root, eb, start, err);

	reada_start_machine(root->fs_info);

	return ret;
}

static struct reada_zone *reada_find_zone(struct btrfs_fs_info *fs_info,
					  struct btrfs_device *dev, u64 logical,
					  struct btrfs_bio *bbio)
{
	int ret;
	struct reada_zone *zone;
	struct btrfs_block_group_cache *cache = NULL;
	u64 start;
	u64 end;
	int i;

	zone = NULL;
	spin_lock(&fs_info->reada_lock);
	ret = radix_tree_gang_lookup(&dev->reada_zones, (void **)&zone,
				     logical >> PAGE_CACHE_SHIFT, 1);
	if (ret == 1)
		kref_get(&zone->refcnt);
	spin_unlock(&fs_info->reada_lock);

	if (ret == 1) {
		if (logical >= zone->start && logical < zone->end)
			return zone;
		spin_lock(&fs_info->reada_lock);
		kref_put(&zone->refcnt, reada_zone_release);
		spin_unlock(&fs_info->reada_lock);
	}

	cache = btrfs_lookup_block_group(fs_info, logical);
	if (!cache)
		return NULL;

	start = cache->key.objectid;
	end = start + cache->key.offset - 1;
	btrfs_put_block_group(cache);

	zone = kzalloc(sizeof(*zone), GFP_NOFS);
	if (!zone)
		return NULL;

	zone->start = start;
	zone->end = end;
	INIT_LIST_HEAD(&zone->list);
	spin_lock_init(&zone->lock);
	zone->locked = 0;
	kref_init(&zone->refcnt);
	zone->elems = 0;
	zone->device = dev; /* our device always sits at index 0 */
	for (i = 0; i < bbio->num_stripes; ++i) {
		/* bounds have already been checked */
		zone->devs[i] = bbio->stripes[i].dev;
	}
	zone->ndevs = bbio->num_stripes;

	spin_lock(&fs_info->reada_lock);
	ret = radix_tree_insert(&dev->reada_zones,
				(unsigned long)(zone->end >> PAGE_CACHE_SHIFT),
				zone);

	if (ret == -EEXIST) {
		kfree(zone);
		ret = radix_tree_gang_lookup(&dev->reada_zones, (void **)&zone,
					     logical >> PAGE_CACHE_SHIFT, 1);
		if (ret == 1)
			kref_get(&zone->refcnt);
	}
	spin_unlock(&fs_info->reada_lock);

	return zone;
}

static struct reada_extent *reada_find_extent(struct btrfs_root *root,
					      u64 logical,
					      struct btrfs_key *top, int level)
{
	int ret;
	struct reada_extent *re = NULL;
	struct reada_extent *re_exist = NULL;
	struct btrfs_fs_info *fs_info = root->fs_info;
	struct btrfs_bio *bbio = NULL;
	struct btrfs_device *dev;
	struct btrfs_device *prev_dev;
	u32 blocksize;
	u64 length;
	int real_stripes;
	int nzones = 0;
	int i;
	unsigned long index = logical >> PAGE_CACHE_SHIFT;
	int dev_replace_is_ongoing;

	spin_lock(&fs_info->reada_lock);
	re = radix_tree_lookup(&fs_info->reada_tree, index);
	if (re)
		re->refcnt++;
	spin_unlock(&fs_info->reada_lock);

	if (re)
		return re;

	re = kzalloc(sizeof(*re), GFP_NOFS);
	if (!re)
		return NULL;

	blocksize = root->nodesize;
	re->logical = logical;
	re->top = *top;
	INIT_LIST_HEAD(&re->extctl);
	spin_lock_init(&re->lock);
	re->refcnt = 1;

	/*
	 * map block
	 */
	length = blocksize;
	ret = btrfs_map_block(fs_info, REQ_GET_READ_MIRRORS, logical, &length,
			      &bbio, 0);
	if (ret || !bbio || length < blocksize)
		goto error;

	if (bbio->num_stripes > BTRFS_MAX_MIRRORS) {
		btrfs_err(root->fs_info,
			   "readahead: more than %d copies not supported",
			   BTRFS_MAX_MIRRORS);
		goto error;
	}

	real_stripes = bbio->num_stripes - bbio->num_tgtdevs;
	for (nzones = 0; nzones < real_stripes; ++nzones) {
		struct reada_zone *zone;

		dev = bbio->stripes[nzones].dev;
		zone = reada_find_zone(fs_info, dev, logical, bbio);
		if (!zone)
			break;

		re->zones[nzones] = zone;
		spin_lock(&zone->lock);
		if (!zone->elems)
			kref_get(&zone->refcnt);
		++zone->elems;
		spin_unlock(&zone->lock);
		spin_lock(&fs_info->reada_lock);
		kref_put(&zone->refcnt, reada_zone_release);
		spin_unlock(&fs_info->reada_lock);
	}
	re->nzones = nzones;
	if (nzones == 0) {
		/* not a single zone found, error and out */
		goto error;
	}

	/* insert extent in reada_tree + all per-device trees, all or nothing */
	btrfs_dev_replace_lock(&fs_info->dev_replace);
	spin_lock(&fs_info->reada_lock);
	ret = radix_tree_insert(&fs_info->reada_tree, index, re);
	if (ret == -EEXIST) {
		re_exist = radix_tree_lookup(&fs_info->reada_tree, index);
		BUG_ON(!re_exist);
		re_exist->refcnt++;
		spin_unlock(&fs_info->reada_lock);
		btrfs_dev_replace_unlock(&fs_info->dev_replace);
		goto error;
	}
	if (ret) {
		spin_unlock(&fs_info->reada_lock);
		btrfs_dev_replace_unlock(&fs_info->dev_replace);
		goto error;
	}
	prev_dev = NULL;
	dev_replace_is_ongoing = btrfs_dev_replace_is_ongoing(
			&fs_info->dev_replace);
	for (i = 0; i < nzones; ++i) {
		dev = bbio->stripes[i].dev;
		if (dev == prev_dev) {
			/*
			 * in case of DUP, just add the first zone. As both
			 * are on the same device, there's nothing to gain
			 * from adding both.
			 * Also, it wouldn't work, as the tree is per device
			 * and adding would fail with EEXIST
			 */
			continue;
		}
		if (!dev->bdev) {
			/*
			 * cannot read ahead on missing device, but for RAID5/6,
			 * REQ_GET_READ_MIRRORS return 1. So don't skip missing
			 * device for such case.
			 */
			if (nzones > 1)
				continue;
		}
		if (dev_replace_is_ongoing &&
		    dev == fs_info->dev_replace.tgtdev) {
			/*
			 * as this device is selected for reading only as
			 * a last resort, skip it for read ahead.
			 */
			continue;
		}
		prev_dev = dev;
		ret = radix_tree_insert(&dev->reada_extents, index, re);
		if (ret) {
			while (--i >= 0) {
				dev = bbio->stripes[i].dev;
				BUG_ON(dev == NULL);
				/* ignore whether the entry was inserted */
				radix_tree_delete(&dev->reada_extents, index);
			}
			BUG_ON(fs_info == NULL);
			radix_tree_delete(&fs_info->reada_tree, index);
			spin_unlock(&fs_info->reada_lock);
			btrfs_dev_replace_unlock(&fs_info->dev_replace);
			goto error;
		}
	}
	spin_unlock(&fs_info->reada_lock);
	btrfs_dev_replace_unlock(&fs_info->dev_replace);

	btrfs_put_bbio(bbio);
	return re;

error:
	while (nzones) {
		struct reada_zone *zone;

		--nzones;
		zone = re->zones[nzones];
		kref_get(&zone->refcnt);
		spin_lock(&zone->lock);
		--zone->elems;
		if (zone->elems == 0) {
			/*
			 * no fs_info->reada_lock needed, as this can't be
			 * the last ref
			 */
			kref_put(&zone->refcnt, reada_zone_release);
		}
		spin_unlock(&zone->lock);

		spin_lock(&fs_info->reada_lock);
		kref_put(&zone->refcnt, reada_zone_release);
		spin_unlock(&fs_info->reada_lock);
	}
	btrfs_put_bbio(bbio);
	kfree(re);
	return re_exist;
}

static void reada_extent_put(struct btrfs_fs_info *fs_info,
			     struct reada_extent *re)
{
	int i;
	unsigned long index = re->logical >> PAGE_CACHE_SHIFT;

	spin_lock(&fs_info->reada_lock);
	if (--re->refcnt) {
		spin_unlock(&fs_info->reada_lock);
		return;
	}

	radix_tree_delete(&fs_info->reada_tree, index);
	for (i = 0; i < re->nzones; ++i) {
		struct reada_zone *zone = re->zones[i];

		radix_tree_delete(&zone->device->reada_extents, index);
	}

	spin_unlock(&fs_info->reada_lock);

	for (i = 0; i < re->nzones; ++i) {
		struct reada_zone *zone = re->zones[i];

		kref_get(&zone->refcnt);
		spin_lock(&zone->lock);
		--zone->elems;
		if (zone->elems == 0) {
			/* no fs_info->reada_lock needed, as this can't be
			 * the last ref */
			kref_put(&zone->refcnt, reada_zone_release);
		}
		spin_unlock(&zone->lock);

		spin_lock(&fs_info->reada_lock);
		kref_put(&zone->refcnt, reada_zone_release);
		spin_unlock(&fs_info->reada_lock);
	}
	if (re->scheduled_for)
		atomic_dec(&re->scheduled_for->reada_in_flight);

	kfree(re);
}

static void reada_zone_release(struct kref *kref)
{
	struct reada_zone *zone = container_of(kref, struct reada_zone, refcnt);

	radix_tree_delete(&zone->device->reada_zones,
			  zone->end >> PAGE_CACHE_SHIFT);

	kfree(zone);
}

static void reada_control_release(struct kref *kref)
{
	struct reada_control *rc = container_of(kref, struct reada_control,
						refcnt);

	kfree(rc);
}

static int reada_add_block(struct reada_control *rc, u64 logical,
			   struct btrfs_key *top, int level, u64 generation)
{
	struct btrfs_root *root = rc->root;
	struct reada_extent *re;
	struct reada_extctl *rec;

	re = reada_find_extent(root, logical, top, level); /* takes one ref */
	if (!re)
		return -1;

	rec = kzalloc(sizeof(*rec), GFP_NOFS);
	if (!rec) {
		reada_extent_put(root->fs_info, re);
		return -ENOMEM;
	}

	rec->rc = rc;
	rec->generation = generation;
	atomic_inc(&rc->elems);

	spin_lock(&re->lock);
	list_add_tail(&rec->list, &re->extctl);
	spin_unlock(&re->lock);

	/* leave the ref on the extent */

	return 0;
}

/*
 * called with fs_info->reada_lock held
 */
static void reada_peer_zones_set_lock(struct reada_zone *zone, int lock)
{
	int i;
	unsigned long index = zone->end >> PAGE_CACHE_SHIFT;

	for (i = 0; i < zone->ndevs; ++i) {
		struct reada_zone *peer;
		peer = radix_tree_lookup(&zone->devs[i]->reada_zones, index);
		if (peer && peer->device != zone->device)
			peer->locked = lock;
	}
}

/*
 * called with fs_info->reada_lock held
 */
static int reada_pick_zone(struct btrfs_device *dev)
{
	struct reada_zone *top_zone = NULL;
	struct reada_zone *top_locked_zone = NULL;
	u64 top_elems = 0;
	u64 top_locked_elems = 0;
	unsigned long index = 0;
	int ret;

	if (dev->reada_curr_zone) {
		reada_peer_zones_set_lock(dev->reada_curr_zone, 0);
		kref_put(&dev->reada_curr_zone->refcnt, reada_zone_release);
		dev->reada_curr_zone = NULL;
	}
	/* pick the zone with the most elements */
	while (1) {
		struct reada_zone *zone;

		ret = radix_tree_gang_lookup(&dev->reada_zones,
					     (void **)&zone, index, 1);
		if (ret == 0)
			break;
		index = (zone->end >> PAGE_CACHE_SHIFT) + 1;
		if (zone->locked) {
			if (zone->elems > top_locked_elems) {
				top_locked_elems = zone->elems;
				top_locked_zone = zone;
			}
		} else {
			if (zone->elems > top_elems) {
				top_elems = zone->elems;
				top_zone = zone;
			}
		}
	}
	if (top_zone)
		dev->reada_curr_zone = top_zone;
	else if (top_locked_zone)
		dev->reada_curr_zone = top_locked_zone;
	else
		return 0;

	dev->reada_next = dev->reada_curr_zone->start;
	kref_get(&dev->reada_curr_zone->refcnt);
	reada_peer_zones_set_lock(dev->reada_curr_zone, 1);

	return 1;
}

static int reada_start_machine_dev(struct btrfs_fs_info *fs_info,
				   struct btrfs_device *dev)
{
	struct reada_extent *re = NULL;
	int mirror_num = 0;
	struct extent_buffer *eb = NULL;
	u64 logical;
	int ret;
	int i;
	int need_kick = 0;

	spin_lock(&fs_info->reada_lock);
	if (dev->reada_curr_zone == NULL) {
		ret = reada_pick_zone(dev);
		if (!ret) {
			spin_unlock(&fs_info->reada_lock);
			return 0;
		}
	}
	/*
	 * FIXME currently we issue the reads one extent at a time. If we have
	 * a contiguous block of extents, we could also coagulate them or use
	 * plugging to speed things up
	 */
	ret = radix_tree_gang_lookup(&dev->reada_extents, (void **)&re,
				     dev->reada_next >> PAGE_CACHE_SHIFT, 1);
	if (ret == 0 || re->logical >= dev->reada_curr_zone->end) {
		ret = reada_pick_zone(dev);
		if (!ret) {
			spin_unlock(&fs_info->reada_lock);
			return 0;
		}
		re = NULL;
		ret = radix_tree_gang_lookup(&dev->reada_extents, (void **)&re,
					dev->reada_next >> PAGE_CACHE_SHIFT, 1);
	}
	if (ret == 0) {
		spin_unlock(&fs_info->reada_lock);
		return 0;
	}
	dev->reada_next = re->logical + fs_info->tree_root->nodesize;
	re->refcnt++;

	spin_unlock(&fs_info->reada_lock);

	/*
	 * find mirror num
	 */
	for (i = 0; i < re->nzones; ++i) {
		if (re->zones[i]->device == dev) {
			mirror_num = i + 1;
			break;
		}
	}
	logical = re->logical;

	spin_lock(&re->lock);
	if (re->scheduled_for == NULL) {
		re->scheduled_for = dev;
		need_kick = 1;
	}
	spin_unlock(&re->lock);

	reada_extent_put(fs_info, re);

	if (!need_kick)
		return 0;

	atomic_inc(&dev->reada_in_flight);
	ret = reada_tree_block_flagged(fs_info->extent_root, logical,
			mirror_num, &eb);
	if (ret)
		__readahead_hook(fs_info->extent_root, NULL, logical, ret);
	else if (eb)
		__readahead_hook(fs_info->extent_root, eb, eb->start, ret);

	if (eb)
		free_extent_buffer(eb);

	return 1;

}

static void reada_start_machine_worker(struct btrfs_work *work)
{
	struct reada_machine_work *rmw;
	struct btrfs_fs_info *fs_info;
	int old_ioprio;

	rmw = container_of(work, struct reada_machine_work, work);
	fs_info = rmw->fs_info;

	kfree(rmw);

	old_ioprio = IOPRIO_PRIO_VALUE(task_nice_ioclass(current),
				       task_nice_ioprio(current));
	set_task_ioprio(current, BTRFS_IOPRIO_READA);
	__reada_start_machine(fs_info);
	set_task_ioprio(current, old_ioprio);
}

static void __reada_start_machine(struct btrfs_fs_info *fs_info)
{
	struct btrfs_device *device;
	struct btrfs_fs_devices *fs_devices = fs_info->fs_devices;
	u64 enqueued;
	u64 total = 0;
	int i;

again:
	do {
		enqueued = 0;
		mutex_lock(&fs_devices->device_list_mutex);
		list_for_each_entry(device, &fs_devices->devices, dev_list) {
			if (atomic_read(&device->reada_in_flight) <
			    MAX_IN_FLIGHT)
				enqueued += reada_start_machine_dev(fs_info,
								    device);
		}
		mutex_unlock(&fs_devices->device_list_mutex);
		total += enqueued;
	} while (enqueued && total < 10000);
	if (fs_devices->seed) {
		fs_devices = fs_devices->seed;
		goto again;
	}

	if (enqueued == 0)
		return;

	/*
	 * If everything is already in the cache, this is effectively single
	 * threaded. To a) not hold the caller for too long and b) to utilize
	 * more cores, we broke the loop above after 10000 iterations and now
	 * enqueue to workers to finish it. This will distribute the load to
	 * the cores.
	 */
	for (i = 0; i < 2; ++i)
		reada_start_machine(fs_info);
}

static void reada_start_machine(struct btrfs_fs_info *fs_info)
{
	struct reada_machine_work *rmw;

	rmw = kzalloc(sizeof(*rmw), GFP_NOFS);
	if (!rmw) {
		/* FIXME we cannot handle this properly right now */
		BUG();
	}
	btrfs_init_work(&rmw->work, btrfs_readahead_helper,
			reada_start_machine_worker, NULL, NULL);
	rmw->fs_info = fs_info;

	btrfs_queue_work(fs_info->readahead_workers, &rmw->work);
}

#ifdef DEBUG
static void dump_devs(struct btrfs_fs_info *fs_info, int all)
{
	struct btrfs_device *device;
	struct btrfs_fs_devices *fs_devices = fs_info->fs_devices;
	unsigned long index;
	int ret;
	int i;
	int j;
	int cnt;

	spin_lock(&fs_info->reada_lock);
	list_for_each_entry(device, &fs_devices->devices, dev_list) {
		printk(KERN_DEBUG "dev %lld has %d in flight\n", device->devid,
			atomic_read(&device->reada_in_flight));
		index = 0;
		while (1) {
			struct reada_zone *zone;
			ret = radix_tree_gang_lookup(&device->reada_zones,
						     (void **)&zone, index, 1);
			if (ret == 0)
				break;
			printk(KERN_DEBUG "  zone %llu-%llu elems %llu locked "
				"%d devs", zone->start, zone->end, zone->elems,
				zone->locked);
			for (j = 0; j < zone->ndevs; ++j) {
				printk(KERN_CONT " %lld",
					zone->devs[j]->devid);
			}
			if (device->reada_curr_zone == zone)
				printk(KERN_CONT " curr off %llu",
					device->reada_next - zone->start);
			printk(KERN_CONT "\n");
			index = (zone->end >> PAGE_CACHE_SHIFT) + 1;
		}
		cnt = 0;
		index = 0;
		while (all) {
			struct reada_extent *re = NULL;

			ret = radix_tree_gang_lookup(&device->reada_extents,
						     (void **)&re, index, 1);
			if (ret == 0)
				break;
			printk(KERN_DEBUG
				"  re: logical %llu size %u empty %d for %lld",
				re->logical, fs_info->tree_root->nodesize,
				list_empty(&re->extctl), re->scheduled_for ?
				re->scheduled_for->devid : -1);

			for (i = 0; i < re->nzones; ++i) {
				printk(KERN_CONT " zone %llu-%llu devs",
					re->zones[i]->start,
					re->zones[i]->end);
				for (j = 0; j < re->zones[i]->ndevs; ++j) {
					printk(KERN_CONT " %lld",
						re->zones[i]->devs[j]->devid);
				}
			}
			printk(KERN_CONT "\n");
			index = (re->logical >> PAGE_CACHE_SHIFT) + 1;
			if (++cnt > 15)
				break;
		}
	}

	index = 0;
	cnt = 0;
	while (all) {
		struct reada_extent *re = NULL;

		ret = radix_tree_gang_lookup(&fs_info->reada_tree, (void **)&re,
					     index, 1);
		if (ret == 0)
			break;
		if (!re->scheduled_for) {
			index = (re->logical >> PAGE_CACHE_SHIFT) + 1;
			continue;
		}
		printk(KERN_DEBUG
			"re: logical %llu size %u list empty %d for %lld",
			re->logical, fs_info->tree_root->nodesize,
			list_empty(&re->extctl),
			re->scheduled_for ? re->scheduled_for->devid : -1);
		for (i = 0; i < re->nzones; ++i) {
			printk(KERN_CONT " zone %llu-%llu devs",
				re->zones[i]->start,
				re->zones[i]->end);
			for (i = 0; i < re->nzones; ++i) {
				printk(KERN_CONT " zone %llu-%llu devs",
					re->zones[i]->start,
					re->zones[i]->end);
				for (j = 0; j < re->zones[i]->ndevs; ++j) {
					printk(KERN_CONT " %lld",
						re->zones[i]->devs[j]->devid);
				}
			}
		}
		printk(KERN_CONT "\n");
		index = (re->logical >> PAGE_CACHE_SHIFT) + 1;
	}
	spin_unlock(&fs_info->reada_lock);
}
#endif

/*
 * interface
 */
struct reada_control *btrfs_reada_add(struct btrfs_root *root,
			struct btrfs_key *key_start, struct btrfs_key *key_end)
{
	struct reada_control *rc;
	u64 start;
	u64 generation;
	int level;
	int ret;
	struct extent_buffer *node;
	static struct btrfs_key max_key = {
		.objectid = (u64)-1,
		.type = (u8)-1,
		.offset = (u64)-1
	};

	rc = kzalloc(sizeof(*rc), GFP_NOFS);
	if (!rc)
		return ERR_PTR(-ENOMEM);

	rc->root = root;
	rc->key_start = *key_start;
	rc->key_end = *key_end;
	atomic_set(&rc->elems, 0);
	init_waitqueue_head(&rc->wait);
	kref_init(&rc->refcnt);
	kref_get(&rc->refcnt); /* one ref for having elements */

	node = btrfs_root_node(root);
	start = node->start;
	level = btrfs_header_level(node);
	generation = btrfs_header_generation(node);
	free_extent_buffer(node);

	ret = reada_add_block(rc, start, &max_key, level, generation);
	if (ret) {
		kfree(rc);
		return ERR_PTR(ret);
	}

	reada_start_machine(root->fs_info);

	return rc;
}

#ifdef DEBUG
int btrfs_reada_wait(void *handle)
{
	struct reada_control *rc = handle;

	while (atomic_read(&rc->elems)) {
		wait_event_timeout(rc->wait, atomic_read(&rc->elems) == 0,
				   5 * HZ);
		dump_devs(rc->root->fs_info,
			  atomic_read(&rc->elems) < 10 ? 1 : 0);
	}

	dump_devs(rc->root->fs_info, atomic_read(&rc->elems) < 10 ? 1 : 0);

	kref_put(&rc->refcnt, reada_control_release);

	return 0;
}
#else
int btrfs_reada_wait(void *handle)
{
	struct reada_control *rc = handle;

	while (atomic_read(&rc->elems)) {
		wait_event(rc->wait, atomic_read(&rc->elems) == 0);
	}

	kref_put(&rc->refcnt, reada_control_release);

	return 0;
}
#endif

void btrfs_reada_detach(void *handle)
{
	struct reada_control *rc = handle;

	kref_put(&rc->refcnt, reada_control_release);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <linux/err.h>
#include <linux/uuid.h>
#include "ctree.h"
#include "transaction.h"
#include "disk-io.h"
#include "print-tree.h"

/*
 * Read a root item from the tree. In case we detect a root item smaller then
 * sizeof(root_item), we know it's an old version of the root structure and
 * initialize all new fields to zero. The same happens if we detect mismatching
 * generation numbers as then we know the root was once mounted with an older
 * kernel that was not aware of the root item structure change.
 */
static void btrfs_read_root_item(struct extent_buffer *eb, int slot,
				struct btrfs_root_item *item)
{
	uuid_le uuid;
	int len;
	int need_reset = 0;

	len = btrfs_item_size_nr(eb, slot);
	read_extent_buffer(eb, item, btrfs_item_ptr_offset(eb, slot),
			min_t(int, len, (int)sizeof(*item)));
	if (len < sizeof(*item))
		need_reset = 1;
	if (!need_reset && btrfs_root_generation(item)
		!= btrfs_root_generation_v2(item)) {
		if (btrfs_root_generation_v2(item) != 0) {
			btrfs_warn(eb->fs_info,
					"mismatching "
					"generation and generation_v2 "
					"found in root item. This root "
					"was probably mounted with an "
					"older kernel. Resetting all "
					"new fields.");
		}
		need_reset = 1;
	}
	if (need_reset) {
		memset(&item->generation_v2, 0,
			sizeof(*item) - offsetof(struct btrfs_root_item,
					generation_v2));

		uuid_le_gen(&uuid);
		memcpy(item->uuid, uuid.b, BTRFS_UUID_SIZE);
	}
}

/*
 * btrfs_find_root - lookup the root by the key.
 * root: the root of the root tree
 * search_key: the key to search
 * path: the path we search
 * root_item: the root item of the tree we look for
 * root_key: the reak key of the tree we look for
 *
 * If ->offset of 'seach_key' is -1ULL, it means we are not sure the offset
 * of the search key, just lookup the root with the highest offset for a
 * given objectid.
 *
 * If we find something return 0, otherwise > 0, < 0 on error.
 */
int btrfs_find_root(struct btrfs_root *root, struct btrfs_key *search_key,
		    struct btrfs_path *path, struct btrfs_root_item *root_item,
		    struct btrfs_key *root_key)
{
	struct btrfs_key found_key;
	struct extent_buffer *l;
	int ret;
	int slot;

	ret = btrfs_search_slot(NULL, root, search_key, path, 0, 0);
	if (ret < 0)
		return ret;

	if (search_key->offset != -1ULL) {	/* the search key is exact */
		if (ret > 0)
			goto out;
	} else {
		BUG_ON(ret == 0);		/* Logical error */
		if (path->slots[0] == 0)
			goto out;
		path->slots[0]--;
		ret = 0;
	}

	l = path->nodes[0];
	slot = path->slots[0];

	btrfs_item_key_to_cpu(l, &found_key, slot);
	if (found_key.objectid != search_key->objectid ||
	    found_key.type != BTRFS_ROOT_ITEM_KEY) {
		ret = 1;
		goto out;
	}

	if (root_item)
		btrfs_read_root_item(l, slot, root_item);
	if (root_key)
		memcpy(root_key, &found_key, sizeof(found_key));
out:
	btrfs_release_path(path);
	return ret;
}

void btrfs_set_root_node(struct btrfs_root_item *item,
			 struct extent_buffer *node)
{
	btrfs_set_root_bytenr(item, node->start);
	btrfs_set_root_level(item, btrfs_header_level(node));
	btrfs_set_root_generation(item, btrfs_header_generation(node));
}

/*
 * copy the data in 'item' into the btree
 */
int btrfs_update_root(struct btrfs_trans_handle *trans, struct btrfs_root
		      *root, struct btrfs_key *key, struct btrfs_root_item
		      *item)
{
	struct btrfs_path *path;
	struct extent_buffer *l;
	int ret;
	int slot;
	unsigned long ptr;
	u32 old_len;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	ret = btrfs_search_slot(trans, root, key, path, 0, 1);
	if (ret < 0) {
		btrfs_abort_transaction(trans, root, ret);
		goto out;
	}

	if (ret != 0) {
		btrfs_print_leaf(root, path->nodes[0]);
		btrfs_crit(root->fs_info, "unable to update root key %llu %u %llu",
		       key->objectid, key->type, key->offset);
		BUG_ON(1);
	}

	l = path->nodes[0];
	slot = path->slots[0];
	ptr = btrfs_item_ptr_offset(l, slot);
	old_len = btrfs_item_size_nr(l, slot);

	/*
	 * If this is the first time we update the root item which originated
	 * from an older kernel, we need to enlarge the item size to make room
	 * for the added fields.
	 */
	if (old_len < sizeof(*item)) {
		btrfs_release_path(path);
		ret = btrfs_search_slot(trans, root, key, path,
				-1, 1);
		if (ret < 0) {
			btrfs_abort_transaction(trans, root, ret);
			goto out;
		}

		ret = btrfs_del_item(trans, root, path);
		if (ret < 0) {
			btrfs_abort_transaction(trans, root, ret);
			goto out;
		}
		btrfs_release_path(path);
		ret = btrfs_insert_empty_item(trans, root, path,
				key, sizeof(*item));
		if (ret < 0) {
			btrfs_abort_transaction(trans, root, ret);
			goto out;
		}
		l = path->nodes[0];
		slot = path->slots[0];
		ptr = btrfs_item_ptr_offset(l, slot);
	}

	/*
	 * Update generation_v2 so at the next mount we know the new root
	 * fields are valid.
	 */
	btrfs_set_root_generation_v2(item, btrfs_root_generation(item));

	write_extent_buffer(l, item, ptr, sizeof(*item));
	btrfs_mark_buffer_dirty(path->nodes[0]);
out:
	btrfs_free_path(path);
	return ret;
}

int btrfs_insert_root(struct btrfs_trans_handle *trans, struct btrfs_root *root,
		      struct btrfs_key *key, struct btrfs_root_item *item)
{
	/*
	 * Make sure generation v1 and v2 match. See update_root for details.
	 */
	btrfs_set_root_generation_v2(item, btrfs_root_generation(item));
	return btrfs_insert_item(trans, root, key, item, sizeof(*item));
}

int btrfs_find_orphan_roots(struct btrfs_root *tree_root)
{
	struct extent_buffer *leaf;
	struct btrfs_path *path;
	struct btrfs_key key;
	struct btrfs_key root_key;
	struct btrfs_root *root;
	int err = 0;
	int ret;
	bool can_recover = true;

	if (tree_root->fs_info->sb->s_flags & MS_RDONLY)
		can_recover = false;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	key.objectid = BTRFS_ORPHAN_OBJECTID;
	key.type = BTRFS_ORPHAN_ITEM_KEY;
	key.offset = 0;

	root_key.type = BTRFS_ROOT_ITEM_KEY;
	root_key.offset = (u64)-1;

	while (1) {
		ret = btrfs_search_slot(NULL, tree_root, &key, path, 0, 0);
		if (ret < 0) {
			err = ret;
			break;
		}

		leaf = path->nodes[0];
		if (path->slots[0] >= btrfs_header_nritems(leaf)) {
			ret = btrfs_next_leaf(tree_root, path);
			if (ret < 0)
				err = ret;
			if (ret != 0)
				break;
			leaf = path->nodes[0];
		}

		btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);
		btrfs_release_path(path);

		if (key.objectid != BTRFS_ORPHAN_OBJECTID ||
		    key.type != BTRFS_ORPHAN_ITEM_KEY)
			break;

		root_key.objectid = key.offset;
		key.offset++;

		/*
		 * The root might have been inserted already, as before we look
		 * for orphan roots, log replay might have happened, which
		 * triggers a transaction commit and qgroup accounting, which
		 * in turn reads and inserts fs roots while doing backref
		 * walking.
		 */
		root = btrfs_lookup_fs_root(tree_root->fs_info,
					    root_key.objectid);
		if (root) {
			WARN_ON(!test_bit(BTRFS_ROOT_ORPHAN_ITEM_INSERTED,
					  &root->state));
			if (btrfs_root_refs(&root->root_item) == 0)
				btrfs_add_dead_root(root);
			continue;
		}

		root = btrfs_read_fs_root(tree_root, &root_key);
		err = PTR_ERR_OR_ZERO(root);
		if (err && err != -ENOENT) {
			break;
		} else if (err == -ENOENT) {
			struct btrfs_trans_handle *trans;

			btrfs_release_path(path);

			trans = btrfs_join_transaction(tree_root);
			if (IS_ERR(trans)) {
				err = PTR_ERR(trans);
				btrfs_std_error(tree_root->fs_info, err,
					    "Failed to start trans to delete "
					    "orphan item");
				break;
			}
			err = btrfs_del_orphan_item(trans, tree_root,
						    root_key.objectid);
			btrfs_end_transaction(trans, tree_root);
			if (err) {
				btrfs_std_error(tree_root->fs_info, err,
					    "Failed to delete root orphan "
					    "item");
				break;
			}
			continue;
		}

		err = btrfs_init_fs_root(root);
		if (err) {
			btrfs_free_fs_root(root);
			break;
		}

		set_bit(BTRFS_ROOT_ORPHAN_ITEM_INSERTED, &root->state);

		err = btrfs_insert_fs_root(root->fs_info, root);
		if (err) {
			BUG_ON(err == -EEXIST);
			btrfs_free_fs_root(root);
			break;
		}

		if (btrfs_root_refs(&root->root_item) == 0)
			btrfs_add_dead_root(root);
	}

	btrfs_free_path(path);
	return err;
}

/* drop the root item for 'key' from 'root' */
int btrfs_del_root(struct btrfs_trans_handle *trans, struct btrfs_root *root,
		   struct btrfs_key *key)
{
	struct btrfs_path *path;
	int ret;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
	ret = btrfs_search_slot(trans, root, key, path, -1, 1);
	if (ret < 0)
		goto out;

	BUG_ON(ret != 0);

	ret = btrfs_del_item(trans, root, path);
out:
	btrfs_free_path(path);
	return ret;
}

int btrfs_del_root_ref(struct btrfs_trans_handle *trans,
		       struct btrfs_root *tree_root,
		       u64 root_id, u64 ref_id, u64 dirid, u64 *sequence,
		       const char *name, int name_len)

{
	struct btrfs_path *path;
	struct btrfs_root_ref *ref;
	struct extent_buffer *leaf;
	struct btrfs_key key;
	unsigned long ptr;
	int err = 0;
	int ret;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	key.objectid = root_id;
	key.type = BTRFS_ROOT_BACKREF_KEY;
	key.offset = ref_id;
again:
	ret = btrfs_search_slot(trans, tree_root, &key, path, -1, 1);
	BUG_ON(ret < 0);
	if (ret == 0) {
		leaf = path->nodes[0];
		ref = btrfs_item_ptr(leaf, path->slots[0],
				     struct btrfs_root_ref);

		WARN_ON(btrfs_root_ref_dirid(leaf, ref) != dirid);
		WARN_ON(btrfs_root_ref_name_len(leaf, ref) != name_len);
		ptr = (unsigned long)(ref + 1);
		WARN_ON(memcmp_extent_buffer(leaf, name, ptr, name_len));
		*sequence = btrfs_root_ref_sequence(leaf, ref);

		ret = btrfs_del_item(trans, tree_root, path);
		if (ret) {
			err = ret;
			goto out;
		}
	} else
		err = -ENOENT;

	if (key.type == BTRFS_ROOT_BACKREF_KEY) {
		btrfs_release_path(path);
		key.objectid = ref_id;
		key.type = BTRFS_ROOT_REF_KEY;
		key.offset = root_id;
		goto again;
	}

out:
	btrfs_free_path(path);
	return err;
}

/*
 * add a btrfs_root_ref item.  type is either BTRFS_ROOT_REF_KEY
 * or BTRFS_ROOT_BACKREF_KEY.
 *
 * The dirid, sequence, name and name_len refer to the directory entry
 * that is referencing the root.
 *
 * For a forward ref, the root_id is the id of the tree referencing
 * the root and ref_id is the id of the subvol  or snapshot.
 *
 * For a back ref the root_id is the id of the subvol or snapshot and
 * ref_id is the id of the tree referencing it.
 *
 * Will return 0, -ENOMEM, or anything from the CoW path
 */
int btrfs_add_root_ref(struct btrfs_trans_handle *trans,
		       struct btrfs_root *tree_root,
		       u64 root_id, u64 ref_id, u64 dirid, u64 sequence,
		       const char *name, int name_len)
{
	struct btrfs_key key;
	int ret;
	struct btrfs_path *path;
	struct btrfs_root_ref *ref;
	struct extent_buffer *leaf;
	unsigned long ptr;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	key.objectid = root_id;
	key.type = BTRFS_ROOT_BACKREF_KEY;
	key.offset = ref_id;
again:
	ret = btrfs_insert_empty_item(trans, tree_root, path, &key,
				      sizeof(*ref) + name_len);
	if (ret) {
		btrfs_abort_transaction(trans, tree_root, ret);
		btrfs_free_path(path);
		return ret;
	}

	leaf = path->nodes[0];
	ref = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_root_ref);
	btrfs_set_root_ref_dirid(leaf, ref, dirid);
	btrfs_set_root_ref_sequence(leaf, ref, sequence);
	btrfs_set_root_ref_name_len(leaf, ref, name_len);
	ptr = (unsigned long)(ref + 1);
	write_extent_buffer(leaf, name, ptr, name_len);
	btrfs_mark_buffer_dirty(leaf);

	if (key.type == BTRFS_ROOT_BACKREF_KEY) {
		btrfs_release_path(path);
		key.objectid = ref_id;
		key.type = BTRFS_ROOT_REF_KEY;
		key.offset = root_id;
		goto again;
	}

	btrfs_free_path(path);
	return 0;
}

/*
 * Old btrfs forgets to init root_item->flags and root_item->byte_limit
 * for subvolumes. To work around this problem, we steal a bit from
 * root_item->inode_item->flags, and use it to indicate if those fields
 * have been properly initialized.
 */
void btrfs_check_and_init_root_item(struct btrfs_root_item *root_item)
{
	u64 inode_flags = btrfs_stack_inode_flags(&root_item->inode);

	if (!(inode_flags & BTRFS_INODE_ROOT_ITEM_INIT)) {
		inode_flags |= BTRFS_INODE_ROOT_ITEM_INIT;
		btrfs_set_stack_inode_flags(&root_item->inode, inode_flags);
		btrfs_set_root_flags(root_item, 0);
		btrfs_set_root_limit(root_item, 0);
	}
}

void btrfs_update_root_times(struct btrfs_trans_handle *trans,
			     struct btrfs_root *root)
{
	struct btrfs_root_item *item = &root->root_item;
	struct timespec ct = CURRENT_TIME;

	spin_lock(&root->root_item_lock);
	btrfs_set_root_ctransid(item, trans->transid);
	btrfs_set_stack_timespec_sec(&item->ctime, ct.tv_sec);
	btrfs_set_stack_timespec_nsec(&item->ctime, ct.tv_nsec);
	spin_unlock(&root->root_item_lock);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*
 * Copyright (C) 2012 Alexander Block.  All rights reserved.
 * Copyright (C) 2012 STRATO.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include "ctree.h"

#define BTRFS_SEND_STREAM_MAGIC "btrfs-stream"
#define BTRFS_SEND_STREAM_VERSION 1

#define BTRFS_SEND_BUF_SIZE (1024 * 64)
#define BTRFS_SEND_READ_SIZE (1024 * 48)

enum btrfs_tlv_type {
	BTRFS_TLV_U8,
	BTRFS_TLV_U16,
	BTRFS_TLV_U32,
	BTRFS_TLV_U64,
	BTRFS_TLV_BINARY,
	BTRFS_TLV_STRING,
	BTRFS_TLV_UUID,
	BTRFS_TLV_TIMESPEC,
};

struct btrfs_stream_header {
	char magic[sizeof(BTRFS_SEND_STREAM_MAGIC)];
	__le32 version;
} __attribute__ ((__packed__));

struct btrfs_cmd_header {
	/* len excluding the header */
	__le32 len;
	__le16 cmd;
	/* crc including the header with zero crc field */
	__le32 crc;
} __attribute__ ((__packed__));

struct btrfs_tlv_header {
	__le16 tlv_type;
	/* len excluding the header */
	__le16 tlv_len;
} __attribute__ ((__packed__));

/* commands */
enum btrfs_send_cmd {
	BTRFS_SEND_C_UNSPEC,

	BTRFS_SEND_C_SUBVOL,
	BTRFS_SEND_C_SNAPSHOT,

	BTRFS_SEND_C_MKFILE,
	BTRFS_SEND_C_MKDIR,
	BTRFS_SEND_C_MKNOD,
	BTRFS_SEND_C_MKFIFO,
	BTRFS_SEND_C_MKSOCK,
	BTRFS_SEND_C_SYMLINK,

	BTRFS_SEND_C_RENAME,
	BTRFS_SEND_C_LINK,
	BTRFS_SEND_C_UNLINK,
	BTRFS_SEND_C_RMDIR,

	BTRFS_SEND_C_SET_XATTR,
	BTRFS_SEND_C_REMOVE_XATTR,

	BTRFS_SEND_C_WRITE,
	BTRFS_SEND_C_CLONE,

	BTRFS_SEND_C_TRUNCATE,
	BTRFS_SEND_C_CHMOD,
	BTRFS_SEND_C_CHOWN,
	BTRFS_SEND_C_UTIMES,

	BTRFS_SEND_C_END,
	BTRFS_SEND_C_UPDATE_EXTENT,
	__BTRFS_SEND_C_MAX,
};
#define BTRFS_SEND_C_MAX (__BTRFS_SEND_C_MAX - 1)

/* attributes in send stream */
enum {
	BTRFS_SEND_A_UNSPEC,

	BTRFS_SEND_A_UUID,
	BTRFS_SEND_A_CTRANSID,

	BTRFS_SEND_A_INO,
	BTRFS_SEND_A_SIZE,
	BTRFS_SEND_A_MODE,
	BTRFS_SEND_A_UID,
	BTRFS_SEND_A_GID,
	BTRFS_SEND_A_RDEV,
	BTRFS_SEND_A_CTIME,
	BTRFS_SEND_A_MTIME,
	BTRFS_SEND_A_ATIME,
	BTRFS_SEND_A_OTIME,

	BTRFS_SEND_A_XATTR_NAME,
	BTRFS_SEND_A_XATTR_DATA,

	BTRFS_SEND_A_PATH,
	BTRFS_SEND_A_PATH_TO,
	BTRFS_SEND_A_PATH_LINK,

	BTRFS_SEND_A_FILE_OFFSET,
	BTRFS_SEND_A_DATA,

	BTRFS_SEND_A_CLONE_UUID,
	BTRFS_SEND_A_CLONE_CTRANSID,
	BTRFS_SEND_A_CLONE_PATH,
	BTRFS_SEND_A_CLONE_OFFSET,
	BTRFS_SEND_A_CLONE_LEN,

	__BTRFS_SEND_A_MAX,
};
#define BTRFS_SEND_A_MAX (__BTRFS_SEND_A_MAX - 1)

#ifdef __KERNEL__
long btrfs_ioctl_send(struct file *mnt_file, void __user *arg);
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <linux/highmem.h>
#include <asm/unaligned.h>

#include "ctree.h"

static inline u8 get_unaligned_le8(const void *p)
{
       return *(u8 *)p;
}

static inline void put_unaligned_le8(u8 val, void *p)
{
       *(u8 *)p = val;
}

/*
 * this is some deeply nasty code.
 *
 * The end result is that anyone who #includes ctree.h gets a
 * declaration for the btrfs_set_foo functions and btrfs_foo functions,
 * which are wappers of btrfs_set_token_#bits functions and
 * btrfs_get_token_#bits functions, which are defined in this file.
 *
 * These setget functions do all the extent_buffer related mapping
 * required to efficiently read and write specific fields in the extent
 * buffers.  Every pointer to metadata items in btrfs is really just
 * an unsigned long offset into the extent buffer which has been
 * cast to a specific type.  This gives us all the gcc type checking.
 *
 * The extent buffer api is used to do the page spanning work required to
 * have a metadata blocksize different from the page size.
 */

#define DEFINE_BTRFS_SETGET_BITS(bits)					\
u##bits btrfs_get_token_##bits(const struct extent_buffer *eb,		\
			       const void *ptr, unsigned long off,	\
			       struct btrfs_map_token *token)		\
{									\
	unsigned long part_offset = (unsigned long)ptr;			\
	unsigned long offset = part_offset + off;			\
	void *p;							\
	int err;							\
	char *kaddr;							\
	unsigned long map_start;					\
	unsigned long map_len;						\
	int size = sizeof(u##bits);					\
	u##bits res;							\
									\
	if (token && token->kaddr && token->offset <= offset &&		\
	    token->eb == eb &&						\
	   (token->offset + PAGE_CACHE_SIZE >= offset + size)) {	\
		kaddr = token->kaddr;					\
		p = kaddr + part_offset - token->offset;		\
		res = get_unaligned_le##bits(p + off);			\
		return res;						\
	}								\
	err = map_private_extent_buffer(eb, offset, size,		\
					&kaddr, &map_start, &map_len);	\
	if (err) {							\
		__le##bits leres;					\
									\
		read_extent_buffer(eb, &leres, offset, size);		\
		return le##bits##_to_cpu(leres);			\
	}								\
	p = kaddr + part_offset - map_start;				\
	res = get_unaligned_le##bits(p + off);				\
	if (token) {							\
		token->kaddr = kaddr;					\
		token->offset = map_start;				\
		token->eb = eb;						\
	}								\
	return res;							\
}									\
void btrfs_set_token_##bits(struct extent_buffer *eb,			\
			    const void *ptr, unsigned long off,		\
			    u##bits val,				\
			    struct btrfs_map_token *token)		\
{									\
	unsigned long part_offset = (unsigned long)ptr;			\
	unsigned long offset = part_offset + off;			\
	void *p;							\
	int err;							\
	char *kaddr;							\
	unsigned long map_start;					\
	unsigned long map_len;						\
	int size = sizeof(u##bits);					\
									\
	if (token && token->kaddr && token->offset <= offset &&		\
	    token->eb == eb &&						\
	   (token->offset + PAGE_CACHE_SIZE >= offset + size)) {	\
		kaddr = token->kaddr;					\
		p = kaddr + part_offset - token->offset;		\
		put_unaligned_le##bits(val, p + off);			\
		return;							\
	}								\
	err = map_private_extent_buffer(eb, offset, size,		\
			&kaddr, &map_start, &map_len);			\
	if (err) {							\
		__le##bits val2;					\
									\
		val2 = cpu_to_le##bits(val);				\
		write_extent_buffer(eb, &val2, offset, size);		\
		return;							\
	}								\
	p = kaddr + part_offset - map_start;				\
	put_unaligned_le##bits(val, p + off);				\
	if (token) {							\
		token->kaddr = kaddr;					\
		token->offset = map_start;				\
		token->eb = eb;						\
	}								\
}

DEFINE_BTRFS_SETGET_BITS(8)
DEFINE_BTRFS_SETGET_BITS(16)
DEFINE_BTRFS_SETGET_BITS(32)
DEFINE_BTRFS_SETGET_BITS(64)

void btrfs_node_key(const struct extent_buffer *eb,
		    struct btrfs_disk_key *disk_key, int nr)
{
	unsigned long ptr = btrfs_node_key_ptr_offset(nr);
	read_eb_member(eb, (struct btrfs_key_ptr *)ptr,
		       struct btrfs_key_ptr, key, disk_key);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/mount.h>
#include <linux/mpage.h>
#include <linux/swap.h>
#include <linux/writeback.h>
#include <linux/statfs.h>
#include <linux/compat.h>
#include <linux/parser.h>
#include <linux/ctype.h>
#include <linux/namei.h>
#include <linux/miscdevice.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <linux/cleancache.h>
#include <linux/ratelimit.h>
#include <linux/btrfs.h>
#include "delayed-inode.h"
#include "ctree.h"
#include "disk-io.h"
#include "transaction.h"
#include "btrfs_inode.h"
#include "print-tree.h"
#include "hash.h"
#include "props.h"
#include "xattr.h"
#include "volumes.h"
#include "export.h"
#include "compression.h"
#include "rcu-string.h"
#include "dev-replace.h"
#include "free-space-cache.h"
#include "backref.h"
#include "tests/btrfs-tests.h"

#include "qgroup.h"
#define CREATE_TRACE_POINTS
#include <trace/events/btrfs.h>

static const struct super_operations btrfs_super_ops;
static struct file_system_type btrfs_fs_type;

static int btrfs_remount(struct super_block *sb, int *flags, char *data);

const char *btrfs_decode_error(int errno)
{
	char *errstr = "unknown";

	switch (errno) {
	case -EIO:
		errstr = "IO failure";
		break;
	case -ENOMEM:
		errstr = "Out of memory";
		break;
	case -EROFS:
		errstr = "Readonly filesystem";
		break;
	case -EEXIST:
		errstr = "Object already exists";
		break;
	case -ENOSPC:
		errstr = "No space left";
		break;
	case -ENOENT:
		errstr = "No such entry";
		break;
	}

	return errstr;
}

static void save_error_info(struct btrfs_fs_info *fs_info)
{
	/*
	 * today we only save the error info into ram.  Long term we'll
	 * also send it down to the disk
	 */
	set_bit(BTRFS_FS_STATE_ERROR, &fs_info->fs_state);
}

/* btrfs handle error by forcing the filesystem readonly */
static void btrfs_handle_error(struct btrfs_fs_info *fs_info)
{
	struct super_block *sb = fs_info->sb;

	if (sb->s_flags & MS_RDONLY)
		return;

	if (test_bit(BTRFS_FS_STATE_ERROR, &fs_info->fs_state)) {
		sb->s_flags |= MS_RDONLY;
		btrfs_info(fs_info, "forced readonly");
		/*
		 * Note that a running device replace operation is not
		 * canceled here although there is no way to update
		 * the progress. It would add the risk of a deadlock,
		 * therefore the canceling is ommited. The only penalty
		 * is that some I/O remains active until the procedure
		 * completes. The next time when the filesystem is
		 * mounted writeable again, the device replace
		 * operation continues.
		 */
	}
}

/*
 * __btrfs_std_error decodes expected errors from the caller and
 * invokes the approciate error response.
 */
__cold
void __btrfs_std_error(struct btrfs_fs_info *fs_info, const char *function,
		       unsigned int line, int errno, const char *fmt, ...)
{
	struct super_block *sb = fs_info->sb;
#ifdef CONFIG_PRINTK
	const char *errstr;
#endif

	/*
	 * Special case: if the error is EROFS, and we're already
	 * under MS_RDONLY, then it is safe here.
	 */
	if (errno == -EROFS && (sb->s_flags & MS_RDONLY))
  		return;

#ifdef CONFIG_PRINTK
	errstr = btrfs_decode_error(errno);
	if (fmt) {
		struct va_format vaf;
		va_list args;

		va_start(args, fmt);
		vaf.fmt = fmt;
		vaf.va = &args;

		printk(KERN_CRIT
			"BTRFS: error (device %s) in %s:%d: errno=%d %s (%pV)\n",
			sb->s_id, function, line, errno, errstr, &vaf);
		va_end(args);
	} else {
		printk(KERN_CRIT "BTRFS: error (device %s) in %s:%d: errno=%d %s\n",
			sb->s_id, function, line, errno, errstr);
	}
#endif

	/* Don't go through full error handling during mount */
	save_error_info(fs_info);
	if (sb->s_flags & MS_BORN)
		btrfs_handle_error(fs_info);
}

#ifdef CONFIG_PRINTK
static const char * const logtypes[] = {
	"emergency",
	"alert",
	"critical",
	"error",
	"warning",
	"notice",
	"info",
	"debug",
};

void btrfs_printk(const struct btrfs_fs_info *fs_info, const char *fmt, ...)
{
	struct super_block *sb = fs_info->sb;
	char lvl[4];
	struct va_format vaf;
	va_list args;
	const char *type = logtypes[4];
	int kern_level;

	va_start(args, fmt);

	kern_level = printk_get_level(fmt);
	if (kern_level) {
		size_t size = printk_skip_level(fmt) - fmt;
		memcpy(lvl, fmt,  size);
		lvl[size] = '\0';
		fmt += size;
		type = logtypes[kern_level - '0'];
	} else
		*lvl = '\0';

	vaf.fmt = fmt;
	vaf.va = &args;

	printk("%sBTRFS %s (device %s): %pV\n", lvl, type, sb->s_id, &vaf);

	va_end(args);
}
#endif

/*
 * We only mark the transaction aborted and then set the file system read-only.
 * This will prevent new transactions from starting or trying to join this
 * one.
 *
 * This means that error recovery at the call site is limited to freeing
 * any local memory allocations and passing the error code up without
 * further cleanup. The transaction should complete as it normally would
 * in the call path but will return -EIO.
 *
 * We'll complete the cleanup in btrfs_end_transaction and
 * btrfs_commit_transaction.
 */
__cold
void __btrfs_abort_transaction(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root, const char *function,
			       unsigned int line, int errno)
{
	trans->aborted = errno;
	/* Nothing used. The other threads that have joined this
	 * transaction may be able to continue. */
	if (!trans->dirty && list_empty(&trans->new_bgs)) {
		const char *errstr;

		errstr = btrfs_decode_error(errno);
		btrfs_warn(root->fs_info,
		           "%s:%d: Aborting unused transaction(%s).",
		           function, line, errstr);
		return;
	}
	ACCESS_ONCE(trans->transaction->aborted) = errno;
	/* Wake up anybody who may be waiting on this transaction */
	wake_up(&root->fs_info->transaction_wait);
	wake_up(&root->fs_info->transaction_blocked_wait);
	__btrfs_std_error(root->fs_info, function, line, errno, NULL);
}
/*
 * __btrfs_panic decodes unexpected, fatal errors from the caller,
 * issues an alert, and either panics or BUGs, depending on mount options.
 */
__cold
void __btrfs_panic(struct btrfs_fs_info *fs_info, const char *function,
		   unsigned int line, int errno, const char *fmt, ...)
{
	char *s_id = "<unknown>";
	const char *errstr;
	struct va_format vaf = { .fmt = fmt };
	va_list args;

	if (fs_info)
		s_id = fs_info->sb->s_id;

	va_start(args, fmt);
	vaf.va = &args;

	errstr = btrfs_decode_error(errno);
	if (fs_info && (fs_info->mount_opt & BTRFS_MOUNT_PANIC_ON_FATAL_ERROR))
		panic(KERN_CRIT "BTRFS panic (device %s) in %s:%d: %pV (errno=%d %s)\n",
			s_id, function, line, &vaf, errno, errstr);

	btrfs_crit(fs_info, "panic in %s:%d: %pV (errno=%d %s)",
		   function, line, &vaf, errno, errstr);
	va_end(args);
	/* Caller calls BUG() */
}

static void btrfs_put_super(struct super_block *sb)
{
	close_ctree(btrfs_sb(sb)->tree_root);
}

enum {
	Opt_degraded, Opt_subvol, Opt_subvolid, Opt_device, Opt_nodatasum,
	Opt_nodatacow, Opt_max_inline, Opt_alloc_start, Opt_nobarrier, Opt_ssd,
	Opt_nossd, Opt_ssd_spread, Opt_thread_pool, Opt_noacl, Opt_compress,
	Opt_compress_type, Opt_compress_force, Opt_compress_force_type,
	Opt_notreelog, Opt_ratio, Opt_flushoncommit, Opt_discard,
	Opt_space_cache, Opt_clear_cache, Opt_user_subvol_rm_allowed,
	Opt_enospc_debug, Opt_subvolrootid, Opt_defrag, Opt_inode_cache,
	Opt_no_space_cache, Opt_recovery, Opt_skip_balance,
	Opt_check_integrity, Opt_check_integrity_including_extent_data,
	Opt_check_integrity_print_mask, Opt_fatal_errors, Opt_rescan_uuid_tree,
	Opt_commit_interval, Opt_barrier, Opt_nodefrag, Opt_nodiscard,
	Opt_noenospc_debug, Opt_noflushoncommit, Opt_acl, Opt_datacow,
	Opt_datasum, Opt_treelog, Opt_noinode_cache,
#ifdef CONFIG_BTRFS_DEBUG
	Opt_fragment_data, Opt_fragment_metadata, Opt_fragment_all,
#endif
	Opt_err,
};

static match_table_t tokens = {
	{Opt_degraded, "degraded"},
	{Opt_subvol, "subvol=%s"},
	{Opt_subvolid, "subvolid=%s"},
	{Opt_device, "device=%s"},
	{Opt_nodatasum, "nodatasum"},
	{Opt_datasum, "datasum"},
	{Opt_nodatacow, "nodatacow"},
	{Opt_datacow, "datacow"},
	{Opt_nobarrier, "nobarrier"},
	{Opt_barrier, "barrier"},
	{Opt_max_inline, "max_inline=%s"},
	{Opt_alloc_start, "alloc_start=%s"},
	{Opt_thread_pool, "thread_pool=%d"},
	{Opt_compress, "compress"},
	{Opt_compress_type, "compress=%s"},
	{Opt_compress_force, "compress-force"},
	{Opt_compress_force_type, "compress-force=%s"},
	{Opt_ssd, "ssd"},
	{Opt_ssd_spread, "ssd_spread"},
	{Opt_nossd, "nossd"},
	{Opt_acl, "acl"},
	{Opt_noacl, "noacl"},
	{Opt_notreelog, "notreelog"},
	{Opt_treelog, "treelog"},
	{Opt_flushoncommit, "flushoncommit"},
	{Opt_noflushoncommit, "noflushoncommit"},
	{Opt_ratio, "metadata_ratio=%d"},
	{Opt_discard, "discard"},
	{Opt_nodiscard, "nodiscard"},
	{Opt_space_cache, "space_cache"},
	{Opt_clear_cache, "clear_cache"},
	{Opt_user_subvol_rm_allowed, "user_subvol_rm_allowed"},
	{Opt_enospc_debug, "enospc_debug"},
	{Opt_noenospc_debug, "noenospc_debug"},
	{Opt_subvolrootid, "subvolrootid=%d"},
	{Opt_defrag, "autodefrag"},
	{Opt_nodefrag, "noautodefrag"},
	{Opt_inode_cache, "inode_cache"},
	{Opt_noinode_cache, "noinode_cache"},
	{Opt_no_space_cache, "nospace_cache"},
	{Opt_recovery, "recovery"},
	{Opt_skip_balance, "skip_balance"},
	{Opt_check_integrity, "check_int"},
	{Opt_check_integrity_including_extent_data, "check_int_data"},
	{Opt_check_integrity_print_mask, "check_int_print_mask=%d"},
	{Opt_rescan_uuid_tree, "rescan_uuid_tree"},
	{Opt_fatal_errors, "fatal_errors=%s"},
	{Opt_commit_interval, "commit=%d"},
#ifdef CONFIG_BTRFS_DEBUG
	{Opt_fragment_data, "fragment=data"},
	{Opt_fragment_metadata, "fragment=metadata"},
	{Opt_fragment_all, "fragment=all"},
#endif
	{Opt_err, NULL},
};

/*
 * Regular mount options parser.  Everything that is needed only when
 * reading in a new superblock is parsed here.
 * XXX JDM: This needs to be cleaned up for remount.
 */
int btrfs_parse_options(struct btrfs_root *root, char *options)
{
	struct btrfs_fs_info *info = root->fs_info;
	substring_t args[MAX_OPT_ARGS];
	char *p, *num, *orig = NULL;
	u64 cache_gen;
	int intarg;
	int ret = 0;
	char *compress_type;
	bool compress_force = false;

	cache_gen = btrfs_super_cache_generation(root->fs_info->super_copy);
	if (cache_gen)
		btrfs_set_opt(info->mount_opt, SPACE_CACHE);

	if (!options)
		goto out;

	/*
	 * strsep changes the string, duplicate it because parse_options
	 * gets called twice
	 */
	options = kstrdup(options, GFP_NOFS);
	if (!options)
		return -ENOMEM;

	orig = options;

	while ((p = strsep(&options, ",")) != NULL) {
		int token;
		if (!*p)
			continue;

		token = match_token(p, tokens, args);
		switch (token) {
		case Opt_degraded:
			btrfs_info(root->fs_info, "allowing degraded mounts");
			btrfs_set_opt(info->mount_opt, DEGRADED);
			break;
		case Opt_subvol:
		case Opt_subvolid:
		case Opt_subvolrootid:
		case Opt_device:
			/*
			 * These are parsed by btrfs_parse_early_options
			 * and can be happily ignored here.
			 */
			break;
		case Opt_nodatasum:
			btrfs_set_and_info(root, NODATASUM,
					   "setting nodatasum");
			break;
		case Opt_datasum:
			if (btrfs_test_opt(root, NODATASUM)) {
				if (btrfs_test_opt(root, NODATACOW))
					btrfs_info(root->fs_info, "setting datasum, datacow enabled");
				else
					btrfs_info(root->fs_info, "setting datasum");
			}
			btrfs_clear_opt(info->mount_opt, NODATACOW);
			btrfs_clear_opt(info->mount_opt, NODATASUM);
			break;
		case Opt_nodatacow:
			if (!btrfs_test_opt(root, NODATACOW)) {
				if (!btrfs_test_opt(root, COMPRESS) ||
				    !btrfs_test_opt(root, FORCE_COMPRESS)) {
					btrfs_info(root->fs_info,
						   "setting nodatacow, compression disabled");
				} else {
					btrfs_info(root->fs_info, "setting nodatacow");
				}
			}
			btrfs_clear_opt(info->mount_opt, COMPRESS);
			btrfs_clear_opt(info->mount_opt, FORCE_COMPRESS);
			btrfs_set_opt(info->mount_opt, NODATACOW);
			btrfs_set_opt(info->mount_opt, NODATASUM);
			break;
		case Opt_datacow:
			btrfs_clear_and_info(root, NODATACOW,
					     "setting datacow");
			break;
		case Opt_compress_force:
		case Opt_compress_force_type:
			compress_force = true;
			/* Fallthrough */
		case Opt_compress:
		case Opt_compress_type:
			if (token == Opt_compress ||
			    token == Opt_compress_force ||
			    strcmp(args[0].from, "zlib") == 0) {
				compress_type = "zlib";
				info->compress_type = BTRFS_COMPRESS_ZLIB;
				btrfs_set_opt(info->mount_opt, COMPRESS);
				btrfs_clear_opt(info->mount_opt, NODATACOW);
				btrfs_clear_opt(info->mount_opt, NODATASUM);
			} else if (strcmp(args[0].from, "lzo") == 0) {
				compress_type = "lzo";
				info->compress_type = BTRFS_COMPRESS_LZO;
				btrfs_set_opt(info->mount_opt, COMPRESS);
				btrfs_clear_opt(info->mount_opt, NODATACOW);
				btrfs_clear_opt(info->mount_opt, NODATASUM);
				btrfs_set_fs_incompat(info, COMPRESS_LZO);
			} else if (strncmp(args[0].from, "no", 2) == 0) {
				compress_type = "no";
				btrfs_clear_opt(info->mount_opt, COMPRESS);
				btrfs_clear_opt(info->mount_opt, FORCE_COMPRESS);
				compress_force = false;
			} else {
				ret = -EINVAL;
				goto out;
			}

			if (compress_force) {
				btrfs_set_and_info(root, FORCE_COMPRESS,
						   "force %s compression",
						   compress_type);
			} else {
				if (!btrfs_test_opt(root, COMPRESS))
					btrfs_info(root->fs_info,
						   "btrfs: use %s compression",
						   compress_type);
				/*
				 * If we remount from compress-force=xxx to
				 * compress=xxx, we need clear FORCE_COMPRESS
				 * flag, otherwise, there is no way for users
				 * to disable forcible compression separately.
				 */
				btrfs_clear_opt(info->mount_opt, FORCE_COMPRESS);
			}
			break;
		case Opt_ssd:
			btrfs_set_and_info(root, SSD,
					   "use ssd allocation scheme");
			break;
		case Opt_ssd_spread:
			btrfs_set_and_info(root, SSD_SPREAD,
					   "use spread ssd allocation scheme");
			btrfs_set_opt(info->mount_opt, SSD);
			break;
		case Opt_nossd:
			btrfs_set_and_info(root, NOSSD,
					     "not using ssd allocation scheme");
			btrfs_clear_opt(info->mount_opt, SSD);
			break;
		case Opt_barrier:
			btrfs_clear_and_info(root, NOBARRIER,
					     "turning on barriers");
			break;
		case Opt_nobarrier:
			btrfs_set_and_info(root, NOBARRIER,
					   "turning off barriers");
			break;
		case Opt_thread_pool:
			ret = match_int(&args[0], &intarg);
			if (ret) {
				goto out;
			} else if (intarg > 0) {
				info->thread_pool_size = intarg;
			} else {
				ret = -EINVAL;
				goto out;
			}
			break;
		case Opt_max_inline:
			num = match_strdup(&args[0]);
			if (num) {
				info->max_inline = memparse(num, NULL);
				kfree(num);

				if (info->max_inline) {
					info->max_inline = min_t(u64,
						info->max_inline,
						root->sectorsize);
				}
				btrfs_info(root->fs_info, "max_inline at %llu",
					info->max_inline);
			} else {
				ret = -ENOMEM;
				goto out;
			}
			break;
		case Opt_alloc_start:
			num = match_strdup(&args[0]);
			if (num) {
				mutex_lock(&info->chunk_mutex);
				info->alloc_start = memparse(num, NULL);
				mutex_unlock(&info->chunk_mutex);
				kfree(num);
				btrfs_info(root->fs_info, "allocations start at %llu",
					info->alloc_start);
			} else {
				ret = -ENOMEM;
				goto out;
			}
			break;
		case Opt_acl:
#ifdef CONFIG_BTRFS_FS_POSIX_ACL
			root->fs_info->sb->s_flags |= MS_POSIXACL;
			break;
#else
			btrfs_err(root->fs_info,
				"support for ACL not compiled in!");
			ret = -EINVAL;
			goto out;
#endif
		case Opt_noacl:
			root->fs_info->sb->s_flags &= ~MS_POSIXACL;
			break;
		case Opt_notreelog:
			btrfs_set_and_info(root, NOTREELOG,
					   "disabling tree log");
			break;
		case Opt_treelog:
			btrfs_clear_and_info(root, NOTREELOG,
					     "enabling tree log");
			break;
		case Opt_flushoncommit:
			btrfs_set_and_info(root, FLUSHONCOMMIT,
					   "turning on flush-on-commit");
			break;
		case Opt_noflushoncommit:
			btrfs_clear_and_info(root, FLUSHONCOMMIT,
					     "turning off flush-on-commit");
			break;
		case Opt_ratio:
			ret = match_int(&args[0], &intarg);
			if (ret) {
				goto out;
			} else if (intarg >= 0) {
				info->metadata_ratio = intarg;
				btrfs_info(root->fs_info, "metadata ratio %d",
				       info->metadata_ratio);
			} else {
				ret = -EINVAL;
				goto out;
			}
			break;
		case Opt_discard:
			btrfs_set_and_info(root, DISCARD,
					   "turning on discard");
			break;
		case Opt_nodiscard:
			btrfs_clear_and_info(root, DISCARD,
					     "turning off discard");
			break;
		case Opt_space_cache:
			btrfs_set_and_info(root, SPACE_CACHE,
					   "enabling disk space caching");
			break;
		case Opt_rescan_uuid_tree:
			btrfs_set_opt(info->mount_opt, RESCAN_UUID_TREE);
			break;
		case Opt_no_space_cache:
			btrfs_clear_and_info(root, SPACE_CACHE,
					     "disabling disk space caching");
			break;
		case Opt_inode_cache:
			btrfs_set_pending_and_info(info, INODE_MAP_CACHE,
					   "enabling inode map caching");
			break;
		case Opt_noinode_cache:
			btrfs_clear_pending_and_info(info, INODE_MAP_CACHE,
					     "disabling inode map caching");
			break;
		case Opt_clear_cache:
			btrfs_set_and_info(root, CLEAR_CACHE,
					   "force clearing of disk cache");
			break;
		case Opt_user_subvol_rm_allowed:
			btrfs_set_opt(info->mount_opt, USER_SUBVOL_RM_ALLOWED);
			break;
		case Opt_enospc_debug:
			btrfs_set_opt(info->mount_opt, ENOSPC_DEBUG);
			break;
		case Opt_noenospc_debug:
			btrfs_clear_opt(info->mount_opt, ENOSPC_DEBUG);
			break;
		case Opt_defrag:
			btrfs_set_and_info(root, AUTO_DEFRAG,
					   "enabling auto defrag");
			break;
		case Opt_nodefrag:
			btrfs_clear_and_info(root, AUTO_DEFRAG,
					     "disabling auto defrag");
			break;
		case Opt_recovery:
			btrfs_info(root->fs_info, "enabling auto recovery");
			btrfs_set_opt(info->mount_opt, RECOVERY);
			break;
		case Opt_skip_balance:
			btrfs_set_opt(info->mount_opt, SKIP_BALANCE);
			break;
#ifdef CONFIG_BTRFS_FS_CHECK_INTEGRITY
		case Opt_check_integrity_including_extent_data:
			btrfs_info(root->fs_info,
				   "enabling check integrity including extent data");
			btrfs_set_opt(info->mount_opt,
				      CHECK_INTEGRITY_INCLUDING_EXTENT_DATA);
			btrfs_set_opt(info->mount_opt, CHECK_INTEGRITY);
			break;
		case Opt_check_integrity:
			btrfs_info(root->fs_info, "enabling check integrity");
			btrfs_set_opt(info->mount_opt, CHECK_INTEGRITY);
			break;
		case Opt_check_integrity_print_mask:
			ret = match_int(&args[0], &intarg);
			if (ret) {
				goto out;
			} else if (intarg >= 0) {
				info->check_integrity_print_mask = intarg;
				btrfs_info(root->fs_info, "check_integrity_print_mask 0x%x",
				       info->check_integrity_print_mask);
			} else {
				ret = -EINVAL;
				goto out;
			}
			break;
#else
		case Opt_check_integrity_including_extent_data:
		case Opt_check_integrity:
		case Opt_check_integrity_print_mask:
			btrfs_err(root->fs_info,
				"support for check_integrity* not compiled in!");
			ret = -EINVAL;
			goto out;
#endif
		case Opt_fatal_errors:
			if (strcmp(args[0].from, "panic") == 0)
				btrfs_set_opt(info->mount_opt,
					      PANIC_ON_FATAL_ERROR);
			else if (strcmp(args[0].from, "bug") == 0)
				btrfs_clear_opt(info->mount_opt,
					      PANIC_ON_FATAL_ERROR);
			else {
				ret = -EINVAL;
				goto out;
			}
			break;
		case Opt_commit_interval:
			intarg = 0;
			ret = match_int(&args[0], &intarg);
			if (ret < 0) {
				btrfs_err(root->fs_info, "invalid commit interval");
				ret = -EINVAL;
				goto out;
			}
			if (intarg > 0) {
				if (intarg > 300) {
					btrfs_warn(root->fs_info, "excessive commit interval %d",
							intarg);
				}
				info->commit_interval = intarg;
			} else {
				btrfs_info(root->fs_info, "using default commit interval %ds",
				    BTRFS_DEFAULT_COMMIT_INTERVAL);
				info->commit_interval = BTRFS_DEFAULT_COMMIT_INTERVAL;
			}
			break;
#ifdef CONFIG_BTRFS_DEBUG
		case Opt_fragment_all:
			btrfs_info(root->fs_info, "fragmenting all space");
			btrfs_set_opt(info->mount_opt, FRAGMENT_DATA);
			btrfs_set_opt(info->mount_opt, FRAGMENT_METADATA);
			break;
		case Opt_fragment_metadata:
			btrfs_info(root->fs_info, "fragmenting metadata");
			btrfs_set_opt(info->mount_opt,
				      FRAGMENT_METADATA);
			break;
		case Opt_fragment_data:
			btrfs_info(root->fs_info, "fragmenting data");
			btrfs_set_opt(info->mount_opt, FRAGMENT_DATA);
			break;
#endif
		case Opt_err:
			btrfs_info(root->fs_info, "unrecognized mount option '%s'", p);
			ret = -EINVAL;
			goto out;
		default:
			break;
		}
	}
out:
	if (!ret && btrfs_test_opt(root, SPACE_CACHE))
		btrfs_info(root->fs_info, "disk space caching is enabled");
	kfree(orig);
	return ret;
}

/*
 * Parse mount options that are required early in the mount process.
 *
 * All other options will be parsed on much later in the mount process and
 * only when we need to allocate a new super block.
 */
static int btrfs_parse_early_options(const char *options, fmode_t flags,
		void *holder, char **subvol_name, u64 *subvol_objectid,
		struct btrfs_fs_devices **fs_devices)
{
	substring_t args[MAX_OPT_ARGS];
	char *device_name, *opts, *orig, *p;
	char *num = NULL;
	int error = 0;

	if (!options)
		return 0;

	/*
	 * strsep changes the string, duplicate it because parse_options
	 * gets called twice
	 */
	opts = kstrdup(options, GFP_KERNEL);
	if (!opts)
		return -ENOMEM;
	orig = opts;

	while ((p = strsep(&opts, ",")) != NULL) {
		int token;
		if (!*p)
			continue;

		token = match_token(p, tokens, args);
		switch (token) {
		case Opt_subvol:
			kfree(*subvol_name);
			*subvol_name = match_strdup(&args[0]);
			if (!*subvol_name) {
				error = -ENOMEM;
				goto out;
			}
			break;
		case Opt_subvolid:
			num = match_strdup(&args[0]);
			if (num) {
				*subvol_objectid = memparse(num, NULL);
				kfree(num);
				/* we want the original fs_tree */
				if (!*subvol_objectid)
					*subvol_objectid =
						BTRFS_FS_TREE_OBJECTID;
			} else {
				error = -EINVAL;
				goto out;
			}
			break;
		case Opt_subvolrootid:
			printk(KERN_WARNING
				"BTRFS: 'subvolrootid' mount option is deprecated and has "
				"no effect\n");
			break;
		case Opt_device:
			device_name = match_strdup(&args[0]);
			if (!device_name) {
				error = -ENOMEM;
				goto out;
			}
			error = btrfs_scan_one_device(device_name,
					flags, holder, fs_devices);
			kfree(device_name);
			if (error)
				goto out;
			break;
		default:
			break;
		}
	}

out:
	kfree(orig);
	return error;
}

char *btrfs_get_subvol_name_from_objectid(struct btrfs_fs_info *fs_info,
					  u64 subvol_objectid)
{
	struct btrfs_root *root = fs_info->tree_root;
	struct btrfs_root *fs_root;
	struct btrfs_root_ref *root_ref;
	struct btrfs_inode_ref *inode_ref;
	struct btrfs_key key;
	struct btrfs_path *path = NULL;
	char *name = NULL, *ptr;
	u64 dirid;
	int len;
	int ret;

	path = btrfs_alloc_path();
	if (!path) {
		ret = -ENOMEM;
		goto err;
	}
	path->leave_spinning = 1;

	name = kmalloc(PATH_MAX, GFP_NOFS);
	if (!name) {
		ret = -ENOMEM;
		goto err;
	}
	ptr = name + PATH_MAX - 1;
	ptr[0] = '\0';

	/*
	 * Walk up the subvolume trees in the tree of tree roots by root
	 * backrefs until we hit the top-level subvolume.
	 */
	while (subvol_objectid != BTRFS_FS_TREE_OBJECTID) {
		key.objectid = subvol_objectid;
		key.type = BTRFS_ROOT_BACKREF_KEY;
		key.offset = (u64)-1;

		ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
		if (ret < 0) {
			goto err;
		} else if (ret > 0) {
			ret = btrfs_previous_item(root, path, subvol_objectid,
						  BTRFS_ROOT_BACKREF_KEY);
			if (ret < 0) {
				goto err;
			} else if (ret > 0) {
				ret = -ENOENT;
				goto err;
			}
		}

		btrfs_item_key_to_cpu(path->nodes[0], &key, path->slots[0]);
		subvol_objectid = key.offset;

		root_ref = btrfs_item_ptr(path->nodes[0], path->slots[0],
					  struct btrfs_root_ref);
		len = btrfs_root_ref_name_len(path->nodes[0], root_ref);
		ptr -= len + 1;
		if (ptr < name) {
			ret = -ENAMETOOLONG;
			goto err;
		}
		read_extent_buffer(path->nodes[0], ptr + 1,
				   (unsigned long)(root_ref + 1), len);
		ptr[0] = '/';
		dirid = btrfs_root_ref_dirid(path->nodes[0], root_ref);
		btrfs_release_path(path);

		key.objectid = subvol_objectid;
		key.type = BTRFS_ROOT_ITEM_KEY;
		key.offset = (u64)-1;
		fs_root = btrfs_read_fs_root_no_name(fs_info, &key);
		if (IS_ERR(fs_root)) {
			ret = PTR_ERR(fs_root);
			goto err;
		}

		/*
		 * Walk up the filesystem tree by inode refs until we hit the
		 * root directory.
		 */
		while (dirid != BTRFS_FIRST_FREE_OBJECTID) {
			key.objectid = dirid;
			key.type = BTRFS_INODE_REF_KEY;
			key.offset = (u64)-1;

			ret = btrfs_search_slot(NULL, fs_root, &key, path, 0, 0);
			if (ret < 0) {
				goto err;
			} else if (ret > 0) {
				ret = btrfs_previous_item(fs_root, path, dirid,
							  BTRFS_INODE_REF_KEY);
				if (ret < 0) {
					goto err;
				} else if (ret > 0) {
					ret = -ENOENT;
					goto err;
				}
			}

			btrfs_item_key_to_cpu(path->nodes[0], &key, path->slots[0]);
			dirid = key.offset;

			inode_ref = btrfs_item_ptr(path->nodes[0],
						   path->slots[0],
						   struct btrfs_inode_ref);
			len = btrfs_inode_ref_name_len(path->nodes[0],
						       inode_ref);
			ptr -= len + 1;
			if (ptr < name) {
				ret = -ENAMETOOLONG;
				goto err;
			}
			read_extent_buffer(path->nodes[0], ptr + 1,
					   (unsigned long)(inode_ref + 1), len);
			ptr[0] = '/';
			btrfs_release_path(path);
		}
	}

	btrfs_free_path(path);
	if (ptr == name + PATH_MAX - 1) {
		name[0] = '/';
		name[1] = '\0';
	} else {
		memmove(name, ptr, name + PATH_MAX - ptr);
	}
	return name;

err:
	btrfs_free_path(path);
	kfree(name);
	return ERR_PTR(ret);
}

static int get_default_subvol_objectid(struct btrfs_fs_info *fs_info, u64 *objectid)
{
	struct btrfs_root *root = fs_info->tree_root;
	struct btrfs_dir_item *di;
	struct btrfs_path *path;
	struct btrfs_key location;
	u64 dir_id;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
	path->leave_spinning = 1;

	/*
	 * Find the "default" dir item which points to the root item that we
	 * will mount by default if we haven't been given a specific subvolume
	 * to mount.
	 */
	dir_id = btrfs_super_root_dir(fs_info->super_copy);
	di = btrfs_lookup_dir_item(NULL, root, path, dir_id, "default", 7, 0);
	if (IS_ERR(di)) {
		btrfs_free_path(path);
		return PTR_ERR(di);
	}
	if (!di) {
		/*
		 * Ok the default dir item isn't there.  This is weird since
		 * it's always been there, but don't freak out, just try and
		 * mount the top-level subvolume.
		 */
		btrfs_free_path(path);
		*objectid = BTRFS_FS_TREE_OBJECTID;
		return 0;
	}

	btrfs_dir_item_key_to_cpu(path->nodes[0], di, &location);
	btrfs_free_path(path);
	*objectid = location.objectid;
	return 0;
}

static int btrfs_fill_super(struct super_block *sb,
			    struct btrfs_fs_devices *fs_devices,
			    void *data, int silent)
{
	struct inode *inode;
	struct btrfs_fs_info *fs_info = btrfs_sb(sb);
	struct btrfs_key key;
	int err;

	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_magic = BTRFS_SUPER_MAGIC;
	sb->s_op = &btrfs_super_ops;
	sb->s_d_op = &btrfs_dentry_operations;
	sb->s_export_op = &btrfs_export_ops;
	sb->s_xattr = btrfs_xattr_handlers;
	sb->s_time_gran = 1;
#ifdef CONFIG_BTRFS_FS_POSIX_ACL
	sb->s_flags |= MS_POSIXACL;
#endif
	sb->s_flags |= MS_I_VERSION;
	sb->s_iflags |= SB_I_CGROUPWB;
	err = open_ctree(sb, fs_devices, (char *)data);
	if (err) {
		printk(KERN_ERR "BTRFS: open_ctree failed\n");
		return err;
	}

	key.objectid = BTRFS_FIRST_FREE_OBJECTID;
	key.type = BTRFS_INODE_ITEM_KEY;
	key.offset = 0;
	inode = btrfs_iget(sb, &key, fs_info->fs_root, NULL);
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto fail_close;
	}

	sb->s_root = d_make_root(inode);
	if (!sb->s_root) {
		err = -ENOMEM;
		goto fail_close;
	}

	save_mount_options(sb, data);
	cleancache_init_fs(sb);
	sb->s_flags |= MS_ACTIVE;
	return 0;

fail_close:
	close_ctree(fs_info->tree_root);
	return err;
}

int btrfs_sync_fs(struct super_block *sb, int wait)
{
	struct btrfs_trans_handle *trans;
	struct btrfs_fs_info *fs_info = btrfs_sb(sb);
	struct btrfs_root *root = fs_info->tree_root;

	trace_btrfs_sync_fs(wait);

	if (!wait) {
		filemap_flush(fs_info->btree_inode->i_mapping);
		return 0;
	}

	btrfs_wait_ordered_roots(fs_info, -1);

	trans = btrfs_attach_transaction_barrier(root);
	if (IS_ERR(trans)) {
		/* no transaction, don't bother */
		if (PTR_ERR(trans) == -ENOENT) {
			/*
			 * Exit unless we have some pending changes
			 * that need to go through commit
			 */
			if (fs_info->pending_changes == 0)
				return 0;
			/*
			 * A non-blocking test if the fs is frozen. We must not
			 * start a new transaction here otherwise a deadlock
			 * happens. The pending operations are delayed to the
			 * next commit after thawing.
			 */
			if (__sb_start_write(sb, SB_FREEZE_WRITE, false))
				__sb_end_write(sb, SB_FREEZE_WRITE);
			else
				return 0;
			trans = btrfs_start_transaction(root, 0);
		}
		if (IS_ERR(trans))
			return PTR_ERR(trans);
	}
	return btrfs_commit_transaction(trans, root);
}

static int btrfs_show_options(struct seq_file *seq, struct dentry *dentry)
{
	struct btrfs_fs_info *info = btrfs_sb(dentry->d_sb);
	struct btrfs_root *root = info->tree_root;
	char *compress_type;
	const char *subvol_name;

	if (btrfs_test_opt(root, DEGRADED))
		seq_puts(seq, ",degraded");
	if (btrfs_test_opt(root, NODATASUM))
		seq_puts(seq, ",nodatasum");
	if (btrfs_test_opt(root, NODATACOW))
		seq_puts(seq, ",nodatacow");
	if (btrfs_test_opt(root, NOBARRIER))
		seq_puts(seq, ",nobarrier");
	if (info->max_inline != BTRFS_DEFAULT_MAX_INLINE)
		seq_printf(seq, ",max_inline=%llu", info->max_inline);
	if (info->alloc_start != 0)
		seq_printf(seq, ",alloc_start=%llu", info->alloc_start);
	if (info->thread_pool_size !=  min_t(unsigned long,
					     num_online_cpus() + 2, 8))
		seq_printf(seq, ",thread_pool=%d", info->thread_pool_size);
	if (btrfs_test_opt(root, COMPRESS)) {
		if (info->compress_type == BTRFS_COMPRESS_ZLIB)
			compress_type = "zlib";
		else
			compress_type = "lzo";
		if (btrfs_test_opt(root, FORCE_COMPRESS))
			seq_printf(seq, ",compress-force=%s", compress_type);
		else
			seq_printf(seq, ",compress=%s", compress_type);
	}
	if (btrfs_test_opt(root, NOSSD))
		seq_puts(seq, ",nossd");
	if (btrfs_test_opt(root, SSD_SPREAD))
		seq_puts(seq, ",ssd_spread");
	else if (btrfs_test_opt(root, SSD))
		seq_puts(seq, ",ssd");
	if (btrfs_test_opt(root, NOTREELOG))
		seq_puts(seq, ",notreelog");
	if (btrfs_test_opt(root, FLUSHONCOMMIT))
		seq_puts(seq, ",flushoncommit");
	if (btrfs_test_opt(root, DISCARD))
		seq_puts(seq, ",discard");
	if (!(root->fs_info->sb->s_flags & MS_POSIXACL))
		seq_puts(seq, ",noacl");
	if (btrfs_test_opt(root, SPACE_CACHE))
		seq_puts(seq, ",space_cache");
	else
		seq_puts(seq, ",nospace_cache");
	if (btrfs_test_opt(root, RESCAN_UUID_TREE))
		seq_puts(seq, ",rescan_uuid_tree");
	if (btrfs_test_opt(root, CLEAR_CACHE))
		seq_puts(seq, ",clear_cache");
	if (btrfs_test_opt(root, USER_SUBVOL_RM_ALLOWED))
		seq_puts(seq, ",user_subvol_rm_allowed");
	if (btrfs_test_opt(root, ENOSPC_DEBUG))
		seq_puts(seq, ",enospc_debug");
	if (btrfs_test_opt(root, AUTO_DEFRAG))
		seq_puts(seq, ",autodefrag");
	if (btrfs_test_opt(root, INODE_MAP_CACHE))
		seq_puts(seq, ",inode_cache");
	if (btrfs_test_opt(root, SKIP_BALANCE))
		seq_puts(seq, ",skip_balance");
	if (btrfs_test_opt(root, RECOVERY))
		seq_puts(seq, ",recovery");
#ifdef CONFIG_BTRFS_FS_CHECK_INTEGRITY
	if (btrfs_test_opt(root, CHECK_INTEGRITY_INCLUDING_EXTENT_DATA))
		seq_puts(seq, ",check_int_data");
	else if (btrfs_test_opt(root, CHECK_INTEGRITY))
		seq_puts(seq, ",check_int");
	if (info->check_integrity_print_mask)
		seq_printf(seq, ",check_int_print_mask=%d",
				info->check_integrity_print_mask);
#endif
	if (info->metadata_ratio)
		seq_printf(seq, ",metadata_ratio=%d",
				info->metadata_ratio);
	if (btrfs_test_opt(root, PANIC_ON_FATAL_ERROR))
		seq_puts(seq, ",fatal_errors=panic");
	if (info->commit_interval != BTRFS_DEFAULT_COMMIT_INTERVAL)
		seq_printf(seq, ",commit=%d", info->commit_interval);
#ifdef CONFIG_BTRFS_DEBUG
	if (btrfs_test_opt(root, FRAGMENT_DATA))
		seq_puts(seq, ",fragment=data");
	if (btrfs_test_opt(root, FRAGMENT_METADATA))
		seq_puts(seq, ",fragment=metadata");
#endif
	seq_printf(seq, ",subvolid=%llu",
		  BTRFS_I(d_inode(dentry))->root->root_key.objectid);
	subvol_name = btrfs_get_subvol_name_from_objectid(info,
			BTRFS_I(d_inode(dentry))->root->root_key.objectid);
	if (!IS_ERR(subvol_name)) {
		seq_puts(seq, ",subvol=");
		seq_escape(seq, subvol_name, " \t\n\\");
		kfree(subvol_name);
	}
	return 0;
}

static int btrfs_test_super(struct super_block *s, void *data)
{
	struct btrfs_fs_info *p = data;
	struct btrfs_fs_info *fs_info = btrfs_sb(s);

	return fs_info->fs_devices == p->fs_devices;
}

static int btrfs_set_super(struct super_block *s, void *data)
{
	int err = set_anon_super(s, data);
	if (!err)
		s->s_fs_info = data;
	return err;
}

/*
 * subvolumes are identified by ino 256
 */
static inline int is_subvolume_inode(struct inode *inode)
{
	if (inode && inode->i_ino == BTRFS_FIRST_FREE_OBJECTID)
		return 1;
	return 0;
}

/*
 * This will add subvolid=0 to the argument string while removing any subvol=
 * and subvolid= arguments to make sure we get the top-level root for path
 * walking to the subvol we want.
 */
static char *setup_root_args(char *args)
{
	char *buf, *dst, *sep;

	if (!args)
		return kstrdup("subvolid=0", GFP_NOFS);

	/* The worst case is that we add ",subvolid=0" to the end. */
	buf = dst = kmalloc(strlen(args) + strlen(",subvolid=0") + 1, GFP_NOFS);
	if (!buf)
		return NULL;

	while (1) {
		sep = strchrnul(args, ',');
		if (!strstarts(args, "subvol=") &&
		    !strstarts(args, "subvolid=")) {
			memcpy(dst, args, sep - args);
			dst += sep - args;
			*dst++ = ',';
		}
		if (*sep)
			args = sep + 1;
		else
			break;
	}
	strcpy(dst, "subvolid=0");

	return buf;
}

static struct dentry *mount_subvol(const char *subvol_name, u64 subvol_objectid,
				   int flags, const char *device_name,
				   char *data)
{
	struct dentry *root;
	struct vfsmount *mnt = NULL;
	char *newargs;
	int ret;

	newargs = setup_root_args(data);
	if (!newargs) {
		root = ERR_PTR(-ENOMEM);
		goto out;
	}

	mnt = vfs_kern_mount(&btrfs_fs_type, flags, device_name, newargs);
	if (PTR_ERR_OR_ZERO(mnt) == -EBUSY) {
		if (flags & MS_RDONLY) {
			mnt = vfs_kern_mount(&btrfs_fs_type, flags & ~MS_RDONLY,
					     device_name, newargs);
		} else {
			mnt = vfs_kern_mount(&btrfs_fs_type, flags | MS_RDONLY,
					     device_name, newargs);
			if (IS_ERR(mnt)) {
				root = ERR_CAST(mnt);
				mnt = NULL;
				goto out;
			}

			down_write(&mnt->mnt_sb->s_umount);
			ret = btrfs_remount(mnt->mnt_sb, &flags, NULL);
			up_write(&mnt->mnt_sb->s_umount);
			if (ret < 0) {
				root = ERR_PTR(ret);
				goto out;
			}
		}
	}
	if (IS_ERR(mnt)) {
		root = ERR_CAST(mnt);
		mnt = NULL;
		goto out;
	}

	if (!subvol_name) {
		if (!subvol_objectid) {
			ret = get_default_subvol_objectid(btrfs_sb(mnt->mnt_sb),
							  &subvol_objectid);
			if (ret) {
				root = ERR_PTR(ret);
				goto out;
			}
		}
		subvol_name = btrfs_get_subvol_name_from_objectid(
					btrfs_sb(mnt->mnt_sb), subvol_objectid);
		if (IS_ERR(subvol_name)) {
			root = ERR_CAST(subvol_name);
			subvol_name = NULL;
			goto out;
		}

	}

	root = mount_subtree(mnt, subvol_name);
	/* mount_subtree() drops our reference on the vfsmount. */
	mnt = NULL;

	if (!IS_ERR(root)) {
		struct super_block *s = root->d_sb;
		struct inode *root_inode = d_inode(root);
		u64 root_objectid = BTRFS_I(root_inode)->root->root_key.objectid;

		ret = 0;
		if (!is_subvolume_inode(root_inode)) {
			pr_err("BTRFS: '%s' is not a valid subvolume\n",
			       subvol_name);
			ret = -EINVAL;
		}
		if (subvol_objectid && root_objectid != subvol_objectid) {
			/*
			 * This will also catch a race condition where a
			 * subvolume which was passed by ID is renamed and
			 * another subvolume is renamed over the old location.
			 */
			pr_err("BTRFS: subvol '%s' does not match subvolid %llu\n",
			       subvol_name, subvol_objectid);
			ret = -EINVAL;
		}
		if (ret) {
			dput(root);
			root = ERR_PTR(ret);
			deactivate_locked_super(s);
		}
	}

out:
	mntput(mnt);
	kfree(newargs);
	kfree(subvol_name);
	return root;
}

static int parse_security_options(char *orig_opts,
				  struct security_mnt_opts *sec_opts)
{
	char *secdata = NULL;
	int ret = 0;

	secdata = alloc_secdata();
	if (!secdata)
		return -ENOMEM;
	ret = security_sb_copy_data(orig_opts, secdata);
	if (ret) {
		free_secdata(secdata);
		return ret;
	}
	ret = security_sb_parse_opts_str(secdata, sec_opts);
	free_secdata(secdata);
	return ret;
}

static int setup_security_options(struct btrfs_fs_info *fs_info,
				  struct super_block *sb,
				  struct security_mnt_opts *sec_opts)
{
	int ret = 0;

	/*
	 * Call security_sb_set_mnt_opts() to check whether new sec_opts
	 * is valid.
	 */
	ret = security_sb_set_mnt_opts(sb, sec_opts, 0, NULL);
	if (ret)
		return ret;

#ifdef CONFIG_SECURITY
	if (!fs_info->security_opts.num_mnt_opts) {
		/* first time security setup, copy sec_opts to fs_info */
		memcpy(&fs_info->security_opts, sec_opts, sizeof(*sec_opts));
	} else {
		/*
		 * Since SELinux(the only one supports security_mnt_opts) does
		 * NOT support changing context during remount/mount same sb,
		 * This must be the same or part of the same security options,
		 * just free it.
		 */
		security_free_mnt_opts(sec_opts);
	}
#endif
	return ret;
}

/*
 * Find a superblock for the given device / mount point.
 *
 * Note:  This is based on get_sb_bdev from fs/super.c with a few additions
 *	  for multiple device setup.  Make sure to keep it in sync.
 */
static struct dentry *btrfs_mount(struct file_system_type *fs_type, int flags,
		const char *device_name, void *data)
{
	struct block_device *bdev = NULL;
	struct super_block *s;
	struct btrfs_fs_devices *fs_devices = NULL;
	struct btrfs_fs_info *fs_info = NULL;
	struct security_mnt_opts new_sec_opts;
	fmode_t mode = FMODE_READ;
	char *subvol_name = NULL;
	u64 subvol_objectid = 0;
	int error = 0;

	if (!(flags & MS_RDONLY))
		mode |= FMODE_WRITE;

	error = btrfs_parse_early_options(data, mode, fs_type,
					  &subvol_name, &subvol_objectid,
					  &fs_devices);
	if (error) {
		kfree(subvol_name);
		return ERR_PTR(error);
	}

	if (subvol_name || subvol_objectid != BTRFS_FS_TREE_OBJECTID) {
		/* mount_subvol() will free subvol_name. */
		return mount_subvol(subvol_name, subvol_objectid, flags,
				    device_name, data);
	}

	security_init_mnt_opts(&new_sec_opts);
	if (data) {
		error = parse_security_options(data, &new_sec_opts);
		if (error)
			return ERR_PTR(error);
	}

	error = btrfs_scan_one_device(device_name, mode, fs_type, &fs_devices);
	if (error)
		goto error_sec_opts;

	/*
	 * Setup a dummy root and fs_info for test/set super.  This is because
	 * we don't actually fill this stuff out until open_ctree, but we need
	 * it for searching for existing supers, so this lets us do that and
	 * then open_ctree will properly initialize everything later.
	 */
	fs_info = kzalloc(sizeof(struct btrfs_fs_info), GFP_NOFS);
	if (!fs_info) {
		error = -ENOMEM;
		goto error_sec_opts;
	}

	fs_info->fs_devices = fs_devices;

	fs_info->super_copy = kzalloc(BTRFS_SUPER_INFO_SIZE, GFP_NOFS);
	fs_info->super_for_commit = kzalloc(BTRFS_SUPER_INFO_SIZE, GFP_NOFS);
	security_init_mnt_opts(&fs_info->security_opts);
	if (!fs_info->super_copy || !fs_info->super_for_commit) {
		error = -ENOMEM;
		goto error_fs_info;
	}

	error = btrfs_open_devices(fs_devices, mode, fs_type);
	if (error)
		goto error_fs_info;

	if (!(flags & MS_RDONLY) && fs_devices->rw_devices == 0) {
		error = -EACCES;
		goto error_close_devices;
	}

	bdev = fs_devices->latest_bdev;
	s = sget(fs_type, btrfs_test_super, btrfs_set_super, flags | MS_NOSEC,
		 fs_info);
	if (IS_ERR(s)) {
		error = PTR_ERR(s);
		goto error_close_devices;
	}

	if (s->s_root) {
		btrfs_close_devices(fs_devices);
		free_fs_info(fs_info);
		if ((flags ^ s->s_flags) & MS_RDONLY)
			error = -EBUSY;
	} else {
		char b[BDEVNAME_SIZE];

		strlcpy(s->s_id, bdevname(bdev, b), sizeof(s->s_id));
		btrfs_sb(s)->bdev_holder = fs_type;
		error = btrfs_fill_super(s, fs_devices, data,
					 flags & MS_SILENT ? 1 : 0);
	}
	if (error) {
		deactivate_locked_super(s);
		goto error_sec_opts;
	}

	fs_info = btrfs_sb(s);
	error = setup_security_options(fs_info, s, &new_sec_opts);
	if (error) {
		deactivate_locked_super(s);
		goto error_sec_opts;
	}

	return dget(s->s_root);

error_close_devices:
	btrfs_close_devices(fs_devices);
error_fs_info:
	free_fs_info(fs_info);
error_sec_opts:
	security_free_mnt_opts(&new_sec_opts);
	return ERR_PTR(error);
}

static void btrfs_resize_thread_pool(struct btrfs_fs_info *fs_info,
				     int new_pool_size, int old_pool_size)
{
	if (new_pool_size == old_pool_size)
		return;

	fs_info->thread_pool_size = new_pool_size;

	btrfs_info(fs_info, "resize thread pool %d -> %d",
	       old_pool_size, new_pool_size);

	btrfs_workqueue_set_max(fs_info->workers, new_pool_size);
	btrfs_workqueue_set_max(fs_info->delalloc_workers, new