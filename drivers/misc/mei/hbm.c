 = cmd->body.type - SVGA3D_SHADERTYPE_MIN;

	vmw_binding_add(ctx_node->staged_bindings, &binding.bi,
			binding.shader_slot, 0);
out_unref:
	if (res)
		vmw_resource_unreference(&res);

	return ret;
}

/**
 * vmw_cmd_dx_set_vertex_buffers - Validates an
 * SVGA_3D_CMD_DX_SET_VERTEX_BUFFERS command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_dx_set_vertex_buffers(struct vmw_private *dev_priv,
					 struct vmw_sw_context *sw_context,
					 SVGA3dCmdHeader *header)
{
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;
	struct vmw_ctx_bindinfo_vb binding;
	struct vmw_resource_val_node *res_node;
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXSetVertexBuffers body;
		SVGA3dVertexBuffer buf[];
	} *cmd;
	int i, ret, num;

	if (unlikely(ctx_node == NULL)) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	cmd = container_of(header, typeof(*cmd), header);
	num = (cmd->header.size - sizeof(cmd->body)) /
		sizeof(SVGA3dVertexBuffer);
	if ((u64)num + (u64)cmd->body.startBuffer >
	    (u64)SVGA3D_DX_MAX_VERTEXBUFFERS) {
		DRM_ERROR("Invalid number of vertex buffers.\n");
		return -EINVAL;
	}

	for (i = 0; i < num; i++) {
		ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
					user_surface_converter,
					&cmd->buf[i].sid, &res_node);
		if (unlikely(ret != 0))
			return ret;

		binding.bi.ctx = ctx_node->res;
		binding.bi.bt = vmw_ctx_binding_vb;
		binding.bi.res = ((res_node) ? res_node->res : NULL);
		binding.offset = cmd->buf[i].offset;
		binding.stride = cmd->buf[i].stride;
		binding.slot = i + cmd->body.startBuffer;

		vmw_binding_add(ctx_node->staged_bindings, &binding.bi,
				0, binding.slot);
	}

	return 0;
}

/**
 * vmw_cmd_dx_ia_set_vertex_buffers - Validate an
 * SVGA_3D_CMD_DX_IA_SET_VERTEX_BUFFERS command.
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_dx_set_index_buffer(struct vmw_private *dev_priv,
				       struct vmw_sw_context *sw_context,
				       SVGA3dCmdHeader *header)
{
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;
	struct vmw_ctx_bindinfo_ib binding;
	struct vmw_resource_val_node *res_node;
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXSetIndexBuffer body;
	} *cmd;
	int ret;

	if (unlikely(ctx_node == NULL)) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	cmd = container_of(header, typeof(*cmd), header);
	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				user_surface_converter,
				&cmd->body.sid, &res_node);
	if (unlikely(ret != 0))
		return ret;

	binding.bi.ctx = ctx_node->res;
	binding.bi.res = ((res_node) ? res_node->res : NULL);
	binding.bi.bt = vmw_ctx_binding_ib;
	binding.offset = cmd->body.offset;
	binding.format = cmd->body.format;

	vmw_binding_add(ctx_node->staged_bindings, &binding.bi, 0, 0);

	return 0;
}

/**
 * vmw_cmd_dx_set_rendertarget - Validate an
 * SVGA_3D_CMD_DX_SET_RENDERTARGETS command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_dx_set_rendertargets(struct vmw_private *dev_priv,
					struct vmw_sw_context *sw_context,
					SVGA3dCmdHeader *header)
{
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXSetRenderTargets body;
	} *cmd = container_of(header, typeof(*cmd), header);
	int ret;
	u32 num_rt_view = (cmd->header.size - sizeof(cmd->body)) /
		sizeof(SVGA3dRenderTargetViewId);

	if (num_rt_view > SVGA3D_MAX_SIMULTANEOUS_RENDER_TARGETS) {
		DRM_ERROR("Invalid DX Rendertarget binding.\n");
		return -EINVAL;
	}

	ret = vmw_view_bindings_add(sw_context, vmw_view_ds,
				    vmw_ctx_binding_ds, 0,
				    &cmd->body.depthStencilViewId, 1, 0);
	if (ret)
		return ret;

	return vmw_view_bindings_add(sw_context, vmw_view_rt,
				     vmw_ctx_binding_dx_rt, 0,
				     (void *)&cmd[1], num_rt_view, 0);
}

/**
 * vmw_cmd_dx_clear_rendertarget_view - Validate an
 * SVGA_3D_CMD_DX_CLEAR_RENDERTARGET_VIEW command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_dx_clear_rendertarget_view(struct vmw_private *dev_priv,
					      struct vmw_sw_context *sw_context,
					      SVGA3dCmdHeader *header)
{
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXClearRenderTargetView body;
	} *cmd = container_of(header, typeof(*cmd), header);

	return vmw_view_id_val_add(sw_context, vmw_view_rt,
				   cmd->body.renderTargetViewId);
}

/**
 * vmw_cmd_dx_clear_rendertarget_view - Validate an
 * SVGA_3D_CMD_DX_CLEAR_DEPTHSTENCIL_VIEW command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_dx_clear_depthstencil_view(struct vmw_private *dev_priv,
					      struct vmw_sw_context *sw_context,
					      SVGA3dCmdHeader *header)
{
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXClearDepthStencilView body;
	} *cmd = container_of(header, typeof(*cmd), header);

	return vmw_view_id_val_add(sw_context, vmw_view_ds,
				   cmd->body.depthStencilViewId);
}

static int vmw_cmd_dx_view_define(struct vmw_private *dev_priv,
				  struct vmw_sw_context *sw_context,
				  SVGA3dCmdHeader *header)
{
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;
	struct vmw_resource_val_node *srf_node;
	struct vmw_resource *res;
	enum vmw_view_type view_type;
	int ret;
	/*
	 * This is based on the fact that all affected define commands have
	 * the same initial command body layout.
	 */
	struct {
		SVGA3dCmdHeader header;
		uint32 defined_id;
		uint32 sid;
	} *cmd;

	if (unlikely(ctx_node == NULL)) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	view_type = vmw_view_cmd_to_type(header->id);
	if (view_type == vmw_view_max)
		return -EINVAL;
	cmd = container_of(header, typeof(*cmd), header);
	if (unlikely(cmd->sid == SVGA3D_INVALID_ID)) {
		DRM_ERROR("Invalid surface id.\n");
		return -EINVAL;
	}
	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				user_surface_converter,
				&cmd->sid, &srf_node);
	if (unlikely(ret != 0))
		return ret;

	res = vmw_context_cotable(ctx_node->res, vmw_view_cotables[view_type]);
	ret = vmw_cotable_notify(res, cmd->defined_id);
	vmw_resource_unreference(&res);
	if (unlikely(ret != 0))
		return ret;

	return vmw_view_add(sw_context->man,
			    ctx_node->res,
			    srf_node->res,
			    view_type,
			    cmd->defined_id,
			    header,
			    header->size + sizeof(*header),
			    &sw_context->staged_cmd_res);
}

