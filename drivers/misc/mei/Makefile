e an
 * SVGA_3D_CMD_DX_SET_SINGLE_CONSTANT_BUFFER command.
 *
 * @dev_priv: Pointer to a device private struct.
 * @sw_context: The software context being used for this batch.
 * @header: Pointer to the command header in the command stream.
 */
static int
vmw_cmd_dx_set_single_constant_buffer(struct vmw_private *dev_priv,
				      struct vmw_sw_context *sw_context,
				      SVGA3dCmdHeader *header)
{
	struct {
		SVGA3dCmdHeader header;
		SVGA3dCmdDXSetSingleConstantBuffer body;
	} *cmd;
	struct vmw_resource_val_node *res_node = NULL;
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;
	struct vmw_ctx_b