be freed using either vmw_apply_relocations() or
 * vmw_free_relocations(). The latter needs to be freed using
 * vmw_clear_validations.
 */
static int vmw_translate_guest_ptr(struct vmw_private *dev_priv,
				   struct vmw_sw_context *sw_context,
				   SVGAGuestPtr *ptr,
				   struct vmw_dma_buffer **vmw_bo_p)
{
	struct vmw_dma_buffer *vmw_bo = NULL;
	uint32_t handle = ptr->gmrId;
	struct vmw_relocation *reloc;
	int ret;

	ret = vmw_user_dmabuf_lookup(sw_context->fp->tfile, handle, &vmw_bo,
				     NULL);
	if (unlikely(ret != 0)) {
		DRM_ERROR("Could not find or use GMR region.\n");
		ret = -EINVAL;
		goto out_no_reloc;
	}

	if (unlikely(sw_context->cur_reloc >= VMWGFX_MAX_RELOCATIONS)) {
		DRM_ERROR("Max number relocations per submission"
			  " exceeded\n");
		ret = -EINVAL;
		goto out_no_reloc;
	}