/**
 * vmw_cmd_dx_set_so_targets - Validate an
 * SVGA_3D_CMD_DX_SET_SOTARGETS command.
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_dx_set_so_targets(struct vmw_private *dev_priv,
				     struct vmw_sw_context *sw_context,
				     SVGA3dCmdHeader *header)
{
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;
	struct vmw_ctx_bindinfo_so binding;
	struct vmw_resource_val_node *res_node;
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXSetSOTargets body;
		SVGA3dSoTarget targets[];
	} *cmd;
	int i, ret, num;

	if (unlikely(ctx_node == NULL)) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	cmd = container_of(header, typeof(*cmd), header);
	num = (cmd->header.size - sizeof(cmd->body)) /
		sizeof(SVGA3dSoTarget);

	if (num > SVGA3D_DX_MAX_SOTARGETS) {
		DRM_ERROR("Invalid DX SO binding.\n");
		return -EINVAL;
	}

	for (i = 0; i < num; i++) {
		ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
					user_surface_converter,
					&cmd->targets[i].sid, &res_node);
		if (unlikely(ret != 0))
			return ret;

		binding.bi.ctx = ctx_node->res;
		binding.bi.res = ((res_node) ? res_node->res : NULL);
		binding.bi.bt = vmw_ctx_binding_so,
		binding.offset = cmd->targets[i].offset;
		binding.size = cmd->targets[i].sizeInBytes;
		binding.slot = i;

		vmw_binding_add(ctx_node->staged_bindings, &binding.bi,
				0, binding.slot);
	}

	return 0;
}

static int vmw_cmd_dx_so_define(struct vmw_private *dev_priv,
				struct vmw_sw_context *sw_context,
				SVGA3dCmdHeader *header)
{
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;
	struct vmw_resource *res;
	/*
	 * This is based on the fact that all affected define commands have
	 * the same initial command body layout.
	 */
	struct {
		SVGA3dCmdHeader header;
		uint32 defined_id;
	} *cmd;
	enum vmw_so_type so_type;
	int ret;

	if (unlikely(ctx_node == NULL)) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	so_type = vmw_so_cmd_to_type(header->id);
	res = vmw_context_cotable(ctx_node->res, vmw_so_cotables[so_type]);
	cmd = container_of(header, typeof(*cmd), header);
	ret = vmw_cotable_notify(res, cmd->defined_id);
	vmw_resource_unreference(&res);

	return ret;
}

