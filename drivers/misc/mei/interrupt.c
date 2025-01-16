kely(cur_size != 0)) {
		DRM_ERROR("Command verifier out of sync.\n");
		return -EINVAL;
	}

	return 0;
}

static void vmw_free_relocations(struct vmw_sw_context *sw_context)
{
	sw_context->cur_reloc = 0;
}

static void vmw_apply_relocations(struct vmw_sw_context *sw_context)
{
	uint32_t i;
	struct vmw_relocation *reloc;
	struct ttm_validate_buffer *validate;
	struct ttm_buffer_object *bo;

	for (i = 0; i < sw_context->cur_reloc; ++i) {
		reloc = &sw_context->relocs[i];
		validate = &sw_context->val_bufs[reloc->index].base;
		bo = validate->bo;
		switch (bo->mem.mem_type) {
		case TTM_PL_VRAM:
			reloc->location->offset += bo->offset;
			reloc->location->gmrId = SVGA_GMR_FRAMEBUFFER;
			break;
		case VMW_PL_GMR:
			reloc->location->gmrId = bo->mem.start;
			break;
		case VMW_PL_MOB:
			*reloc->mob_loc = bo->mem.start;
			break;
		default:
			BUG();
		}
	}
	vmw_free_relocations(sw_context);
}

/**
 * vmw_resource_list_unrefererence - Free up a resource list and unreference
 * all resources referenced by it.
 *
 * @list: The resource list.
 */
static void vmw_resource_list_unreference(struct vmw_sw_context *sw_context,
					  struct list_head *list)
{
	struct vmw_resource_val_node *val, *val_next;

	/*
	 * Drop references to resources held during command submission.
	 */

	list_for_each_entry_safe(val, val_next, list, head) {
		list_del_init(&val->head);
		vmw_resource_unreference(&val->res);

		if (val->staged_bindings) {
			if (val->staged_bindings != sw_context->staged_bindings)
				vmw_binding_state_free(val->staged_bindings);
			else
				sw_context->staged_bindings_inuse = false;
			val->staged_bindings = NULL;
		}

		kfree(val);
	}
}

static void vmw_clear_validations(struct vmw_sw_context *sw_context)
{
	struct vmw_validate_buffer *entry, *next;
	struct vmw_resource_val_node *val;

	/*
	 * Drop references to DMA buffers held during command submission.
	 */
	list_for_each_entry_safe(entry, next, &sw_context->validate_nodes,
				 base.head) {
		list_del(&entry->base.head);
		ttm_bo_unref(&entry->base.bo);
		(void) drm_ht_remove_item(&sw_context->res_ht, &entry->hash);
		sw_context->cur_val_buf--;
	}
	BUG_ON(sw_context->cur_val_buf != 0);

	list_for_each_entry(val, &sw_context->resource_list, head)
		(void) drm_ht_remove_item(&sw_context->res_ht, &val->hash);
}

int vmw_validate_single_buffer(struct vmw_private *dev_priv,
			       struct ttm_buffer_object *bo,
			       bool interruptible,
			       bool validate_as_mob)
{
	struct vmw_dma_buffer *vbo = container_of(bo, struct vmw_dma_buffer,
						  base);
	int ret;

	if (vbo->pin_count > 0)
		return 0;

	if (validate_as_mob)
		return ttm_bo_validate(bo, &vmw_mob_placement, interruptible,
				       false);

	/**
	 * Put BO in VRAM if there is space, otherwise as a GMR.
	 * If there is no space in VRAM and GMR ids are all used up,
	 * start evicting GMRs to make room. If the DMA buffer can't be
	 * used as a GMR, this will return -ENOMEM.
	 */

	ret = ttm_bo_validate(bo, &vmw_vram_gmr_placement, interruptible,
			      false);
	if (likely(ret == 0 || ret == -ERESTARTSYS))
		return ret;

	/**
	 * If that failed, try VRAM again, this time evicting
	 * previous contents.
	 */

	ret = ttm_bo_validate(bo, &vmw_vram_placement, interruptible, false);
	return ret;
}

static int vmw_validate_buffers(struct vmw_private *dev_priv,
				struct vmw_sw_context *sw_context)
{
	struct vmw_validate_buffer *entry;
	int ret;

	list_for_each_entry(entry, &sw_context->validate_nodes, base.head) {
		ret = vmw_validate_single_buffer(dev_priv, entry->base.bo,
						 true,
						 entry->validate_as_mob);
		if (unlikely(ret != 0))
			return ret;
	}
	return 0;
}

static int vmw_resize_cmd_bounce(struct vmw_sw_context *sw_context,
				 uint32_t size)
{
	if (likely(sw_context->cmd_bounce_size >= size))
		return 0;

	if (sw_context->cmd_bounce_size == 0)
		sw_context->cmd_bounce_size = VMWGFX_CMD_BOUNCE_INIT_SIZE;

	while (sw_context->cmd_bounce_size < size) {
		sw_context->cmd_bounce_size =
			PAGE_ALIGN(sw_context->cmd_bounce_size +
				   (sw_context->cmd_bounce_size >> 1));
	}

	if (sw_context->cmd_bounce != NULL)
		vfree(sw_context->cmd_bounce);

	sw_context->cmd_bounce = vmalloc(sw_context->cmd_bounce_size);

	if (sw_context->cmd_bounce == NULL) {
		DRM_ERROR("Failed to allocate command bounce buffer.\n");
		sw_context->cmd_bounce_size = 0;
		return -ENOMEM;
	}

	return 0;
}

