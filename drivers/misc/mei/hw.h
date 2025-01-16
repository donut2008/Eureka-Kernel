->has_mob) {
		ret = vmw_rebind_contexts(sw_context);
		if (unlikely(ret != 0))
			goto out_unlock_binding;
	}

	if (!header) {
		ret = vmw_execbuf_submit_fifo(dev_priv, kernel_commands,
					      command_size, sw_context);
	} else {
		ret = vmw_execbuf_submit_cmdbuf(dev_priv, header, command_size,
						sw_context);
		header = NULL;
	}
	mutex_unlock(&dev_priv->binding_mutex);
	if (ret)
		goto out_err;

	vmw_query_bo_switch_commit(dev_priv, sw_context);
	ret = vmw_execbuf_fence_commands(file_priv, dev_priv,
					 &fence,
					 (user_fence_rep) ? &handle : NULL);
	/*
	 * This error is harmless, because if fence submission fails,
	 * vmw_fifo_send_fence will sync. The error will be propagated to
	 * user-space in @fence_rep
	 */

	if (ret != 0)
		DRM_ERROR("Fence submission error. Syncing.\n");

	vmw_resources_unreserve(sw_context, false);

	ttm_eu_fence_buffer_objects(&ticket, &sw_context->validate_nodes,
				    (void *) fence);

	if (unlikely(dev_priv->pinned_bo != NULL &&
		     !dev_priv->query_cid_valid))
		__vmw_execbuf_release_pinned_bo(dev_priv, fence);

	vmw_clear_validations(sw_context);
	vmw_execbuf_copy_fence_user(dev_priv, vmw_fpriv(file_priv), ret,
				    user_fence_rep, fence, handle);

	/* Don't unreference when handing fence out */
	if (unlikely(out_fence != NULL)) {
		*out_fence = fence;
		fence = NULL;
	} else if (likely(fence != NULL)) {
		vmw_fence_obj_unreference(&fence);
	}

	list_splice_init(&sw_context->resource_list, &resource_list);
	vmw_cmdbuf_res_commit(&sw_context->staged_cmd_res);
	mutex_unlock(&dev_priv->cmdbuf_mutex);

	/*
	 * Unreference resources outside of the cmdbuf_mutex to
	 * avoid deadlocks in resource destruction paths.
	 */
	vmw_resource_list_unreference(sw_context, &resource_list);

	return 0;

out_unlock_binding:
	mutex_unlock(&dev_priv->binding_mutex);
out_err:
	ttm_eu_backoff_reservation(&ticket, &sw_context->validate_nodes);
out_err_nores:
	vmw_resources_unreserve(sw_context, true);
	vmw_resource_relocations_free(&sw_context->res_relocations);
	vmw_free_relocations(sw_context);
	vmw_clear_validations(sw_context);
	if (unlikely(dev_priv->pinned_bo != NULL &&
		     !dev_priv->query_cid_valid))
		__vmw_execbuf_release_pinned_bo(dev_priv, NULL);
out_unlock:
	list_splice_init(&sw_context->resource_list, &resource_list);
	error_resource = sw_context->error_resource;
	sw_context->error_resource = NULL;
	vmw_cmdbuf_res_revert(&sw_context->staged_cmd_res);
	mutex_unlock(&dev_priv->cmdbuf_mutex);

	/*
	 * Unreference resources outside of the cmdbuf_mutex to
	 * avoid deadlocks in resource destruction paths.
	 */
	vmw_resource_list_unreference(sw_context, &resource_list);
	if (unlikely(error_resource != NULL))
		vmw_resource_unreference(&error_resource);
out_free_header:
	if (header)
		vmw_cmdbuf_header_free(header);

	return ret;
}

/**
 * vmw_execbuf_unpin_panic - Idle the fifo and unpin the query buffer.
 *
 * @dev_priv: The device private structure.
 *
 * This function is called to idle the fifo and unpin the query buffer
 * if the normal way to do this hits an error, which should typically be
 * extremely rare.
 */
static void vmw_execbuf_unpin_panic(struct vmw_private *dev_priv)
{
	DRM_ERROR("Can't unpin query buffer. Trying to recover.\n");

	(void) vmw_fallback_wait(dev_priv, false, true, 0, false, 10*HZ);
	vmw_bo_pin_reserved(dev_priv->pinned_bo, false);
	if (dev_priv->dummy_query_bo_pinned) {
		vmw_bo_pin_reserved(dev_priv->dummy_query_bo, false);
		dev_priv->dummy_query_bo_pinned = false;
	}
}


/**
 * __vmw_execbuf_release_pinned_bo - Flush queries and unpin the pinned
 * query bo.
 *
 * @dev_priv: The device private structure.
 * @fence: If non-NULL should point to a struct vmw_fence_obj issued
 * _after_ a query barrier that flushes all queries touching the current
 * buffer pointed to by @dev_priv->pinned_bo
 *
 * This function should be used to unpin the pinned query bo, or
 * as a query barrier when we need to make sure that all queries have
 * finished before the next fifo command. (For example on hardware
 * context destructions where the hardware may otherwise leak unfinished
 * queries).
 *
 * This function does not return any failure codes, but make attempts
 * to do safe unpinning in case of errors.
 *
 * The function will synchronize on the previous query barrier, and will
 * thus not finish until that barrier has executed.
 *
 * the @dev_priv->cmdbuf_mutex needs to be held by the current thread
 * before calling this function.
 */
void __vmw_execbuf_release_pinned_bo(struct vmw_private *dev_priv,
				     struct vmw_fence_obj *fence)
{
	int ret = 0;
	struct list_head validate_list;
	struct ttm_validate_buffer pinned_val, query_val;
	struct vmw_fence_obj *lfence = NULL;
	struct ww_acquire_ctx ticket;