/**
 * vmw_cmd_dx_check_subresource - Validate an
 * SVGA_3D_CMD_DX_[X]_SUBRESOURCE command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_dx_check_subresource(struct vmw_private *dev_priv,
					struct vmw_sw_context *sw_context,
					SVGA3dCmdHeader *header)
{
	struct {
		SVGA3dCmdHeader header;
		union {
			SVGA3dCmdDXReadbackSubResource r_body;
			SVGA3dCmdDXInvalidateSubResource i_body;
			SVGA3dCmdDXUpdateSubResource u_body;
			SVGA3dSurfaceId sid;
		};
	} *cmd;

	BUILD_BUG_ON(offsetof(typeof(*cmd), r_body.sid) !=
		     offsetof(typeof(*cmd), sid));
	BUILD_BUG_ON(offsetof(typeof(*cmd), i_body.sid) !=
		     offsetof(typeof(*cmd), sid));
	BUILD_BUG_ON(offsetof(typeof(*cmd), u_body.sid) !=
		     offsetof(typeof(*cmd), sid));

	cmd = container_of(header, typeof(*cmd), header);

	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				 user_surface_converter,
				 &cmd->sid, NULL);
}

static int vmw_cmd_dx_cid_check(struct vmw_private *dev_priv,
				struct vmw_sw_context *sw_context,
				SVGA3dCmdHeader *header)
{
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;

	if (unlikely(ctx_node == NULL)) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	return 0;
}

/**
 * vmw_cmd_dx_view_remove - validate a view remove command and
 * schedule the view resource for removal.
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 *
 * Check that the view exists, and if it was not created using this
 * command batch, make sure it's validated (present in the device) so that
 * the remove command will not confuse the device.
 */
static int vmw_cmd_dx_view_remove(struct vmw_private *dev_priv,
				  struct vmw_sw_context *sw_context,
				  SVGA3dCmdHeader *header)
{
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;
	struct {
		SVGA3dCmdHeader header;
		union vmw_view_destroy body;
	} *cmd = container_of(header, typeof(*cmd), header);
	enum vmw_view_type view_type = vmw_view_cmd_to_type(header->id);
	struct vmw_resource *view;
	int ret;

	if (!ctx_node) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	ret = vmw_view_remove(sw_context->man,
			      cmd->body.view_id, view_type,
			      &sw_context->staged_cmd_res,
			      &view);
	if (ret || !view)
		return ret;

	/*
	 * Add view to the validate list iff it was not created using this
	 * command batch.
	 */
	return vmw_view_res_val_add(sw_context, view);
}