/**
 * vmw_execbuf_fence_commands - create and submit a command stream fence
 *
 * Creates a fence object and submits a command stream marker.
 * If this fails for some reason, We sync the fifo and return NULL.
 * It is then safe to fence buffers with a NULL pointer.
 *
 * If @p_handle is not NULL @file_priv must also not be NULL. Creates
 * a userspace handle if @p_handle is not NULL, otherwise not.
 */

int vmw_execbuf_fence_commands(struct drm_file *file_priv,
			       struct vmw_private *dev_priv,
			       struct vmw_fence_obj **p_fence,
			       uint32_t *p_handle)
{
	uint32_t sequence;
	int ret;
	bool synced = false;

	/* p_handle implies file_priv. */
	BUG_ON(p_handle != NULL && file_priv == NULL);

	ret = vmw_fifo_send_fence(dev_priv, &sequence);
	if (unlikely(ret != 0)) {
		DRM_ERROR("Fence submission error. Syncing.\n");
		synced = true;
	}

	if (p_handle != NULL)
		ret = vmw_user_fence_create(file_priv, dev_priv->fman,
					    sequence, p_fence, p_handle);
	else
		ret = vmw_fence_create(dev_priv->fman, sequence, p_fence);

	if (unlikely(ret != 0 && !synced)) {
		(void) vmw_fallback_wait(dev_priv, false, false,
					 sequence, false,
					 VMW_FENCE_WAIT_TIMEOUT);
		*p_fence = NULL;
	}

	return ret;
}

/**
 * vmw_execbuf_copy_fence_user - copy fence object information to
 * user-space.
 *
 * @dev_priv: Pointer to a vmw_private struct.
 * @vmw_fp: Pointer to the struct vmw_fpriv representing the calling file.
 * @ret: Return value from fence object creation.
 * @user_fence_rep: User space address of a struct drm_vmw_fence_rep to
 * which the information should be copied.
 * @fence: Pointer to the fenc object.
 * @fence_handle: User-space fence handle.
 *
 * This function copies fence information to user-space. If copying fails,
 * The user-space struct drm_vmw_fence_rep::error member is hopefully
 * left untouched, and if it's preloaded with an -EFAULT by user-space,
 * the error will hopefully be detected.
 * Also if copying fails, user-space will be unable to signal the fence
 * object so we wait for it immediately, and then unreference the
 * user-space reference.
 */
void
vmw_execbuf_copy_fence_user(struct vmw_private *dev_priv,
			    struct vmw_fpriv *vmw_fp,
			    int ret,
			    struct drm_vmw_fence_rep __user *user_fence_rep,
			    struct vmw_fence_obj *fence,
			    uint32_t fence_handle)
{
	struct drm_vmw_fence_rep fence_rep;

	if (user_fence_rep == NULL)
		return;

	memset(&fence_rep, 0, sizeof(fence_rep));

	fence_rep.error = ret;
	if (ret == 0) {
		BUG_ON(fence == NULL);

		fence_rep.handle = fence_handle;
		fence_rep.seqno = fence->base.seqno;
		vmw_update_seqno(dev_priv, &dev_priv->fifo);
		fence_rep.passed_seqno = dev_priv->last_read_seqno;
	}

	/*
	 * copy_to_user errors will be detected by user space not
	 * seeing fence_rep::error filled in. Typically
	 * user-space would have pre-set that member to -EFAULT.
	 */
	ret = copy_to_user(user_fence_rep, &fence_rep,
			   sizeof(fence_rep));

	/*
	 * User-space lost the fence object. We need to sync
	 * and unreference the handle.
	 */
	if (unlikely(ret != 0) && (fence_rep.error == 0)) {
		ttm_ref_object_base_unref(vmw_fp->tfile,
					  fence_handle, TTM_REF_USAGE);
		DRM_ERROR("Fence copy error. Syncing.\n");
		(void) vmw_fence_obj_wait(fence, false, false,
					  VMW_FENCE_WAIT_TIMEOUT);
	}
}

/**
 * vmw_execbuf_submit_fifo - Patch a command batch and submit it using
 * the fifo.
 *
 * @dev_priv: Pointer to a device private structure.
 * @kernel_commands: Pointer to the unpatched command batch.
 * @command_size: Size of the unpatched command batch.
 * @sw_context: Structure holding the relocation lists.
 *
 * Side effects: If this function returns 0, then the command batch
 * pointed to by @kernel_commands will have been modified.
 */
static int vmw_execbuf_submit_fifo(struct vmw_private *dev_priv,
				   void *kernel_commands,
				   u32 command_size,
				   struct vmw_sw_context *sw_context)
{
	void *cmd;