	if (dev_priv->pinned_bo == NULL)
		goto out_unlock;

	INIT_LIST_HEAD(&validate_list);

	pinned_val.bo = ttm_bo_reference(&dev_priv->pinned_bo->base);
	pinned_val.shared = false;
	list_add_tail(&pinned_val.head, &validate_list);

	query_val.bo = ttm_bo_reference(&dev_priv->dummy_query_bo->base);
	query_val.shared = false;
	list_add_tail(&query_val.head, &validate_list);

	ret = ttm_eu_reserve_buffers(&ticket, &validate_list,
				     false, NULL);
	if (unlikely(ret != 0)) {
		vmw_execbuf_unpin_panic(dev_priv);
		goto out_no_reserve;
	}

	if (dev_priv->query_cid_valid) {
		BUG_ON(fence != NULL);
		ret = vmw_fifo_emit_dummy_query(dev_priv, dev_priv->query_cid);
		if (unlikely(ret != 0)) {
			vmw_execbuf_unpin_panic(dev_priv);
			goto out_no_emit;
		}
		dev_priv->query_cid_valid = false;
	}

	vmw_bo_pin_reserved(dev_priv->pinned_bo, false);
	if (dev_priv->dummy_query_bo_pinned) {
		vmw_bo_pin_reserved(dev_priv->dummy_query_bo, false);
		dev_priv->dummy_query_bo_pinned = false;
	}
	if (fence == NULL) {
		(void) vmw_execbuf_fence_commands(NULL, dev_priv, &lfence,
						  NULL);
		fence = lfence;
	}
	ttm_eu_fence_buffer_objects(&ticket, &validate_list, (void *) fence);
	if (lfence != NULL)
		vmw_fence_obj_unreference(&lfence);

	ttm_bo_unref(&query_val.bo);
	ttm_bo_unref(&pinned_val.bo);
	vmw_dmabuf_unreference(&dev_priv->pinned_bo);
	DRM_INFO("Dummy query bo pin count: %d\n",
		 dev_priv->dummy_query_bo->pin_count);

out_unlock:
	return;

out_no_emit:
	ttm_eu_backoff_reservation(&ticket, &validate_list);
out_no_reserve:
	ttm_bo_unref(&query_val.bo);
	ttm_bo_unref(&pinned_val.bo);
	vmw_dmabuf_unreference(&dev_priv->pinned_bo);
}

/**
 * vmw_execbuf_release_pinned_bo - Flush queries and unpin the pinned
 * query bo.
 *
 * @dev_priv: The device private structure.
 *
 * This function should be used to unpin the pinned query bo, or
 * as a query barrier when we need to make sure that all queries have
 * finished before the next fifo command. (For example on hardware
 * context destructions where the hardware may otherwise leak unfinished
 * queries).
 *
 * This function does not return any failure codes, but make attempts
 * to do safe unpinning in case of errors.
 *
 * The function will synchronize on the previous query barrier, and will
 * thus not finish until that barrier has executed.
 */
void vmw_execbuf_release_pinned_bo(struct vmw_private *dev_priv)
{
	mutex_lock(&dev_priv->cmdbuf_mutex);
	if (dev_priv->query_cid_valid)
		__vmw_execbuf_release_pinned_bo(dev_priv, NULL);
	mutex_unlock(&dev_priv->cmdbuf_mutex);
}

int vmw_execbuf_ioctl(struct drm_device *dev, unsigned long data,
		      struct drm_file *file_priv, size_t size)
{
	struct vmw_private *dev_priv = vmw_priv(dev);
	struct drm_vmw_execbuf_arg arg;
	int ret;
	static const size_t copy_offset[] = {
		offsetof(struct drm_vmw_execbuf_arg, context_handle),
		sizeof(struct drm_vmw_execbuf_arg)};

	if (unlikely(size < copy_offset[0])) {
		DRM_ERROR("Invalid command size, ioctl %d\n",
			  DRM_VMW_EXECBUF);
		return -EINVAL;
	}

	if (copy_from_user(&arg, (void __user *) data, copy_offset[0]) != 0)
		return -EFAULT;

	/*
	 * Extend the ioctl argument while
	 * maintaining backwards compatibility:
	 * We take different code paths depending on the value of
	 * arg.version.
	 */

	if (unlikely(arg.version > DRM_VMW_EXECBUF_VERSION ||
		     arg.version == 0)) {
		DRM_ERROR("Incorrect execbuf version.\n");
		return -EINVAL;
	}

	if (arg.version > 1 &&
	    copy_from_user(&arg.context_handle,
			   (void __user *) (data + copy_offset[0]),
			   copy_offset[arg.version - 1] -
			   copy_offset[0]) != 0)
		return -EFAULT;

	switch (arg.version) {
	case 1:
		arg.context_handle = (uint32_t) -1;
		break;
	case 2:
		if (arg.pad64 != 0) {
			DRM_ERROR("Unused IOCTL data not set to zero.\n");
			return -EINVAL;
		}
		break;
	default:
		break;
	}

	ret = ttm_read_lock(&dev_priv->reservation_sem, true);
	if (unlikely(ret != 0))
		return ret;

	ret = vmw_execbuf_process(file_priv, dev_priv,
				  (void __user *)(unsigned long)arg.commands,
				  NULL, arg.command_size, arg.throttle_us,
				  arg.context_handle,
				  (void __user *)(unsigned long)arg.fence_rep,
				  NULL);
	ttm_read_unlock(&dev_priv->reservation_sem);
	if (unlikely(ret != 0))
		return ret;

	vmw_kms_cursor_post_execbuf(dev_priv);

	return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       