/**
 * vmw_cmd_dx_define_shader - Validate an SVGA_3D_CMD_DX_DEFINE_SHADER
 * command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_dx_define_shader(struct vmw_private *dev_priv,
				    struct vmw_sw_context *sw_context,
				    SVGA3dCmdHeader *header)
{
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;
	struct vmw_resource *res;
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXDefineShader body;
	} *cmd = container_of(header, typeof(*cmd), header);
	int ret;

	if (!ctx_node) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	res = vmw_context_cotable(ctx_node->res, SVGA_COTABLE_DXSHADER);
	ret = vmw_cotable_notify(res, cmd->body.shaderId);
	vmw_resource_unreference(&res);
	if (ret)
		return ret;

	return vmw_dx_shader_add(sw_context->man, ctx_node->res,
				 cmd->body.shaderId, cmd->body.type,
				 &sw_context->staged_cmd_res);
}

/**
 * vmw_cmd_dx_destroy_shader - Validate an SVGA_3D_CMD_DX_DESTROY_SHADER
 * command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_dx_destroy_shader(struct vmw_private *dev_priv,
				     struct vmw_sw_context *sw_context,
				     SVGA3dCmdHeader *header)
{
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXDestroyShader body;
	} *cmd = container_of(header, typeof(*cmd), header);
	int ret;

	if (!ctx_node) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	ret = vmw_shader_remove(sw_context->man, cmd->body.shaderId, 0,
				&sw_context->staged_cmd_res);
	if (ret)
		DRM_ERROR("Could not find shader to remove.\n");

	return ret;
}

/**
 * vmw_cmd_dx_bind_shader - Validate an SVGA_3D_CMD_DX_BIND_SHADER
 * command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_dx_bind_shader(struct vmw_private *dev_priv,
				  struct vmw_sw_context *sw_context,
				  SVGA3dCmdHeader *header)
{
	struct vmw_resource_val_node *ctx_node;
	struct vmw_resource_val_node *res_node;
	struct vmw_resource *res;
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXBindShader body;
	} *cmd = container_of(header, typeof(*cmd), header);
	int ret;

	if (cmd->body.cid != SVGA3D_INVALID_ID) {
		ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_context,
					user_context_converter,
					&cmd->body.cid, &ctx_node);
		if (ret)
			return ret;
	} else {
		ctx_node = sw_context->dx_ctx_node;
		if (!ctx_node) {
			DRM_ERROR("DX Context not set.\n");
			return -EINVAL;
		}
	}

	res = vmw_shader_lookup(vmw_context_res_man(ctx_node->res),
				cmd->body.shid, 0);
	if (IS_ERR(res)) {
		DRM_ERROR("Could not find shader to bind.\n");
		return PTR_ERR(res);
	}

	ret = vmw_resource_val_add(sw_context, res, &res_node);
	if (ret) {
		DRM_ERROR("Error creating resource validation node.\n");
		goto out_unref;
	}


	ret = vmw_cmd_res_switch_backup(dev_priv, sw_context, res_node,
					&cmd->body.mobid,
					cmd->body.offsetInBytes);
out_unref:
	vmw_resource_unreference(&res);

	return ret;
}

static int vmw_cmd_check_not_3d(struct vmw_private *dev_priv,
				struct vmw_sw_context *sw_context,
				void *buf, uint32_t *size)
{
	uint32_t size_remaining = *size;
	uint32_t cmd_id;

	cmd_id = ((uint32_t *)buf)[0];
	switch (cmd_id) {
	case SVGA_CMD_UPDATE:
		*size = sizeof(uint32_t) + sizeof(SVGAFifoCmdUpdate);
		break;
	case SVGA_CMD_DEFINE_GMRFB:
		*size = sizeof(uint32_t) + sizeof(SVGAFifoCmdDefineGMRFB);
		break;
	case SVGA_CMD_BLIT_GMRFB_TO_SCREEN:
		*size = sizeof(uint32_t) + sizeof(SVGAFifoCmdBlitGMRFBToScreen);
		break;
	case SVGA_CMD_BLIT_SCREEN_TO_GMRFB:
		*size = sizeof(uint32_t) + sizeof(SVGAFifoCmdBlitGMRFBToScreen);
		break;
	default:
		DRM_ERROR("Unsupported SVGA command: %u.\n", cmd_id);
		return -EINVAL;
	}

	if (*size > size_remaining) {
		DRM_ERROR("Invalid SVGA command (size mismatch):"
			  " %u.\n", cmd_id);
		return -EINVAL;
	}

	if (unlikely(!sw_context->kernel)) {
		DRM_ERROR("Kernel only SVGA command: %u.\n", cmd_id);
		return -EPERM;
	}

	if (cmd_id == SVGA_CMD_DEFINE_GMRFB)
		return vmw_cmd_check_define_gmrfb(dev_priv, sw_context, buf);

	return 0;
}

static const struct vmw_cmd_entry vmw_cmd_entries[SVGA_3D_CMD_MAX] = {
	VMW_CMD_DEF(SVGA_3D_CMD_SURFACE_DEFINE, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SURFACE_DESTROY, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SURFACE_COPY, &vmw_cmd_surface_copy_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SURFACE_STRETCHBLT, &vmw_cmd_stretch_blt_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SURFACE_DMA, &vmw_cmd_dma,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_CONTEXT_DEFINE, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_CONTEXT_DESTROY, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SETTRANSFORM, &vmw_cmd_cid_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SETZRANGE, &vmw_cmd_cid_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SETRENDERSTATE, &vmw_cmd_cid_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SETRENDERTARGET,
		    &vmw_cmd_set_render_target_check, true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SETTEXTURESTATE, &vmw_cmd_tex_state,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SETMATERIAL, &vmw_cmd_cid_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SETLIGHTDATA, &vmw_cmd_cid_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SETLIGHTENABLED, &vmw_cmd_cid_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SETVIEWPORT, &vmw_cmd_cid_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SETCLIPPLANE, &vmw_cmd_cid_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_CLEAR, &vmw_cmd_cid_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_PRESENT, &vmw_cmd_present_check,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SHADER_DEFINE, &vmw_cmd_shader_define,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SHADER_DESTROY, &vmw_cmd_shader_destroy,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SET_SHADER, &vmw_cmd_set_shader,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SET_SHADER_CONST, &vmw_cmd_set_shader_const,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_DRAW_PRIMITIVES, &vmw_cmd_draw,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SETSCISSORRECT, &vmw_cmd_cid_check,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_BEGIN_QUERY, &vmw_cmd_begin_query,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_END_QUERY, &vmw_cmd_end_query,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_WAIT_FOR_QUERY, &vmw_cmd_wait_query,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_PRESENT_READBACK, &vmw_cmd_ok,
		    true, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN,
		    &vmw_cmd_blt_surf_screen_check, false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SURFACE_DEFINE_V2, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_GENERATE_MIPMAPS, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_ACTIVATE_SURFACE, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_DEACTIVATE_SURFACE, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SCREEN_DMA, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SET_UNITY_SURFACE_COOKIE, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_OPEN_CONTEXT_SURFACE, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_LOGICOPS_BITBLT, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_LOGICOPS_TRANSBLT, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_LOGICOPS_STRETCHBLT, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_LOGICOPS_COLORFILL, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_LOGICOPS_ALPHABLEND, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_LOGICOPS_CLEARTYPEBLEND, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_SET_OTABLE_BASE, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_READBACK_OTABLE, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DEFINE_GB_MOB, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DESTROY_GB_MOB, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_REDEFINE_GB_MOB64, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_UPDATE_GB_MOB_MAPPING, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DEFINE_GB_SURFACE, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DESTROY_GB_SURFACE, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_BIND_GB_SURFACE, &vmw_cmd_bind_gb_surface,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_COND_BIND_GB_SURFACE, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_UPDATE_GB_IMAGE, &vmw_cmd_update_gb_image,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_UPDATE_GB_SURFACE,
		    &vmw_cmd_update_gb_surface, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_READBACK_GB_IMAGE,
		    &vmw_cmd_readback_gb_image, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_READBACK_GB_SURFACE,
		    &vmw_cmd_readback_gb_surface, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_INVALIDATE_GB_IMAGE,
		    &vmw_cmd_invalidate_gb_image, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_INVALIDATE_GB_SURFACE,
		    &vmw_cmd_invalidate_gb_surface, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DEFINE_GB_CONTEXT, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DESTROY_GB_CONTEXT, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_BIND_GB_CONTEXT, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_READBACK_GB_CONTEXT, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_INVALIDATE_GB_CONTEXT, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DEFINE_GB_SHADER, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_BIND_GB_SHADER, &vmw_cmd_bind_gb_shader,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DESTROY_GB_SHADER, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_SET_OTABLE_BASE64, &vmw_cmd_invalid,
		    false, false, false),
	VMW_CMD_DEF(SVGA_3D_CMD_BEGIN_GB_QUERY, &vmw_cmd_begin_gb_query,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_END_GB_QUERY, &vmw_cmd_end_gb_query,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_WAIT_FOR_GB_QUERY, &vmw_cmd_wait_gb_query,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_NOP, &vmw_cmd_ok,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_ENABLE_GART, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DISABLE_GART, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_MAP_MOB_INTO_GART, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_UNMAP_GART_RANGE, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DEFINE_GB_SCREENTARGET, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DESTROY_GB_SCREENTARGET, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_BIND_GB_SCREENTARGET, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_UPDATE_GB_SCREENTARGET, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_READBACK_GB_IMAGE_PARTIAL, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_INVALIDATE_GB_IMAGE_PARTIAL, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_SET_GB_SHADERCONSTS_INLINE, &vmw_cmd_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_GB_SCREEN_DMA, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_BIND_GB_SURFACE_WITH_PITCH, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_GB_MOB_FENCE, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DEFINE_GB_SURFACE_V2, &vmw_cmd_invalid,
		    false, false, true),

	/*
	 * DX commands
	 */
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_CONTEXT, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_CONTEXT, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_BIND_CONTEXT, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_READBACK_CONTEXT, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_INVALIDATE_CONTEXT, &vmw_cmd_invalid,
		    false, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_SINGLE_CONSTANT_BUFFER,
		    &vmw_cmd_dx_set_single_constant_buffer, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_SHADER_RESOURCES,
		    &vmw_cmd_dx_set_shader_res, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_SHADER, &vmw_cmd_dx_set_shader,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_SAMPLERS, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DRAW, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DRAW_INDEXED, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DRAW_INSTANCED, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DRAW_INDEXED_INSTANCED,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DRAW_AUTO, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_VERTEX_BUFFERS,
		    &vmw_cmd_dx_set_vertex_buffers, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_INDEX_BUFFER,
		    &vmw_cmd_dx_set_index_buffer, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_RENDERTARGETS,
		    &vmw_cmd_dx_set_rendertargets, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_BLEND_STATE, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_DEPTHSTENCIL_STATE,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_RASTERIZER_STATE,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_QUERY, &vmw_cmd_dx_define_query,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_QUERY, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_BIND_QUERY, &vmw_cmd_dx_bind_query,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_QUERY_OFFSET,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_BEGIN_QUERY, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_END_QUERY, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_READBACK_QUERY, &vmw_cmd_invalid,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_PREDICATION, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_VIEWPORTS, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_SCISSORRECTS, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_CLEAR_RENDERTARGET_VIEW,
		    &vmw_cmd_dx_clear_rendertarget_view, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_CLEAR_DEPTHSTENCIL_VIEW,
		    &vmw_cmd_dx_clear_depthstencil_view, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_PRED_COPY, &vmw_cmd_invalid,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_GENMIPS, &vmw_cmd_invalid,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_UPDATE_SUBRESOURCE,
		    &vmw_cmd_dx_check_subresource, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_READBACK_SUBRESOURCE,
		    &vmw_cmd_dx_check_subresource, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_INVALIDATE_SUBRESOURCE,
		    &vmw_cmd_dx_check_subresource, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_SHADERRESOURCE_VIEW,
		    &vmw_cmd_dx_view_define, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_SHADERRESOURCE_VIEW,
		    &vmw_cmd_dx_view_remove, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_RENDERTARGET_VIEW,
		    &vmw_cmd_dx_view_define, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_RENDERTARGET_VIEW,
		    &vmw_cmd_dx_view_remove, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_VIEW,
		    &vmw_cmd_dx_view_define, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_DEPTHSTENCIL_VIEW,
		    &vmw_cmd_dx_view_remove, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_ELEMENTLAYOUT,
		    &vmw_cmd_dx_so_define, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_ELEMENTLAYOUT,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_BLEND_STATE,
		    &vmw_cmd_dx_so_define, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_BLEND_STATE,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_STATE,
		    &vmw_cmd_dx_so_define, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_DEPTHSTENCIL_STATE,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_RASTERIZER_STATE,
		    &vmw_cmd_dx_so_define, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_RASTERIZER_STATE,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_SAMPLER_STATE,
		    &vmw_cmd_dx_so_define, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_SAMPLER_STATE,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_SHADER,
		    &vmw_cmd_dx_define_shader, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_SHADER,
		    &vmw_cmd_dx_destroy_shader, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_BIND_SHADER,
		    &vmw_cmd_dx_bind_shader, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DEFINE_STREAMOUTPUT,
		    &vmw_cmd_dx_so_define, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_DESTROY_STREAMOUTPUT,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_STREAMOUTPUT, &vmw_cmd_dx_cid_check,
		    true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_SOTARGETS,
		    &vmw_cmd_dx_set_so_targets, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_INPUT_LAYOUT,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_SET_TOPOLOGY,
		    &vmw_cmd_dx_cid_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_BUFFER_COPY,
		    &vmw_cmd_buffer_copy_check, true, false, true),
	VMW_CMD_DEF(SVGA_3D_CMD_DX_PRED_COPY_REGION,
		    &vmw_cmd_pred_copy_check, true, false, true),
};

static int vmw_cmd_check(struct vmw_private *dev_priv,
			 struct vmw_sw_context *sw_context,
			 void *buf, uint32_t *size)
{
	uint32_t cmd_id;
	uint32_t size_remaining = *size;
	SVGA3dCmdHeader *header = (SVGA3dCmdHeader *) buf;
	int ret;
	const struct vmw_cmd_entry *entry;
	bool gb = dev_priv->capabilities & SVGA_CAP_GBOBJECTS;

	cmd_id = ((uint32_t *)buf)[0];
	/* Handle any none 3D commands */
	if (unlikely(cmd_id < SVGA_CMD_MAX))
		return vmw_cmd_check_not_3d(dev