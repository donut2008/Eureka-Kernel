			   header);

	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_context,
				 user_context_converter, &cmd->q.cid,
				 NULL);
}

/**
 * vmw_cmd_begin_query - validate a  SVGA_3D_CMD_BEGIN_QUERY command.
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context used for this command submission.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_begin_query(struct vmw_private *dev_priv,
			       struct vmw_sw_context *sw_context,
			       SVGA3dCmdHeader *header)
{
	struct vmw_begin_query_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdBeginQuery q;
	} *cmd;

	cmd = container_of(header, struct vmw_begin_query_cmd,
			   header);

	if (unlikely(dev_priv->has_mob)) {
		struct {
			SVGA3dCmdHeader header;
			SVGA3dCmdBeginGBQuery q;
		} gb_cmd;

		BUG_ON(sizeof(gb_cmd) != sizeof(*cmd));

		gb_cmd.header.id = SVGA_3D_CMD_BEGIN_GB_QUERY;
		gb_cmd.header.size = cmd->header.size;
		gb_cmd.q.cid = cmd->q.cid;
		gb_cmd.q.type = cmd->q.type;

		memcpy(cmd, &gb_cmd, sizeof(*cmd));
		return vmw_cmd_begin_gb_query(dev_priv, sw_context, header);
	}

	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_context,
				 user_context_converter, &cmd->q.cid,
				 NULL);
}

/**
 * vmw_cmd_end_gb_query - validate a  SVGA_3D_CMD_END_GB_QUERY command.
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context used for this command submission.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_end_gb_query(struct vmw_private *dev_priv,
				struct vmw_sw_context *sw_context,
				SVGA3dCmdHeader *header)
{
	struct vmw_dma_buffer *vmw_bo;
	struct vmw_query_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdEndGBQuery q;
	} *cmd;
	int ret;

	cmd = container_of(header, struct vmw_query_cmd, header);
	ret = vmw_cmd_cid_check(dev_priv, sw_context, header);
	if (unlikely(ret != 0))
		return ret;

	ret = vmw_translate_mob_ptr(dev_priv, sw_context,
				    &cmd->q.mobid,
				    &vmw_bo);
	if (unlikely(ret != 0))
		return ret;

	ret = vmw_query_bo_switch_prepare(dev_priv, vmw_bo, sw_context);

	vmw_dmabuf_unreference(&vmw_bo);
	return ret;
}

/**
 * vmw_cmd_end_query - validate a  SVGA_3D_CMD_END_QUERY command.
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context used for this command submission.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_end_query(struct vmw_private *dev_priv,
			     struct vmw_sw_context *sw_context,
			     SVGA3dCmdHeader *header)
{
	struct vmw_dma_buffer *vmw_bo;
	struct vmw_query_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdEndQuery q;
	} *cmd;
	int ret;

	cmd = container_of(header, struct vmw_query_cmd, header);
	if (dev_priv->has_mob) {
		struct {
			SVGA3dCmdHeader header;
			SVGA3dCmdEndGBQuery q;
		} gb_cmd;

		BUG_ON(sizeof(gb_cmd) != sizeof(*cmd));

		gb_cmd.header.id = SVGA_3D_CMD_END_GB_QUERY;
		gb_cmd.header.size = cmd->header.size;
		gb_cmd.q.cid = cmd->q.cid;
		gb_cmd.q.type = cmd->q.type;
		gb_cmd.q.mobid = cmd->q.guestResult.gmrId;
		gb_cmd.q.offset = cmd->q.guestResult.offset;

		memcpy(cmd, &gb_cmd, sizeof(*cmd));
		return vmw_cmd_end_gb_query(dev_priv, sw_context, header);
	}

	ret = vmw_cmd_cid_check(dev_priv, sw_context, header);
	if (unlikely(ret != 0))
		return ret;

	ret = vmw_translate_guest_ptr(dev_priv, sw_context,
				      &cmd->q.guestResult,
				      &vmw_bo);
	if (unlikely(ret != 0))
		return ret;

	ret = vmw_query_bo_switch_prepare(dev_priv, vmw_bo, sw_context);

	vmw_dmabuf_unreference(&vmw_bo);
	return ret;
}

/**
 * vmw_cmd_wait_gb_query - validate a  SVGA_3D_CMD_WAIT_GB_QUERY command.
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context used for this command submission.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_wait_gb_query(struct vmw_private *dev_priv,
				 struct vmw_sw_context *sw_context,
				 SVGA3dCmdHeader *header)
{
	struct vmw_dma_buffer *vmw_bo;
	struct vmw_query_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdWaitForGBQuery q;
	} *cmd;
	int ret;

	cmd = container_of(header, struct vmw_query_cmd, header);
	ret = vmw_cmd_cid_check(dev_priv, sw_context, header);
	if (unlikely(ret != 0))
		return ret;

	ret = vmw_translate_mob_ptr(dev_priv, sw_context,
				    &cmd->q.mobid,
				    &vmw_bo);
	if (unlikely(ret != 0))
		return ret;

	vmw_dmabuf_unreference(&vmw_bo);
	return 0;
}

/**
 * vmw_cmd_wait_query - validate a  SVGA_3D_CMD_WAIT_QUERY command.
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context used for this command submission.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_wait_query(struct vmw_private *dev_priv,
			      struct vmw_sw_context *sw_context,
			      SVGA3dCmdHeader *header)
{
	struct vmw_dma_buffer *vmw_bo;
	struct vmw_query_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdWaitForQuery q;
	} *cmd;
	int ret;

	cmd = container_of(header, struct vmw_query_cmd, header);
	if (dev_priv->has_mob) {
		struct {
			SVGA3dCmdHeader header;
			SVGA3dCmdWaitForGBQuery q;
		} gb_cmd;

		BUG_ON(sizeof(gb_cmd) != sizeof(*cmd));

		gb_cmd.header.id = SVGA_3D_CMD_WAIT_FOR_GB_QUERY;
		gb_cmd.header.size = cmd->header.size;
		gb_cmd.q.cid = cmd->q.cid;
		gb_cmd.q.type = cmd->q.type;
		gb_cmd.q.mobid = cmd->q.guestResult.gmrId;
		gb_cmd.q.offset = cmd->q.guestResult.offset;

		memcpy(cmd, &gb_cmd, sizeof(*cmd));
		return vmw_cmd_wait_gb_query(dev_priv, sw_context, header);
	}

	ret = vmw_cmd_cid_check(dev_priv, sw_context, header);
	if (unlikely(ret != 0))
		return ret;

	ret = vmw_translate_guest_ptr(dev_priv, sw_context,
				      &cmd->q.guestResult,
				      &vmw_bo);
	if (unlikely(ret != 0))
		return ret;

	vmw_dmabuf_unreference(&vmw_bo);
	return 0;
}

static int vmw_cmd_dma(struct vmw_private *dev_priv,
		       struct vmw_sw_context *sw_context,
		       SVGA3dCmdHeader *header)
{
	struct vmw_dma_buffer *vmw_bo = NULL;
	struct vmw_surface *srf = NULL;
	struct vmw_dma_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdSurfaceDMA dma;
	} *cmd;
	int ret;
	SVGA3dCmdSurfaceDMASuffix *suffix;
	uint32_t bo_size;

	cmd = container_of(header, struct vmw_dma_cmd, header);
	suffix = (SVGA3dCmdSurfaceDMASuffix *)((unsigned long) &cmd->dma +
					       header->size - sizeof(*suffix));

	/* Make sure device and verifier stays in sync. */
	if (unlikely(suffix->suffixSize != sizeof(*suffix))) {
		DRM_ERROR("Invalid DMA suffix size.\n");
		return -EINVAL;
	}

	ret = vmw_translate_guest_ptr(dev_priv, sw_context,
				      &cmd->dma.guest.ptr,
				      &vmw_bo);
	if (unlikely(ret != 0))
		return ret;

	/* Make sure DMA doesn't cross BO boundaries. */
	bo_size = vmw_bo->base.num_pages * PAGE_SIZE;
	if (unlikely(cmd->dma.guest.ptr.offset > bo_size)) {
		DRM_ERROR("Invalid DMA offset.\n");
		return -EINVAL;
	}

	bo_size -= cmd->dma.guest.ptr.offset;
	if (unlikely(suffix->maximumOffset > bo_size))
		suffix->maximumOffset = bo_size;

	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				user_surface_converter, &cmd->dma.host.sid,
				NULL);
	if (unlikely(ret != 0)) {
		if (unlikely(ret != -ERESTARTSYS))
			DRM_ERROR("could not find surface for DMA.\n");
		goto out_no_surface;
	}

	srf = vmw_res_to_srf(sw_context->res_cache[vmw_res_surface].res);

	vmw_kms_cursor_snoop(srf, sw_context->fp->tfile, &vmw_bo->base,
			     header);

