
	struct vmw_resource_val_node *node;
	int ret;

	if (*id_loc == SVGA3D_INVALID_ID) {
		if (p_val)
			*p_val = NULL;
		if (res_type == vmw_res_context) {
			DRM_ERROR("Illegal context invalid id.\n");
			return -EINVAL;
		}
		return 0;
	}

	/*
	 * Fastpath in case of repeated commands referencing the same
	 * resource
	 */

	if (likely(rcache->valid && *id_loc == rcache->handle)) {
		const struct vmw_resource *res = rcache->res;

		rcache->node->first_usage = false;
		if (p_val)
			*p_val = rcache->node;

		return vmw_resource_relocation_add
			(&sw_context->res_relocations, res,
			 id_loc - sw_context->buf_start);
	}

	ret = vmw_user_resource_lookup_handle(dev_priv,
					      sw_context->fp->tfile,
					      *id_loc,
					      converter,
					      &res);
	if (unlikely(ret != 0)) {
		DRM_ERROR("Could not find or use resource 0x%08x.\n",
			  (unsigned) *id_loc);
		dump_stack();
		return ret;
	}

	rcache->valid = true;
	rcache->res = res;
	rcache->handle = *id_loc;

	ret = vmw_cmd_res_reloc_add(dev_priv, sw_context, id_loc,
				    res, &node);
	if (unlikely(ret != 0))
		goto out_no_reloc;

	rcache->node = node;
	if (p_val)
		*p_val = node;
	vmw_resource_unreference(&res);
	return 0;

out_no_reloc:
	BUG_ON(sw_context->error_resource != NULL);
	sw_context->error_resource = res;

	return ret;
}

/**
 * vmw_rebind_dx_query - Rebind DX query associated with the context
 *
 * @ctx_res: context the query belongs to
 *
 * This function assumes binding_mutex is held.
 */
static int vmw_rebind_all_dx_query(struct vmw_resource *ctx_res)
{
	struct vmw_private *dev_priv = ctx_res->dev_priv;
	struct vmw_dma_buffer *dx_query_mob;
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXBindAllQuery body;
	} *cmd;


	dx_query_mob = vmw_context_get_dx_query_mob(ctx_res);

	if (!dx_query_mob || dx_query_mob->dx_query_ctx)
		return 0;

	cmd = vmw_fifo_reserve_dx(dev_priv, sizeof(*cmd), ctx_res->id);

	if (cmd == NULL) {
		DRM_ERROR("Failed to rebind queries.\n");
		return -ENOMEM;
	}

	cmd->header.id = SVGA_3D_CMD_DX_BIND_ALL_QUERY;
	cmd->header.size = sizeof(cmd->body);
	cmd->body.cid = ctx_res->id;
	cmd->body.mobid = dx_query_mob->base.mem.start;
	vmw_fifo_commit(dev_priv, sizeof(*cmd));

	vmw_context_bind_dx_query(ctx_res, dx_query_mob);

	return 0;
}

/**
 * vmw_rebind_contexts - Rebind all resources previously bound to
 * referenced contexts.
 *
 * @sw_context: Pointer to the software context.
 *
 * Rebind context binding points that have been scrubbed because of eviction.
 */
static int vmw_rebind_contexts(struct vmw_sw_context *sw_context)
{
	struct vmw_resource_val_node *val;
	int ret;

	list_for_each_entry(val, &sw_context->resource_list, head) {
		if (unlikely(!val->staged_bindings))
			break;

		ret = vmw_binding_rebind_all
			(vmw_context_binding_state(val->res));
		if (unlikely(ret != 0)) {
			if (ret != -ERESTARTSYS)
				DRM_ERROR("Failed to rebind context.\n");
			return ret;
		}

		ret = vmw_rebind_all_dx_query(val->res);
		if (ret != 0)
			return ret;
	}

	return 0;
}

/**
 * vmw_view_bindings_add - Add an array of view bindings to a context
 * binding state tracker.
 *
 * @sw_context: The execbuf state used for this command.
 * @view_type: View type for the bindings.
 * @binding_type: Binding type for the bindings.
 * @shader_slot: The shader slot to user for the bindings.
 * @view_ids: Array of view ids to be bound.
 * @num_views: Number of view ids in @view_ids.
 * @first_slot: The binding slot to be used for the first view id in @view_ids.
 */
