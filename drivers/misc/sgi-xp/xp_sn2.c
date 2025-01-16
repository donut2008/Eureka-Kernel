_REG];
	u32 eir = I915_READ(EIR);
	int pipe, i;

	if (!eir)
		return;

	pr_err("render error detected, EIR: 0x%08x\n", eir);

	i915_get_extra_instdone(dev, instdone);

	if (IS_G4X(dev)) {
		if (eir & (GM45_ERROR_MEM_PRIV | GM45_ERROR_CP_PRIV)) {
			u32 ipeir = I915_READ(IPEIR_I965);

			pr_err("  IPEIR: 0x%08x\n", I915_READ(IPEIR_I965));
			pr_err("  IPEHR: 0x%08x\n", I915_READ(IPEHR_I965));
			for (i = 0; i < ARRAY_SIZE(instdone); i++)
				pr_err("  INSTDONE_%d: 0x%08x\n", i, instdone[i]);
			pr_err("  INSTPS: 0x%08x\n", I915_READ(INSTPS));
			pr_err("  ACTHD: 0x%08x\n", I915_READ(ACTHD_I965));
			I915_WRITE(IPEIR_I965, ipeir);
			POSTING_READ(IPEIR_I965);
		}
		if (eir & GM45_ERROR_PAGE_TABLE) {
			u32 pgtbl_err = I915_READ(PGTBL_ER);
			pr_err("page table error\n");
			pr_err("  PGTBL_ER: 0x%08x\n", pgtbl_err);
			I915_WRITE(PGTBL_ER, pgtbl_err);
			POSTING_READ(PGTBL_ER);
		}
	}

	if (!IS_GEN2(dev)) {
		if (eir & I915_ERROR_PAGE_TABLE) {
			u32 pgtbl_err = I915_READ(PGTBL_ER);
			pr_err("page table error\n");
			pr_err("  PGTBL_ER: 0x%08x\n", pgtbl_err);
			I915_WRITE(PGTBL_ER, pgtbl_err);
			POSTING_READ(PGTBL_ER);
		}
	}

	if (eir & I915_ERROR_MEMORY_REFRESH) {
		pr_err("memory refresh error:\n");
		for_each_pipe(dev_priv, pipe)
			pr_err("pipe %c stat: 0x%08x\n",
			       pipe_name(pipe), I915_READ(PIPESTAT(pipe)));
		/* pipestat has already been acked */
	}
	if (eir & I915_ERROR_INSTRUCTION) {
		pr_err("instruction error\n");
		pr_err("  INSTPM: 0x%08x\n", I915_READ(INSTPM));
		for (i = 0; i < ARRAY_SIZE(instdone); i++)
			pr_err("  INSTDONE_%d: 0x%08x\n", i, instdone[i]);
		if (INTEL_INFO(dev)->gen < 4) {
			u32 ipeir = I915_READ(IPEIR);

			pr_err("  IPEIR: 0x%08x\n", I915_READ(IPEIR));
			pr_err("  IPEHR: 0x%08x\n", I915_READ(IPEHR));
			pr_err("  ACTHD: 0x%08x\n", I915_READ(ACTHD));
			I915_WRITE(IPEIR, ipeir);
			POSTING_READ(IPEIR);
		} else {
			u32 ipeir = I915_READ(IPEIR_I965);

			pr_err("  IPEIR: 0x%08x\n", I915_READ(IPEIR_I965));
			pr_err("  IPEHR: 0x%08x\n", I915_READ(IPEHR_I965));
			pr_err("  INSTPS: 0x%08x\n", I915_READ(INSTPS));
			pr_err("  ACTHD: 0x%08x\n", I915_READ(ACTHD_I965));
			I915_WRITE(IPEIR_I965, ipeir);
			POSTING_READ(IPEIR_I965);
		}
	}

	I915_WRITE(EIR, eir);
	POSTING_READ(EIR);
	eir = I915_READ(EIR);
	if (eir) {
		/*
		 * some errors might have become stuck,
		 * mask them.
		 */
		DRM_ERROR("EIR stuck: 0x%08x, masking\n", eir);
		I915_WRITE(EMR, I915_READ(EMR) | eir);
		I915_WRITE(IIR, I915_RENDER_COMMAND_PARSER_ERROR_INTERRUPT);
	}
}

/**
 * i915_handle_error - handle a gpu error
 * @dev: drm device
 *
 * Do some basic checking of register state at error time and
 * dump it to the syslog.  Also call i915_capture_error_state() to make
 * sure we get a record and make it available in debugfs.  Fire a uevent
 * so userspace knows something bad happened (should trigger collection
 * of a ring dump etc.).
 */
void i915_handle_error(struct drm_device *dev, bool wedged,
		       const char *fmt, ...)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	va_list args;
	char error_msg[80];

	va_start(args, fmt);
	vscnprintf(error_msg, sizeof(error_msg), fmt, args);
	va_end(args);

	i915_capture_error_state(dev, wedged, error_msg);
	i915_report_and_clear_eir(dev);

	if (wedged) {
		atomic_or(I915_RESET_IN_PROGRESS_FLAG,
				&dev_priv->gpu_error.reset_counter);

		/*
		 * Wakeup waiting processes so that the reset function
		 * i915_reset_and_wakeup doesn't deadlock trying to grab
		 * various locks. By bumping the reset counter first, the woken
		 * processes will see a reset in progress and back off,
		 * releasing their locks and then wait for the reset completion.
		 * We must do this for _all_ gpu waiters that might hold locks
		 * that the reset work needs to acquire.
		 *
		 * Note: The wake_up serves as the required memory barrier to
		 * ensure that the waiters see the updated value of the reset
		 * counter atomic_t.
		 */
		i915_error_wake_up(dev_priv, false);
	}

	i915_reset_and_wakeup(dev);
}

/* Called from drm generic code, passed 'crtc' which
 * we use as a pipe index
 */
static int i915_enable_vblank(struct drm_device *dev, unsigned int pipe)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	unsigned long irqflags;

	spin_lock_irqsave(&dev_priv->irq_lock, irqflags);
	if (INTEL_INFO(dev)->gen >= 4)
		i915_enable_pipestat(dev_priv, pipe,
				     PIPE_START_VBLANK_INTERRUPT_STATUS);
	else
		i915_enable_pipestat(dev_priv, pipe,
				     PIPE_VBLANK_INTERRUPT_STATUS);
	spin_unlock_irqrestore(&dev_priv->irq_lock, irqflags);

	return 0;
}

static int ironlake_enable_vblank(struct drm_device *dev, unsigned int pipe)
{
	struct drm_i91