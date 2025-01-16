verter,
				 &cmd->body.image.sid, NULL);
}

/**
 * vmw_cmd_readback_gb_surface - Validate an SVGA_3D_CMD_READBACK_GB_SURFACE
 * command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_readback_gb_surface(struct vmw_private *dev_priv,
				       struct vmw_sw_context *sw_context,
				       SVGA3dCmdHeader *header)
{
	struct vmw_gb_surface_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdReadbackGBSurface body;
	} *cmd;

	cmd = container_of(header, struct vmw_gb_surface_cmd, header);

	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				 user_surface_converter,
				 &cmd->body.sid, NULL);
}

/**
 * vmw_cmd_invalidate_gb_image - Validate an SVGA_3D_CMD_INVALIDATE_GB_IMAGE
 * command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_invalidate_gb_image(struct vmw_private *dev_priv,
				       struct vmw_sw_context *sw_context,
				       SVGA3dCmdHeader *header)
{
	struct vmw_gb_surface_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdInvalidateGBImage body;
	} *cmd;

	cmd = container_of(header, struct vmw_gb_surface_cmd, header);

	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				 user_surface_converter,
				 &cmd->body.image.sid, NULL);
}

/**
 * vmw_cmd_invalidate_gb_surface - Validate an
 * SVGA_3D_CMD_INVALIDATE_GB_SURFACE command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_invalidate_gb_surface(struct vmw_private *dev_priv,
					 struct vmw_sw_context *sw_context,
					 SVGA3dCmdHeader *header)
{
	struct vmw_gb_surface_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdInvalidateGBSurface body;
	} *cmd;

	cmd = container_of(header, struct vmw_gb_surface_cmd, header);

	return vmw_cmd_res_check(dev_priv, sw_context, vmw_res_surface,
				 user_surface_converter,
				 &cmd->body.sid, NULL);
}


/**
 * vmw_cmd_shader_define - Validate an SVGA_3D_CMD_SHADER_DEFINE
 * command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_shader_define(struct vmw_private *dev_priv,
				 struct vmw_sw_context *sw_context,
				 SVGA3dCmdHeader *header)
{
	struct vmw_shader_define_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdDefineShader body;
	} *cmd;
	int ret;
	size_t size;
	struct vmw_resource_val_node *val;

	cmd = container_of(header, struct vmw_shader_define_cmd,
			   header);

	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_context,
				user_context_converter, &cmd->body.cid,
				&val);
	if (unlikely(ret != 0))
		return ret;

	if (unlikely(!dev_priv->has_mob))
		return 0;

	size = cmd->header.size - sizeof(cmd->body);
	ret = vmw_compat_shader_add(dev_priv,
				    vmw_context_res_man(val->res),
				    cmd->body.shid, cmd + 1,
				    cmd->body.type, size,
				    &sw_context->staged_cmd_res);
	if (unlikely(ret != 0))
		return ret;

	return vmw_resource_relocation_add(&sw_context->res_relocations,
					   NULL, &cmd->header.id -
					   sw_context->buf_start);

	return 0;
}

/**
 * vmw_cmd_shader_destroy - Validate an SVGA_3D_CMD_SHADER_DESTROY
 * command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_shader_destroy(struct vmw_private *dev_priv,
				  struct vmw_sw_context *sw_context,
				  SVGA3dCmdHeader *header)
{
	struct vmw_shader_destroy_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdDestroyShader body;
	} *cmd;
	int ret;
	struct vmw_resource_val_node *val;

	cmd = container_of(header, struct vmw_shader_destroy_cmd,
			   header);

	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_context,
				user_context_converter, &cmd->body.cid,
				&val);
	if (unlikely(ret != 0))
		return ret;

	if (unlikely(!dev_priv->has_mob))
		return 0;

	ret = vmw_shader_remove(vmw_context_res_man(val->res),
				cmd->body.shid,
				cmd->body.type,
				&sw_context->staged_cmd_res);
	if (unlikely(ret != 0))
		return ret;

	return vmw_resource_relocation_add(&sw_context->res_relocations,
					   NULL, &cmd->header.id -
					   sw_context->buf_start);

	return 0;
}

/**
 * vmw_cmd_set_shader - Validate an SVGA_3D_CMD_SET_SHADER
 * command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_set_shader(struct vmw_private *dev_priv,
			      struct vmw_sw_context *sw_context,
			      SVGA3dCmdHeader *header)
{
	struct vmw_set_shader_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdSetShader body;
	} *cmd;
	struct vmw_resource_val_node *ctx_node, *res_node = NULL;
	struct vmw_ctx_bindinfo_shader binding;
	struct vmw_resource *res = NULL;
	int ret;

	cmd = container_of(header, struct vmw_set_shader_cmd,
			   header);

	if (cmd->body.type >= SVGA3D_SHADERTYPE_PREDX_MAX) {
		DRM_ERROR("Illegal shader type %u.\n",
			  (unsigned) cmd->body.type);
		return -EINVAL;
	}

	ret = vmw_cmd_res_check(dev_priv, sw_context, vmw_res_context,
				user_context_converter, &cmd->body.cid,
				&ctx_node);
	if (unlikely(ret != 0))
		return ret;

	if (!dev_priv->has_mob)
		return 0;

	if (cmd->body.shid != SVGA3D_INVALID_ID) {
		res = vmw_shader_lookup(vmw_context_res_man(ctx_node->res),
					cmd->body.shid,
					cmd->body.type);

		if (!IS_ERR(res)) {
			ret = vmw_cmd_res_reloc_add(dev_priv, sw_context,
						    &cmd->body.shid, res,
						    &res_node);
			vmw_resource_unreference(&res);
			if (unlikely(ret != 0))
				return ret;
		}
	}

	if (!res_node) {
		ret = vmw_cmd_res_check(dev_priv, sw_context,
					vmw_res_shader,
					user_shader_converter,
					&cmd->body.shid, &res_node);
		if (unlikely(ret != 0))
			return ret;
	}

	binding.bi.ctx = ctx_node->res;
	binding.bi.res = res_node ? res_node->res : NULL;
	binding.bi.bt = vmw_ctx_binding_shader;
	binding.shader_slot = cmd->body.type - SVGA3D_SHADERTYPE_MIN;
	vmw_binding_add(ctx_node->staged_bindings, &binding.bi,
			binding.shader_slot, 0);
	return 0;
}

/**
 * vmw_cmd_set_shader_const - Validate an SVGA_3D_CMD_SET_SHADER_CONST
 * command
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int vmw_cmd_set_shader_const(struct vmw_private *dev_priv,
				    struct vmw_sw_context *sw_context,
				    SVGA3dCmdHeader *header)
{
	struct vmw_set_shader_const_cmd {
		SVGA3dCmdHeader header;
		SVGA3dCmdSetShaderConst body;
	} *cmd;
	int ret;

	cmd = container_of(header, struct vmw_set_shader_con