	if (sw_context->dx_ctx_node)
		cmd = vmw_fifo_reserve_dx(dev_priv, command_size,
					  sw_context->dx_ctx_node->res->id);
	else
		cmd = vmw_fifo_reserve(dev_priv, command_size);
	if (!cmd) {
		DRM_ERROR("Failed reserving fifo space for commands.\n");
		return -ENOMEM;
	}

	vmw_apply_relocations(sw_context);
	memcpy(cmd, kernel_commands, command_size);
	vmw_resource_relocations_apply(cmd, &sw_context->res_relocations);
	vmw_resource_relocations_free(&sw_context->res_relocations);
	vmw_fifo_commit(dev_priv, command_size);

	return 0;
}

/**
 * vmw_execbuf_submit_cmdbuf - Patch a command batch and submit it using
 * the command buffer manager.
 *
 * @dev_priv: Pointer to a device private structure.
 * @header: Opaque handle to the command buffer allocation.
 * @command_size: Size of the unpatched command batch.
 * @sw_context: Structure holding the relocation lists.
 *
 * Side effects: If this function returns 0, then the command buffer
 * represented by @header will have been modified.
 */
static int vmw_execbuf_submit_cmdbuf(struct vmw_private *dev_priv,
				     struct vmw_cmdbuf_header *header,
				     u32 command_size,
				     struct vmw_sw_context *sw_context)
{
	u32 id = ((sw_context->dx_ctx_node) ? sw_context->dx_ctx_node->res->id :
		  SVGA3D_INVALID_ID);
	void *cmd = vmw_cmdbuf_reserve(dev_priv->cman, command_size,
				       id, false, header);

	vmw_apply_relocations(sw_context);
	vmw_resource_relocations_apply(cmd, &sw_context->res_relocations);
	vmw_resource_relocations_free(&sw_context->res_relocations);
	vmw_cmdbuf_commit(dev_priv->cman, command_size, header, false);

	return 0;
}

/**
 * vmw_execbuf_cmdbuf - Prepare, if possible, a user-space command batch for
 * submission using a command buffer.
 *
 * @dev_priv: Pointer to a device private structure.
 * @user_commands: User-space pointer to the commands to be submitted.
 * @command_size: Size of the unpatched command batch.
 * @header: Out parameter returning the opaque pointer to the command buffer.
 *
 * This function checks whether we can use the command buffer manager for
 * submission and if so, creates a command buffer of suitable size and
 * copies the user data into that buffer.
 *
 * On successful return, the function returns a pointer to the data in the
 * command buffer and *@header is set to non-NULL.
 * If command buffers could not be used, the function will return the value
 * of @kernel_commands on function call. That value may be NULL. In that case,
 * the value of *@header will be set to NULL.
 * If an error is encountered, the function will return a pointer error value.
 * If the function is interrupted by a signal while sleeping, it will return
 * -ERESTARTSYS casted to a pointer error value.
 */
static void *vmw_execbuf_cmdbuf(struct vmw_private *dev_priv,
				void __user *user_commands,
				void *kernel_commands,
				u32 command_size,
				struct vmw_cmdbuf_header **header)
{
	size_t cmdbuf_size;
	int ret;

	*header = NULL;
	if (command_size > SVGA_CB_MAX_SIZE) {
		DRM_ERROR("Command buffer is too large.\n");
		return ERR_PTR(-EINVAL);
	}

	if (!dev_priv->cman || kernel_commands)
		return kernel_commands;

	/* If possible, add a little space for fencing. */
	cmdbuf_size = command_size + 512;
	cmdbuf_size = min_t(size_t, cmdbuf_size, SVGA_CB_MAX_SIZE);
	kernel_commands = vmw_cmdbuf_alloc(dev_priv->cman, cmdbuf_size,
					   true, header);
	if (IS_ERR(kernel_commands))
		return kernel_commands;

	ret = copy_from_user(kernel_commands, user_commands,
			     command_size);
	if (ret) {
		DRM_ERROR("Failed copying commands.\n");
		vmw_cmdbuf_header_free(*header);
		*header = NULL;
		return ERR_PTR(-EFAULT);
	}

	return kernel_commands;
}

static int vmw_execbuf_tie_context(struct vmw_private *dev_priv,
				   struct vmw_sw_context *sw_context,
				   uint32_t handle)
{
	struct vmw_resource_val_node *ctx_node;
	struct vmw_resource *res;
	int ret;

	if (handle == SVGA3D_INVALID_ID)
		return 0;

	ret = vmw_user_resource_lookup_handle(dev_priv, sw_context->fp->tfile,
					      handle, user_context_converter,
					      &res);
	if (unlikely(ret != 0)) {
		DRM_ERROR("Could not find or user DX context 0x%08x.\n",
			  (unsigned) handle);
		return ret;
	}

	ret = vmw_resource_val_add(sw_context, res, &ctx_node);
	if (unlikely(ret != 0))
		goto out_err;

	sw_context->dx_ctx_node = ctx_node;
	sw_context->man = vmw_context_res_man(res);
out_err:
	vmw_resource_unreference(&res);
	return ret;
}

int vmw_execbuf_process(struct drm_file *file_priv,
			struct vmw_private *dev_pr