static int vmw_view_bindings_add(struct vmw_sw_context *sw_context,
				 enum vmw_view_type view_type,
				 enum vmw_ctx_binding_type binding_type,
				 uint32 shader_slot,
				 uint32 view_ids[], u32 num_views,
				 u32 first_slot)
{
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;
	struct vmw_cmdbuf_res_manager *man;
	u32 i;
	int ret;

	if (!ctx_node) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	man = sw_context->man;
	for (i = 0; i < num_views; ++i) {
		struct vmw_ctx_bindinfo_view binding;
		struct vmw_resource *view = NULL;

		if (view_ids[i] != SVGA3D_INVALID_ID) {
			view = vmw_view_lookup(man, view_type, view_ids[i]);
			if (IS_ERR(view)) {
				DRM_ERROR("View not found.\n");
				return PTR_ERR(view);
			}

			ret = vmw_view_res_val_add(sw_context, view);
			if (ret) {
				DRM_ERROR("Could not add view to "
					  "validation list.\n");
				vmw_resource_unreference(&view);
				return ret;
			}
		}
		binding.bi.ctx = ctx_node->res;
		binding.bi.res = view;
		binding.bi.bt = binding_type;
		binding.shader_slot = shader_slot;
		binding.slot = first_slot + i;
		vmw_binding_add(ctx_node->staged_bindings, &binding.bi,
				shader_slot, binding.slot);
		if (view)
			vmw_resource_unreference(&view);
	}

	return 0;
}

/**
 * vmw_cmd_cid_check - Check a command header for valid context information.
 *
 * @dev_priv: Pointer to a device private structure.
 * @sw_context: Pointer to the software context.
 * @header: A command header with an embedded user-space context handle.
 *
 * Convenience function: Call vmw_cmd_res_check with the user-space context
 * handle embedded in @header.
 */
static int vmw_cmd_cid_check(struct vmw_private *dev_priv,
			     struct vmw_sw_context *sw_context,
			     SVGA3dCmdHeader *header)
{
	struct vmw_cid_cmd {
		SVGA3dCmdHeader header;
		uint32_t cid;
	} *cmd;

	cmd = container_of(header, struct vmw_cid_cmd, header);
	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_context,
				 user_context_converter, &cmd->cid, NULL);
}

static int vmw_cmd_set_render_target_check(struct vmw_private *dev_priv,
					   struct vmw_sw_context *sw_context,
					   SVGA3dCmdHeader *header)
{
	struct vmw_sid_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdSetRenderTarget body;
	} *cmd;
	struct vmw_resource_val_node *ctx_node;
	struct vmw_resource_val_node *res_node;
	int ret;

	cmd = container_of(header, struct vmw_sid_cmd, header);

	if (cmd->body.type >= SVGA3D_RT_MAX) {
		DRM_ERROR("Illegal render target type %u.\n",
			  (unsigned) cmd->body.type);
		return -EINVAL;
	}

	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_context,
				user_context_converter, &cmd->body.cid,
				&ctx_node);
	if (unlikely(ret != 0))
		return ret;

	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				user_surface_converter,
				&cmd->body.target.sid, &res_node);
	if (unlikely(ret != 0))
		return ret;

	if (dev_priv->has_mob) {
		struct vmw_ctx_bindinfo_view binding;

		binding.bi.ctx = ctx_node->res;
		binding.bi.res = res_node ? res_node->res : NULL;
		binding.bi.bt = vmw_ctx_binding_rt;
		binding.slot = cmd->body.type;
		vmw_binding_add(ctx_node->staged_bindings,
				&binding.bi, 0, binding.slot);
	}

	return 0;
}

static int vmw_cmd_surface_copy_check(struct vmw_private *dev_priv,
				      struct vmw_sw_context *sw_context,
				      SVGA3dCmdHeader *header)
{
	struct vmw_sid_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdSurfaceCopy body;
	} *cmd;
	int ret;

	cmd = container_of(header, struct vmw_sid_cmd, header);

	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				user_surface_converter,
				&cmd->body.src.sid, NULL);
	if (ret)
		return ret;

	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				 user_surface_converter,
				 &cmd->body.dest.sid, NULL);
}

static int vmw_cmd_buffer_copy_check(struct vmw_private *dev_priv,
				      struct vmw_sw_context *sw_context,
				      SVGA3dCmdHeader *header)
{
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXBufferCopy body;
	} *cmd;
	int ret;

	cmd = container_of(header, typeof(*cmd), header);
	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				user_surface_converter,
				&cmd->body.src, NULL);
	if (ret != 0)
		return ret;

	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				 user_surface_converter,
				 &cmd->body.dest, NULL);
}