out_no_surface:
	vmw_dmabuf_unreference(&vmw_bo);
	return ret;
}

static int vmw_cmd_draw(struct vmw_private *dev_priv,
			struct vmw_sw_context *sw_context,
			SVGA3dCmdHeader *header)
{
	struct vmw_draw_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdDrawPrimitives body;
	} *cmd;
	SVGA3dVertexDecl *decl = (SVGA3dVertexDecl *)(
		(unsigned long)header + sizeof(*cmd));
	SVGA3dPrimitiveRange *range;
	uint32_t i;
	uint32_t maxnum;
	int ret;

	ret = vmw_cmd_cid_check(dev_priv, sw_context, header);
	if (unlikely(ret != 0))
		return ret;

	cmd = container_of(header, struct vmw_draw_cmd, header);
	maxnum = (header->size - sizeof(cmd->body)) / sizeof(*decl);

	if (unlikely(cmd->body.numVertexDecls > maxnum)) {
		DRM_ERROR("Illegal number of vertex declarations.\n");
		return -EINVAL;
	}

	for (i = 0; i < cmd->body.numVertexDecls; ++i, ++decl) {
		ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
					user_surface_converter,
					&decl->array.surfaceId, NULL);
		if (unlikely(ret != 0))
			return ret;
	}

	maxnum = (header->size - sizeof(cmd->body) -
		  cmd->body.numVertexDecls * sizeof(*decl)) / sizeof(*range);
	if (unlikely(cmd->body.numRanges > maxnum)) {
		DRM_ERROR("Illegal number of index ranges.\n");
		return -EINVAL;
	}

	range = (SVGA3dPrimitiveRange *) decl;
	for (i = 0; i < cmd->body.numRanges; ++i, ++range) {
		ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
					user_surface_converter,
					&range->indexArray.surfaceId, NULL);
		if (unlikely(ret != 0))
			return ret;
	}
	return 0;
}


