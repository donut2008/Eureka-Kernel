TING_READ(VLV_IMR);
}

static void valleyview_display_irqs_uninstall(struct drm_i915_private *dev_priv)
{
	u32 pipestat_mask;
	u32 iir_mask;
	enum pipe pipe;

	iir_mask = I915_DISPLAY_PORT_INTERRUPT |
		   I915_DISPLAY_PIPE_A_EVENT_INTERRUPT |
		   I915_DISPLAY_PIPE_B_EVENT_INTERRUPT;
	if (IS_CHERRYVIEW(dev_priv))
		iir_mask |= I915_DISPLAY_PIPE_C_EVENT_INTERRUPT;

	dev_priv->irq_mask |= iir_mask;
	I915_WRITE(VLV_IMR, dev_priv->irq_mask);
	I915_WRITE(VLV_IER, ~dev_priv->irq_mask);
	I915_WRITE(VLV_IIR, iir_mask);
	I915_WRITE(VLV_IIR, iir_mask);
	POSTING_READ(VLV_IIR);

	pipestat_mask = PLANE_FLIP_DONE_INT_STATUS_VLV |
			PIPE_CRC_DONE_INTERRUPT_STATUS;

	i915_disable_pipestat(dev_priv, PIPE_A, PIPE_GMBUS_INTERRUPT_STATUS);
	for_each_pipe(dev_priv, pipe)
		i915_disable_pipestat(dev_priv, pipe, pipestat_mask);

	pipestat_mask = PIPESTAT_INT_STATUS_MASK |
			PIPE_FIFO_UNDERRUN_STATUS;

	for_each_pipe(dev_priv, pipe)
		I915_WRITE(PIPESTAT(pipe), pipestat_mask);
	POSTING_READ(PIPESTAT(PIPE_A));
}

void valleyview_enable_display_irqs(struct drm_i915_private *dev_priv)
{
	assert_spin_locked(&dev_priv->irq_lock);

	if (dev_priv->display_irqs_enabled)
		return;

	dev_priv->display_irqs_enabled = true;

	if (intel_irqs_enabled(dev_priv))
		valleyview_display_irqs_install(dev_priv);
}

void valleyview_disable_display_