static int vmw_cmd_pred_copy_check(struct vmw_private *dev_priv,
				   struct vmw_sw_context *sw_context,
				   SVGA3dCmdHeader *header)
{
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXPredCopyRegion body;
	} *cmd;
	int ret;

	cmd = container_of(header, typeof(*cmd), header);
	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				user_surface_converter,
				&cmd->body.srcSid, NULL);
	if (ret != 0)
		return ret;

	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				 user_surface_converter,
				 &cmd->body.dstSid, NULL);
}

static int vmw_cmd_stretch_blt_check(struct vmw_private *dev_priv,
				     struct vmw_sw_context *sw_context,
				     SVGA3dCmdHeader *header)
{
	struct vmw_sid_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdSurfaceStretchBlt body;
	} *cmd;
	int ret;

	cmd = container_of(header, struct vmw_sid_cmd, header);
	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				user_surface_converter,
				&cmd->body.src.sid, NULL);
	if (unlikely(ret != 0))
		return ret;
	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				 user_surface_converter,
				 &cmd->body.dest.sid, NULL);
}

static int vmw_cmd_blt_surf_screen_check(struct vmw_private *dev_priv,
					 struct vmw_sw_context *sw_context,
					 SVGA3dCmdHeader *header)
{
	struct vmw_sid_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdBlitSurfaceToScreen body;
	} *cmd;

	cmd = container_of(header, struct vmw_sid_cmd, header);

	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				 user_surface_converter,
				 &cmd->body.srcImage.sid, NULL);
}

static int vmw_cmd_present_check(struct vmw_private *dev_priv,
				 struct vmw_sw_context *sw_context,
				 SVGA3dCmdHeader *header)
{
	struct vmw_sid_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdPresent body;
	} *cmd;


	cmd = container_of(header, struct vmw_sid_cmd, header);

	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				 user_surface_converter, &cmd->body.sid,
				 NULL);
}

/**
 * vmw_query_bo_switch_prepare - Prepare to switch pinned buffer for queries.
 *
 * @dev_priv: The device private structure.
 * @new_query_bo: The new buffer holding query results.
 * @sw_context: The software context used for this command submission.
 *
 * This function checks whether @new_query_bo is suitable for holding
 * query results, and if another buffer currently is pinned for query
 * results. If so, the function prepares the state of @sw_context for
 * switching pinned buffers after successful submission of the current
 * command batch.
 */
static int vmw_query_bo_switch_prepare(struct vmw_private *dev_priv,
				       struct vmw_dma_buffer *new_query_bo,
				       struct vmw_sw_context *sw_context)
{
	struct vmw_res_cache_entry *ctx_entry =
		&sw_context->res_cache[vmw_res_context];
	int ret;

	BUG_ON(!ctx_entry->valid);
	sw_context->last_query_ctx = ctx_entry->res;

	if (unlikely(new_query_bo != sw_context->cur_query_bo)) {

		if (unlikely(new_query_bo->base.num_pages > 4)) {
			DRM_ERROR("Query buffer too large.\n");
			return -EINVAL;
		}

		if (unlikely(sw_context->cur_query_bo != NULL)) {
			sw_context->needs_post_query_barrier = true;
			ret = vmw_bo_to_validate_list(sw_context,
						      sw_context->cur_query_bo,
						      dev_priv->has_mob, NULL);
			if (unlikely(ret != 0))
				return ret;
		}
		sw_context->cur_query_bo = new_query_bo;

		ret = vmw_bo_to_validate_list(sw_context,
					      dev_priv->dummy_query_bo,
					      dev_priv->has_mob, NULL);
		if (unlikely(ret != 0))
			return ret;

	}

	return 0;
}


/**
 * vmw_query_bo_switch_commit - Finalize switching pinned query buffer
 *
 * @dev_priv: The device private structure.
 * @sw_context: The software context used for this command submission batch.
 *
 * This function will check if we're switching query buffers, and will then,
 * issue a dummy occlusion query wait used as a query barrier. When the fence
 * object following that query wait has signaled, we are sure that all
 * preceding queries have finished, and the old query buffer can be unpinned.
 * However, since both the new query buffer and the old one are fenced with
 * that fence, we can do an asynchronus unpin now, and be sure that the
 * old query buffer won't be moved until the fence has signaled.
 *
 * As mentioned above, both the new - and old query buffers need to be fenced
 * using a sequence emitted *after* calling this function.
 */
static void vmw_query_bo_switch_commit(struct vmw_private *dev_priv,
				     struct vmw_sw_context *sw_context)
{
	/*
	 * The validate list should still hold references to all
	 * contexts here.
	 */

	if (sw_context->needs_post_query_barrier) {
		struct vmw_res_cache_entry *ctx_entry =
			&sw_context->res_cache[vmw_res_context];
		struct vmw_resource *ctx;