static int vmw_cmd_tex_state(struct vmw_private *dev_priv,
			     struct vmw_sw_context *sw_context,
			     SVGA3dCmdHeader *header)
{
	struct vmw_tex_state_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdSetTextureState state;
	} *cmd;

	SVGA3dTextureState *last_state = (SVGA3dTextureState *)
	  ((unsigned long) header + header->size + sizeof(*header));
	SVGA3dTextureState *cur_state = (SVGA3dTextureState *)
		((unsigned long) header + sizeof(struct vmw_tex_state_cmd));
	struct vmw_resource_val_node *ctx_node;
	struct vmw_resource_val_node *res_node;
	int ret;

	cmd = container_of(header, struct vmw_tex_state_cmd,
			   header);

	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_context,
				user_context_converter, &cmd->state.cid,
				&ctx_node);
	if (unlikely(ret != 0))
		return ret;

	for (; cur_state < last_state; ++cur_state) {
		if (likely(cur_state->name != SVGA3D_TS_BIND_TEXTURE))
			continue;

		if (cur_state->stage >= SVGA3D_NUM_TEXTURE_UNITS) {
			DRM_ERROR("Illegal texture/sampler unit %u.\n",
				  (unsigned) cur_state->stage);
			return -EINVAL;
		}

		ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
					user_surface_converter,
					&cur_state->value, &res_node);
		if (unlikely(ret != 0))
			return ret;

		if (dev_priv->has_mob) {
			struct vmw_ctx_bindinfo_tex binding;

			binding.bi.ctx = ctx_node->res;
			binding.bi.res = res_node ? res_node->res : NULL;
			binding.bi.bt = vmw_ctx_binding_tex;
			binding.texture_stage = cur_state->stage;
			vmw_binding_add(ctx_node->staged_bindings, &binding.bi,
					0, binding.texture_stage);
		}
	}

	return 0;
}

static int vmw_cmd_check_define_gmrfb(struct vmw_private *dev_priv,
				      struct vmw_sw_context *sw_context,
				      void *buf)
{
	struct vmw_dma_buffer *vmw_bo;
	int ret;

	struct {
		uint32_t header;
		SVGAFifoCmdDefineGMRFB body;
	} *cmd = buf;

	ret = vmw_translate_guest_ptr(dev_priv, sw_context,
				      &cmd->body.ptr,
				      &vmw_bo);
	if (unlikely(ret != 0))
		return ret;

	vmw_dmabuf_unreference(&vmw_bo);

	return ret;
}


/**
 * vmw_cmd_res_switch_backup - Utility function to handle backup buffer
 * switching
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @val_node: The validation node representing the resource.
 * @buf_id: Pointer to the user-space backup buffer handle in the command
 * stream.
 * @backup_offset: Offset of backup into MOB.
 *
 * This function prepares for registering a switch of backup buffers
 * in the resource metadata just prior to unreserving. It's basically a wrapper
 * around vmw_cmd_res_switch_backup with a different interface.
 */
static int vmw_cmd_res_switch_backup(struct vmw_private *dev_priv,
				     struct vmw_sw_context *sw_context,
				     struct vmw_resource_val_node *val_node,
				     uint32_t *buf_id,
				     unsigned long backup_offset)
{
	struct vmw_dma_buffer *dma_buf;
	int ret;

	ret = vmw_translate_mob_ptr(dev_priv, sw_context, buf_id, &dma_buf);
	if (ret)
		return ret;

	val_node->switching_backup = true;
	if (val_node->first_usage)
		val_node->no_buffer_needed = true;

	vmw_dmabuf_unreference(&val_node->new_backup);
	val_node->new_backup = dma_buf;
	val_node->new_backup_offset = backup_offset;

	return 0;
}


/**
 * vmw_cmd_switch_backup - Utility function to handle backup buffer switching
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The sof