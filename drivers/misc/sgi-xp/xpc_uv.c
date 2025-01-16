pipe pipe;

	if (INTEL_INFO(dev_priv)->gen >= 9) {
		de_pipe_masked |= GEN9_PIPE_PLANE1_FLIP_DONE |
				  GEN9_DE_PIPE_IRQ_FAULT_ERRORS;
		de_port_masked |= GEN9_AUX_CHANNEL_B | GEN9_AUX_CHANNEL_C |
				  GEN9_AUX_CHANNEL_D;
		if (IS_BROXTON(dev_priv))
			de_port_masked |= BXT_DE_PORT_GMBUS;
	} else {
		de_pipe_masked |= GEN8_PIPE_PRIMARY_FLIP_DONE |
				  GEN8_DE_PIPE_IRQ_FAULT_ERRORS;
	}

	de_pipe_enables = de_pipe_masked | GEN8_PIPE_VBLANK |
					   GEN8_PIPE_FIFO_UNDERRUN;

	de_port_enables = de_port_masked;
	if (IS_BROXTON(dev_priv))
		de_port_enables |= BXT_DE_PORT_HOTPLUG_MASK;
	else if (IS_BROADWELL(dev_priv))
		de_port_enables |= GEN8_PORT_DP_A_HOTPLUG;

	dev_priv->de_irq_mask[PIPE_A] = ~de_pipe_masked;
	dev_priv->de_irq_mask[PIPE_B] = ~de_pipe_masked;
	dev_priv->de_irq_mask[PIPE_C] = ~de_pipe_masked;

	for_each_pipe(dev_priv, pipe)
		if (intel_display_power_is_enabled(dev_priv,
				POWER_DOMAIN_PIPE(pipe)))
			GEN8_IRQ_INIT_NDX(DE_PIPE, pipe,
					  dev_priv->de_irq_mask[pipe],
					  de_pipe_enables);

	GEN5_IRQ_INIT(GEN8_DE_PORT_, ~de_port_masked, de_port_enables);
}

static int gen8_irq_postinstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (HAS_PCH_SPLIT(dev))
		ibx_irq_pre_postinstall(dev);

	gen8_gt_irq_postinstall(dev_priv);
	gen8_de_irq_postinstall(dev_priv);

	if (HAS_PCH_SPLIT(dev))
		ibx_irq_postinstall(dev);

	I915_WRITE(GEN8_MASTER_IRQ, DE_MASTER_IRQ_CONTROL);
	POSTING_READ(GEN8_MASTER_IRQ);

	return 0;
}

static int cherryview_irq_postinstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	vlv_display_irq_postinstall(dev_priv);

	gen8_gt_irq_postinstall(dev_priv);

	I915_WRITE(GEN8_MASTER_IRQ, MASTER_INTERRUPT_ENABLE);
	POSTING_READ(GEN8_MASTER_IRQ);

	return 0;
}

static void gen8_irq_uninstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (!dev_priv)
		return;

	gen8_irq_reset(dev);
}

static void vlv_display_irq_uninstall(struct drm_i915_private *dev_priv)
{
	/* Interrupt setup is already guaranteed to be single-threaded, this is
	 * just to make the assert_spin_locked check happy. */
	spin_lock_irq(&dev_priv->irq_lock);
	if (dev_priv->display_irqs_enabled)
		valleyview_display_irqs_uninstall(dev_priv);
	spin_unlock_irq(&dev_priv->irq_lock);

	vlv_display_irq_reset(dev_priv);

	dev_priv->irq_mask = ~0;
}

static void valleyview_irq_uninstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (!dev_priv)
		return;

	I915_WRITE(VLV_MASTER_IER, 0);

	gen5_gt_irq_reset(dev);

	I915_WRITE(HWSTAM, 0xffffffff);

	vlv_display_irq_uninstall(dev_priv);
}

static void cherryview_irq_uninstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (!dev_priv)
		return;

	I915_WRITE(GEN8_MASTER_IRQ, 0);
	POSTING_READ(GEN8_MASTER_IRQ);

	gen8_gt_irq_reset(dev_priv);

	GEN5_IRQ_RESET(GEN8_PCU_);

	vlv_display_irq_uninstall(dev_priv);
}

static void ironlake_irq_uninstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (!dev_priv)
		return;

	ironlake_irq_reset(dev);
}

static void i8xx_irq_preinstall(struct drm_device * dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int pipe;

	for_each_pipe(dev_priv, pipe)
		I915_WRITE(PIPESTAT(pipe), 0);
	I915_WRITE16(IMR, 0xffff);
	I915_WRITE16(IER, 0x0);
	POSTING_READ16(IER);
}

static int i8xx_irq_postinstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	I915_WRITE16(EMR,
		     ~(I915_ERROR_PAGE_TABLE | I915_ERROR_MEMORY_REFRESH));

	/* Unmask the interrupts that we always want on. */
	dev_priv->irq_mask =
		~(I915_DISPLAY_PIPE_A_EVENT_INTERRUPT |
		  I915_DISPLAY_PIPE_B_EVENT_INTERRUPT |
		  I915_DISPLAY_PLANE_A_FLIP_PENDING_INTERRUPT |
		  I915_DISPLAY_PLANE_B_FLIP_PENDING_INTERRUPT);
	I915_WRITE16(IMR, dev_priv->irq_mask);

	I915_WRITE16(IER,
		     I915_DISPLAY_PIPE_A_EVENT_INTERRUPT |
		     I915_DISPLAY_PIPE_B_EVENT_INTERRUPT |
		     I915_USER_INTERRUPT);
	POSTING_READ16(IER);

	/* Interrupt setup is already guaranteed to be single-threaded, this is
	 * just to make the assert_spin_locked check happy. */
	spin_lock_irq(&dev_priv->irq_lock);
	i915_enable_pipestat(dev_priv, PIPE_A, PIPE_CRC_DONE_INTERRUPT_STATUS);
	i915_enable_pipestat(dev_priv, PIPE_B, PIPE_CRC_DONE_INTERRUPT_STATUS);
	spin_unlock_irq(&dev_priv->irq_lock);

	return 0;
}

/*
 * Returns true when a page flip has completed.
 */
static bool i8xx_handle_vblank(struct drm_device *dev,
			       int plane, int pipe, u32 iir)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u16 flip_pending = DISPLAY_PLANE_FLIP_PENDING(plane);

	if (!intel_pipe_handle_vblank(dev, pipe))
		return false;

	if ((iir & flip_pending) == 0)
		goto check_page_flip;

	/* We detect FlipDone by looking for the change in PendingFlip from '1'
	 * to '0' on the following vblank, i.e. IIR has the Pendingflip
	 * asserted following the MI_DISPLAY_FLIP, but ISR is deasserted, hence
	 * the flip is completed (no longer pending). Since this doesn't raise
	 * an interrupt per se, we watch for the change at vblank.
	 */
	if (I915_READ16(ISR) & flip_pending)
		goto check_page_flip;

	intel_prepare_page_flip(dev, plane);
	intel_finish_page_flip(dev, pipe);
	return true;

check_page_flip:
	intel_check_page_flip(dev, pipe);
	return false;
}

static irqreturn_t i8xx_irq_handler(int irq, void *arg)
{
	struct drm_device *dev = arg;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u16 iir, new_iir;
	u32 pipe_stats[2];
	int pipe;
	u16 flip_mask =
		I915_DISPLAY_PLANE_A_FLIP_PENDING_INTERRUPT |
		I915_DISPLAY_PLANE_B_FLIP_PENDING_INTERRUPT;

	if (!intel_irqs_enabled(dev_priv))
		return IRQ_NONE;

	iir = I915_READ16(IIR);
	if (iir == 0)
		return IRQ_NONE;

	while (iir & ~flip_mask) {
		/* Can't rely on pipestat interrupt bit in iir as it might
		 * have been cleared after the pipestat interrupt was received.
		 * It doesn't set the bit in iir again, but it still produces
		 * interrupts (for non-MSI).
		 */
		spin_lock(&dev_priv->irq_lock);
		if (iir & I915_RENDER_COMMAND_PARSER_ERROR_INTERRUPT)
			DRM_DEBUG("Command parser error, iir 0x%08x\n", iir);

		for_each_pipe(dev_priv, pipe) {
			int reg = PIPESTAT(pipe);
			pipe_stats[pipe] = I915_READ(reg);

			/*
			 * Clear the PIPE*STAT regs before the IIR
			 */
			if (pipe_stats[pipe] & 0x8000ffff)
				I915_WRITE(reg, pipe_stats[pipe]);
		}
		spin_unlock(&dev_priv->irq_lock);

		I915_WRITE16(IIR, iir & ~flip_mask);
		new_iir = I915_READ16(IIR); /* Flush posted writes */

		if (iir & I915_USER_INTERRUPT)
			notify_ring(&dev_priv->ring[RCS]);

		for_each_pipe(dev_priv, pipe) {
			int plane = pipe;
			if (HAS_FBC(dev))
				plane = !plane;

			if (pipe_stats[pipe] & PIPE_VBLANK_INTERRUPT_STATUS &&
			    i8xx_handle_vblank(dev, plane, pipe, iir))
				flip_mask &= ~DISPLAY_PLANE_FLIP_PENDING(plane);

			if (pipe_stats[pipe] & PIPE_CRC_DONE_INTERRUPT_STATUS)
				i9xx_pipe_crc_irq_handler(dev, pipe);

			if (pipe_stats[pipe] & PIPE_FIFO_UNDERRUN_STATUS)
				intel_cpu_fifo_underrun_irq_handler(dev_priv,
								    pipe);
		}

		iir = new_iir;
	}

	return IRQ_HANDLED;
}

static void i8xx_irq_uninstall(struct drm_device * dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int pipe;

	for_each_pipe(dev_priv, pipe) {
		/* Clear enable bits; then clear status bits */
		I915_WRITE(PIPESTAT(pipe), 0);
		I915_WRITE(PIPESTAT(pipe), I915_READ(PIPESTAT(pipe)));
	}
	I915_WRITE16(IMR, 0xffff);
	I915_WRITE16(IER, 0x0);
	I915_WRITE16(IIR, I915_READ16(IIR));
}

static void i915_irq_preinstall(struct drm_device * dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int pipe;

	if (I915_HAS_HOTPLUG(dev)) {
		i915_hotplug_interrupt_update(dev_priv, 0xffffffff, 0);
		I915_WRITE(PORT_HOTPLUG_STAT, I915_READ(PORT_HOTPLUG_STAT));
	}

	I915_WRITE16(HWSTAM, 0xeffe);
	for_each_pipe(dev_priv, pipe)
		I915_WRITE(PIPESTAT(pipe), 0);
	I915_WRITE(IMR, 0xffffffff);
	I915_WRITE(IER, 0x0);
	POSTING_READ(IER);
}

static int i915_irq_postinstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 enable_mask;

	I915_WRITE(EMR, ~(I915_ERROR_PAGE_TABLE | I915_ERROR_MEMORY_REFRESH));

	/* Unmask the interrupts that we always want on. */
	dev_priv->irq_mask =
		~(I915_ASLE_INTERRUPT |
		  I915_DISPLAY_PIPE_A_EVENT_INTERRUPT |
		  I915_DISPLAY_PIPE_B_EVENT_INTERRUPT |
		  I915_DISPLAY_PLANE_A_FLIP_PENDING_INTERRUPT |
		  I915_DISPLAY_PLANE_B_FLIP_PENDING_INTERRUPT);

	enable_mask =
		I915_ASLE_INTERRUPT |
		I915_DISPLAY_PIPE_A_EVENT_INTERRUPT |
		I915_DISPLAY_PIPE_B_EVENT_INTERRUPT |
		I915_USER_INTERRUPT;

	if (I915_HAS_HOTPLUG(dev)) {
		i915_hotplug_interrupt_update(dev_priv, 0xffffffff, 0);
		POSTING_READ(PORT_HOTPLUG_EN);

		/* Enable in IER... */
		enable_mask |= I915_DISPLAY_PORT_INTERRUPT;
		/* and unmask in IMR */
		dev_priv->irq_mask &= ~I915_DISPLAY_PORT_INTERRUPT;
	}

	I915_WRITE(IMR, dev_priv->irq_mask);
	I915_WRITE(IER, enable_mask);
	POSTING_READ(IER);

	i915_enable_asle_pipestat(dev);

	/* Interrupt setup is already guaranteed to be single-threaded, this is
	 * just to make the assert_spin_locked check happy. */
	spin_lock_irq(&dev_priv->irq_lock);
	i915_enable_pipestat(dev_priv, PIPE_A, PIPE_CRC_DONE_INTERRUPT_STATUS);
	i915_enable_pipestat(dev_priv, PIPE_B, PIPE_CRC_DONE_INTERRUPT_STATUS);
	spin_unlock_irq(&dev_priv->irq_lock);

	return 0;
}

/*
 * Returns true when a page flip has completed.
 */
static bool i915_handle_vblank(struct drm_device *dev,
			       int plane, int pipe, u32 iir)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 flip_pending = DISPLAY_PLANE_FLIP_PENDING(plane);

	if (!intel_pipe_handle_vblank(dev, pipe))
		return false;

	if ((iir & flip_pending) == 0)
		goto check_page_flip;

	/* We detect FlipDone by looking for the change in PendingFlip from '1'
	 * to '0' on the following vblank, i.e. IIR has the Pendingflip
	 * asserted following the MI_DISPLAY_FLIP, but ISR is deasserted, hence
	 * the flip is completed (no longer pending). Since this doesn't raise
	 * an interrupt per se, we watch for the change at vblank.
	 */
	if (I915_READ(ISR) & flip_pending)
		goto check_page_flip;

	intel_prepare_page_flip(dev, plane);
	intel_finish_page_flip(dev, pipe);
	return true;

check_page_flip:
	intel_check_page_flip(dev, pipe);
	return false;
}

static irqreturn_t i915_irq_handler(int irq, void *arg)
{
	struct drm_device *dev = arg;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 iir, new_iir, pipe_stats[I915_MAX_PIPES];
	u32 flip_mask =
		I915_DISPLAY_PLANE_A_FLIP_PENDING_INTERRUPT |
		I915_DISPLAY_PLANE_B_FLIP_PENDING_INTERRUPT;
	int pipe, ret = IRQ_NONE;

	if (!intel_irqs_enabled(dev_priv))
		return IRQ_NONE;

	iir = I915_READ(IIR);
	do {
		bool irq_received = (iir & ~flip_mask) != 0;
		bool blc_event = false;

		/* Can't rely on pipestat interrupt bit in iir as it might
		 * have been cleared after the pipestat interrupt was received.
		 * It doesn't set the bit in iir again, but it still produces
		 * interrupts (for non-MSI).
		 */
		spin_lock(&dev_priv->irq_lock);
		if (iir & I915_RENDER_COMMAND_PARSER_ERROR_INTERRUPT)
			DRM_DEBUG("Command parser error, iir 0x%08x\n", iir);

		for_each_pipe(dev_priv, pipe) {
			int reg = PIPESTAT(pipe);
			pipe_stats[pipe] = I915_READ(reg);

			/* Clear the PIPE*STAT regs before the IIR */
			if (pipe_stats[pipe] & 0x8000ffff) {
				I915_WRITE(reg, pipe_stats[pipe]);
				irq_received = true;
			}
		}
		spin_unlock(&dev_priv->irq_lock);

		if (!irq_received)
			break;

		/* Consume port.  Then clear IIR or we'll miss events */
		if (I915_HAS_HOTPLUG(dev) &&
		    iir & I915_DISPLAY_PORT_INTERRUPT)
			i9xx_hpd_irq_handler(dev);

		I915_WRITE(IIR, iir & ~flip_mask);
		new_iir = I915_READ(IIR); /* Flush posted writes */

		if (iir & I915_USER_INTERRUPT)
			notify_ring(&dev_priv->ring[RCS]);

		for_each_pipe(dev_priv, pipe) {
			int plane = pipe;
			if (HAS_FBC(dev))
				plane = !plane;

			if (pipe_stats[pipe] & PIPE_VBLANK_INTERRUPT_STATUS &&
			    i915_handle_vblank(dev, plane, pipe, iir))
				flip_mask &= ~DISPLAY_PLANE_FLIP_PENDING(plane);

			if (pipe_stats[pipe] & PIPE_LEGACY_BLC_EVENT_STATUS)
				blc_event = true;

			if (pipe_stats[pipe] & PIPE_CRC_DONE_INTERRUPT_STATUS)
				i9xx_pipe_crc_irq_handler(dev, pipe);

			if (pipe_stats[pipe] & PIPE_FIFO_UNDERRUN_STATUS)
				intel_cpu_fifo_underrun_irq_handler(dev_priv,
								    pipe);
		}

		if (blc_event || (iir & I915_ASLE_INTERRUPT))
			intel_opregion_asle_intr(dev);

		/* With MSI, interrupts are only generated when iir
		 * transitions from zero to nonzero.  If another bit got
		 * set while we were handling the existing iir bits, then
		 * we would never get another interrupt.
		 *
		 * This is fine on non-MSI as well, as if we hit this path
		 * we avoid exiting the interrupt handler only to generate
		 * another one.
		 *
		 * Note that for MSI this could cause a stray interrupt report
		 * if an interrupt landed in the time between writing IIR and
		 * the posting read.  This should be rare enough to never
		 * trigger the 99% of 100,000 interrupts test for disabling
		 * stray interrupts.
		 */
		ret = IRQ_HANDLED;
		iir = new_iir;
	} while (iir & ~flip_mask);

	return ret;
}

static void i915_irq_uninstall(struct drm_device * dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int pipe;

	if (I915_HAS_HOTPLUG(dev)) {
		i915_hotplug_interrupt_update(dev_priv, 0xffffffff, 0);
		I915_WRITE(PORT_HOTPLUG_STAT, I915_READ(PORT_HOTPLUG_STAT));
	}

	I915_WRITE16(HWSTAM, 0xffff);
	for_each_pipe(dev_priv, pipe) {
		/* Clear enable bits; then clear status bits */
		I915_WRITE(PIPESTAT(pipe), 0);
		I915_WRITE(PIPESTAT(pipe), I915_READ(PIPESTAT(pipe)));
	}
	I915_WRITE(IMR, 0xffffffff);
	I915_WRITE(IER, 0x0);

	I915_WRITE(IIR, I915_READ(IIR));
}

static void i965_irq_preinstall(struct drm_device * dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int pipe;

	i915_hotplug_interrupt_update(dev_priv, 0xffffffff, 0);
	I915_WRITE(PORT_HOTPLUG_STAT, I915_READ(PORT_HOTPLUG_STAT));

	I915_WRITE(HWSTAM, 0xeffe);
	for_each_pipe(dev_priv, pipe)
		I915_WRITE(PIPESTAT(pipe), 0);
	I915_WRITE(IMR, 0xffffffff);
	I915_WRITE(IER, 0x0);
	POSTING_READ(IER);
}

static int i965_irq_postinstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 enable_mask;
	u32 error_mask;

	/* Unmask the interrupts that we always want on. */
	dev_priv->irq_mask = ~(I915_ASLE_INTERRUPT |
			       I915_DISPLAY_PORT_INTERRUPT |
			       I915_DISPLAY_PIPE_A_EVENT_INTERRUPT |
			       I915_DISPLAY_PIPE_B_EVENT_INTERRUPT |
			       I915_DISPLAY_PLANE_A_FLIP_PENDING_INTERRUPT |
			       I915_DISPLAY_PLANE_B_FLIP_PENDING_INTERRUPT |
			       I915_RENDER_COMMAND_PARSER_ERROR_INTERRUPT);

	enable_mask = ~dev_priv->irq_mask;
	enable_mask &= ~(I915_DISPLAY_PLANE_A_FLIP_PENDING_INTERRUPT |
			 I915_DISPLAY_PLANE_B_FLIP_PENDING_INTERRUPT);
	enable_mask |= I915_USER_INTERRUPT;

	if (IS_G4X(dev))
		enable_mask |= I915_BSD_USER_INTERRUPT;

	/* Interrupt setup is already guaranteed to be single-threaded, this is
	 * just to make the assert_spin_locked check happy. */
	spin_lock_irq(&dev_priv->irq_lock);
	i915_enable_pipestat(dev_priv, PIPE_A, PIPE_GMBUS_INTERRUPT_STATUS);
	i915_enable_pipestat(dev_priv, PIPE_A, PIPE_CRC_DONE_INTERRUPT_STATUS);
	i915_enable_pipestat(dev_priv, PIPE_B, PIPE_CRC_DONE_INTERRUPT_STATUS);
	spin_unlock_irq(&dev_priv->irq_lock);

	/*
	 * Enable some error detection, note the instruction error mask
	 * bit is reserved, so we leave it masked.
	 */
	if (IS_G4X(dev)) {
		error_mask = ~(GM45_ERROR_PAGE_TABLE |
			       GM45_ERROR_MEM_PRIV |
			       GM45_ERROR_CP_PRIV |
			       I915_ERROR_MEMORY_REFRESH);
	} else {
		error_mask = ~(I915_ERROR_PAGE_TABLE |
			       I915_ERROR_MEMORY_REFRESH);
	}
	I915_WRITE(EMR, error_mask);

	I915_WRITE(IMR, dev_priv->irq_mask);
	I915_WRITE(IER, enable_mask);
	POSTING_READ(IER);

	i915_hotplug_interrupt_update(dev_priv, 0xffffffff, 0);
	POSTING_READ(PORT_HOTPLUG_EN);

	i915_enable_asle_pipestat(dev);

	return 0;
}

static void i915_hpd_irq_setup(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 hotplug_en;

	assert_spin_locked(&dev_priv->irq_lock);

	/* Note HDMI and DP share hotplug bits */
	/* enable bits are the same for all generations */
	hotplug_en = intel_hpd_enabled_irqs(dev, hpd_mask_i915);
	/* Programming the CRT detection parameters tends
	   to generate a spurious hotplug event about three
	   seconds later.  So just do it once.
	*/
	if (IS_G4X(dev))
		hotplug_en |= CRT_HOTPLUG_ACTIVATION_PERIOD_64;
	hotplug_en |= CRT_HOTPLUG_VOLTAGE_COMPARE_50;

	/* Ignore TV since it's buggy */
	i915_hotplug_interrupt_update_locked(dev_priv,
					     HOTPLUG_INT_EN_MASK |
					     CRT_HOTPLUG_VOLTAGE_COMPARE_MASK |
					     CRT_HOTPLUG_ACTIVATION_PERIOD_64,
					     hotplug_en);
}

static irqreturn_t i965_irq_handler(int irq, void *arg)
{
	struct drm_device *dev = arg;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 iir, new_iir;
	u32 pipe_stats[I915_MAX_PIPES];
	int ret = IRQ_NONE, pipe;
	u32 flip_mask =
		I915_DISPLAY_PLANE_A_FLIP_PENDING_INTERRUPT |
		I915_DISPLAY_PLANE_B_FLIP_PENDING_INTERRUPT;

	if (!intel_irqs_enabled(dev_priv))
		return IRQ_NONE;

	iir = I915_READ(IIR);

	for (;;) {
		bool irq_received = (iir & ~flip_mask) != 0;
		bool blc_event = false;

		/* Can't rely on pipestat interrupt bit in iir as it might
		 * have been cleared after the pipestat interrupt was received.
		 * It doesn't set the bit in iir again, but it still produces
		 * interrupts (for non-MSI).
		 */
		spin_lock(&dev_priv->irq_lock);
		if (iir & I915_RENDER_COMMAND_PARSER_ERROR_INTERRUPT)
			DRM_DEBUG("Command parser error, iir 0x%08x\n", iir);

		for_each_pipe(dev_priv, pipe) {
			int reg = PIPESTAT(pipe);
			pipe_stats[pipe] = I915_READ(reg);

			/*
			 * Clear the PIPE*STAT regs before the IIR
			 */
			if (pipe_stats[pipe] & 0x8000ffff) {
				I915_WRITE(reg, pipe_stats[pipe]);
				irq_received = true;
			}
		}
		spin_unlock(&dev_priv->irq_lock);

		if (!irq_received)
			break;

		ret = IRQ_HANDLED;

		/* Consume port.  Then clear IIR or we'll miss events */
		if (iir & I915_DISPLAY_PORT_INTERRUPT)
			i9xx_hpd_irq_handler(dev);

		I915_WRITE(IIR, iir & ~flip_mask);
		new_iir = I915_READ(IIR); /* Flush posted writes */

		if (iir & I915_USER_INTERRUPT)
			notify_ring(&dev_priv->ring[RCS]);
		if (iir & I915_BSD_USER_INTERRUPT)
			notify_ring(&dev_priv->ring[VCS]);

		for_each_pipe(dev_priv, pipe) {
			if (pipe_stats[pipe] & PIPE_START_VBLANK_INTERRUPT_STATUS &&
			    i915_handle_vblank(dev, pipe, pipe, iir))
				flip_mask &= ~DISPLAY_PLANE_FLIP_PENDING(pipe);

			if (pipe_stats[pipe] & PIPE_LEGACY_BLC_EVENT_STATUS)
				blc_event = true;

			if (pipe_stats[pipe] & PIPE_CRC_DONE_INTERRUPT_STATUS)
				i9xx_pipe_crc_irq_handler(dev, pipe);

			if (pipe_stats[pipe] & PIPE_FIFO_UNDERRUN_STATUS)
				intel_cpu_fifo_underrun_irq_handler(dev_priv, pipe);
		}

		if (blc_event || (iir & I915_ASLE_INTERRUPT))
			intel_opregion_asle_intr(dev);

		if (pipe_stats[0] & PIPE_GMBUS_INTERRUPT_STATUS)
			gmbus_irq_handler(dev);

		/* With MSI, interrupts are only generated when iir
		 * transitions from zero to nonzero.  If another bit got
		 * set while we were handling the existing iir bits, then
		 * we would never get another interrupt.
		 *
		 * This is fine on non-MSI as well, as if we hit this path
		 * we avoid exiting the interrupt handler only to generate
		 * another one.
		 *
		 * Note that for MSI this could cause a stray interrupt report
		 * if an interrupt landed in the time between writing IIR and
		 * the posting read.  This should be rare enough to never
		 * trigger the 99% of 100,000 interrupts test for disabling
		 * stray interrupts.
		 */
		iir = new_iir;
	}

	return ret;
}

static void i965_irq_uninstall(struct drm_device * dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int pipe;

	if (!dev_priv)
		return;

	i915_hotplug_interrupt_update(dev_priv, 0xffffffff, 0);
	I915_WRITE(PORT_HOTPLUG_STAT, I915_READ(PORT_HOTPLUG_STAT));

	I915_WRITE(HWSTAM, 0xffffffff);
	for_each_pipe(dev_priv, pipe)
		I915_WRITE(PIPESTAT(pipe), 0);
	I915_WRITE(IMR, 0xffffffff);
	I915_WRITE(IER, 0x0);

	for_each_pipe(dev_priv, pipe)
		I915_WRITE(PIPESTAT(pipe),
			   I915_READ(PIPESTAT(pipe)) & 0x8000ffff);
	I915_WRITE(IIR, I915_READ(IIR));
}

/**
 * intel_irq_init - initializes irq support
 * @dev_priv: i915 device instance
 *
 * This function initializes all the irq support including work items, timers
 * and all the vtables. It does not setup the interrupt itself though.
 */
void intel_irq_init(struct drm_i915_private *dev_priv)
{
	struct drm_device *dev = dev_priv->dev;

	intel_hpd_init_work(dev_priv);

	INIT_WORK(&dev_priv->rps.work, gen6_pm_rps_work);
	INIT_WORK(&dev_priv->l3_parity.error_work, ivybridge_parity_work);

	/* Let's track the enabled rps events */
	if (IS_VALLEYVIEW(dev_priv) && !IS_CHERRYVIEW(dev_priv))
		/* WaGsvRC0ResidencyMethod:vlv */
		dev_priv->pm_rps_events = GEN6_PM_RP_UP_EI_EXPIRED;
	else
		dev_priv->pm_rps_events = GEN6_PM_RPS_EVENTS;

	INIT_DELAYED_WORK(&dev_priv->gpu_error.hangcheck_work,
			  i915_hangcheck_elapsed);

	pm_qos_add_request(&dev_priv->pm_qos, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);

	if (IS_GEN2(dev_priv)) {
		dev->max_vblank_count = 0;
		dev->driver->get_vblank_counter = i8xx_get_vblank_counter;
	} else if (IS_G4X(dev_priv) || INTEL_INFO(dev_priv)->gen >= 5) {
		dev->max_vblank_count = 0xffffffff; /* full 32 bit counter */
		dev->driver->get_vblank_counter = g4x_get_vblank_counter;
	} else {
		dev->driver->get_vblank_counter = i915_get_vblank_counter;
		dev->max_vblank_count = 0xffffff; /* only 24 bits of frame count */
	}

	/*
	 * Opt out of the vblank disable timer on everything except gen2.
	 * Gen2 doesn't have a hardware frame counter and so depends on
	 * vblank interrupts to produce sane vblank seuquence numbers.
	 */
	if (!IS_GEN2(dev_priv))
		dev->vblank_disable_immediate = true;

	dev->driver->get_vblank_timestamp = i915_get_vblank_timestamp;
	dev->driver->get_scanout_position = i915_get_crtc_scanoutpos;

	if (IS_CHERRYVIEW(dev_priv)) {
		dev->driver->irq_handler = cherryview_irq_handler;
		dev->driver->irq_preinstall = cherryview_irq_preinstall;
		dev->driver->irq_postinstall = cherryview_irq_postinstall;
		dev->driver->irq_uninstall = cherryview_irq_uninstall;
		dev->driver->enable_vblank = valleyview_enable_vblank;
		dev->driver->disable_vblank = valleyview_disable_vblank;
		dev_priv->display.hpd_irq_setup = i915_hpd_irq_setup;
	} else if (IS_VALLEYVIEW(dev_priv)) {
		dev->driver->irq_handler = valleyview_irq_handler;
		dev->driver->irq_preinstall = valleyview_irq_preinstall;
		dev->driver->irq_postinstall = valleyview_irq_postinstall;
		dev->driver->irq_uninstall = valleyview_irq_uninstall;
		dev->driver->enable_vblank = valleyview_enable_vblank;
		dev->driver->disable_vblank = valleyview_disable_vblank;
		dev_priv->display.hpd_irq_setup = i915_hpd_irq_setup;
	} else if (INTEL_INFO(dev_priv)->gen >= 8) {
		dev->driver->irq_handler = gen8_irq_handler;
		dev->driver->irq_preinstall = gen8_irq_reset;
		dev->driver->irq_postinstall = gen8_irq_postinstall;
		dev->driver->irq_uninstall = gen8_irq_uninstall;
		dev->driver->enable_vblank = gen8_enable_vblank;
		dev->driver->disable_vblank = gen8_disable_vblank;
		if (IS_BROXTON(dev))
			dev_priv->display.hpd_irq_setup = bxt_hpd_irq_setup;
		else if (HAS_PCH_SPT(dev))
			dev_priv->display.hpd_irq_setup = spt_hpd_irq_setup;
		else
			dev_priv->display.hpd_irq_setup = ilk_hpd_irq_setup;
	} else if (HAS_PCH_SPLIT(dev)) {
		dev->driver->irq_handler = ironlake_irq_handler;
		dev->driver->irq_preinstall = ironlake_irq_reset;
		dev->driver->irq_postinstall = ironlake_irq_postinstall;
		dev->driver->irq_uninstall = ironlake_irq_uninstall;
		dev->driver->enable_vblank = ironlake_enable_vblank;
		dev->driver->disable_vblank = ironlake_disable_vblank;
		dev_priv->display.hpd_irq_setup = ilk_hpd_irq_setup;
	} else {
		if (INTEL_INFO(dev_priv)->gen == 2) {
			dev->driver->irq_preinstall = i8xx_irq_preinstall;
			dev->driver->irq_postinstall = i8xx_irq_postinstall;
			dev->driver->irq_handler = i8xx_irq_handler;
			dev->driver->irq_uninstall = i8xx_irq_uninstall;
		} else if (INTEL_INFO(dev_priv)->gen == 3) {
			dev->driver->irq_preinstall = i915_irq_preinstall;
			dev->driver->irq_postinstall = i915_irq_postinstall;
			dev->driver->irq_uninstall = i915_irq_uninstall;
			dev->driver->irq_handler = i915_irq_handler;
		} else {
			dev->driver->irq_preinstall = i965_irq_preinstall;
			dev->driver->irq_postinstall = i965_irq_postinstall;
			dev->driver->irq_uninstall = i965_irq_uninstall;
			dev->driver->irq_handler = i965_irq_handler;
		}
		if (I915_HAS_HOTPLUG(dev_priv))
			dev_priv->display.hpd_irq_setup = i915_hpd_irq_setup;
		dev->driver->enable_vblank = i915_enable_vblank;
		dev->driver->disable_vblank = i915_disable_vblank;
	}
}

/**
 * intel_irq_install - enables the hardware interrupt
 * @dev_priv: i915 device instance
 *
 * This function enables the hardware interrupt handling, but leaves the hotplug
 * handling still disabled. It is called after intel_irq_init().
 *
 * In the driver load and resume code we need working interrupts in a few places
 * but don't want to deal with the hassle of concurrent probe and hotplug
 * workers. Hence the split into this two-stage approach.
 */
int intel_irq_install(struct drm_i915_private *dev_priv)
{
	/*
	 * We enable some interrupt sources in our postinstall hooks, so mark
	 * interrupts as enabled _before_ actually enabling them to avoid
	 * special cases in our ordering checks.
	 */
	dev_priv->pm.irqs_enabled = true;

	return drm_irq_install(dev_priv->dev, dev_priv->dev->pdev->irq);
}

/**
 * intel_irq_uninstall - finilizes all irq handling
 * @dev_priv: i915 device instance
 *
 * This stops interrupt and hotplug handling and unregisters and frees all
 * resources acquired in the init functions.
 */
void intel_irq_uninstall(struct drm_i915_private *dev_priv)
{
	drm_irq_uninstall(dev_priv->dev);
	intel_hpd_cancel_work(dev_priv);
	dev_priv->pm.irqs_enabled = false;
}

/**
 * intel_runtime_pm_disable_interrupts - runtime interrupt disabling
 * @dev_priv: i915 device instance
 *
 * This function is used to disable interrupts at runtime, both in the runtime
 * pm and the system suspend/resume code.
 */
void intel_runtime_pm_disable_interrupts(struct drm_i915_private *dev_priv)
{
	dev_priv->dev->driver->irq_uninstall(dev_priv->dev);
	dev_priv->pm.irqs_enabled = false;
	synchronize_irq(dev_priv->dev->irq);
}

/**
 * intel_runtime_pm_enable_interrupts - runtime interrupt enabling
 * @dev_priv: i915 device instance
 *
 * This function is used to enable interrupts at runtime, both in the runtime
 * pm and the system suspend/resume code.
 */
void intel_runtime_pm_enable_interrupts(struct drm_i915_private *dev_priv)
{
	dev_priv->pm.irqs_enabled = true;
	dev_priv->dev->driver->irq_preinstall(dev_priv->dev);
	dev_priv->dev->driver->irq_postinstall(dev_priv->dev);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /**************************************************************************
 *
 * Copyright © 2009 - 2015 VMware, Inc., Palo Alto, CA., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "vmwgfx_drv.h"
#include "vmwgfx_reg.h"
#include <drm/ttm/ttm_bo_api.h>
#include <drm/ttm/ttm_placement.h>
#include "vmwgfx_so.h"
#include "vmwgfx_binding.h"

#define VMW_RES_HT_ORDER 12

/**
 * struct vmw_resource_relocation - Relocation info for resources
 *
 * @head: List head for the software context's relocation list.
 * @res: Non-ref-counted pointer to the resource.
 * @offset: Offset of 4 byte entries into the command buffer where the
 * id that needs fixup is located.
 */
struct vmw_resource_relocation {
	struct list_head head;
	const struct vmw_resource *res;
	unsigned long offset;
};

/**
 * struct vmw_resource_val_node - Validation info for resources
 *
 * @head: List head for the software context's resource list.
 * @hash: Hash entry for quick resouce to val_node lookup.
 * @res: Ref-counted pointer to the resource.
 * @switch_backup: Boolean whether to switch backup buffer on unreserve.
 * @new_backup: Refcounted pointer to the new backup buffer.
 * @staged_bindings: If @res is a context, tracks bindings set up during
 * the command batch. Otherwise NULL.
 * @new_backup_offset: New backup buffer offset if @new_backup is non-NUll.
 * @first_usage: Set to true the first time the resource is referenced in
 * the command stream.
 * @switching_backup: The command stream provides a new backup buffer for a
 * resource.
 * @no_buffer_needed: This means @switching_backup is true on first buffer
 * reference. So resource reservation does not need to allocate a backup
 * buffer for the resource.
 */
struct vmw_resource_val_node {
	struct list_head head;
	struct drm_hash_item hash;
	struct vmw_resource *res;
	struct vmw_dma_buffer *new_backup;
	struct vmw_ctx_binding_state *staged_bindings;
	unsigned long new_backup_offset;
	u32 first_usage : 1;
	u32 switching_backup : 1;
	u32 no_buffer_needed : 1;
};

/**
 * struct vmw_cmd_entry - Describe a command for the verifier
 *
 * @user_allow: Whether allowed from the execbuf ioctl.
 * @gb_disable: Whether disabled if guest-backed objects are available.
 * @gb_enable: Whether enabled iff guest-backed objects are available.
 */
struct vmw_cmd_entry {
	int (*func) (struct vmw_private *, struct vmw_sw_context *,
		     SVGA3dCmdHeader *);
	bool user_allow;
	bool gb_disable;
	bool gb_enable;
};

#define VMW_CMD_DEF(_cmd, _func, _user_allow, _gb_disable, _gb_enable)	\
	[(_cmd) - SVGA_3D_CMD_BASE] = {(_func), (_user_allow),\
				       (_gb_disable), (_gb_enable)}

static int vmw_resource_context_res_add(struct vmw_private *dev_priv,
					struct vmw_sw_context *sw_context,
					struct vmw_resource *ctx);
static int vmw_translate_mob_ptr(struct vmw_private *dev_priv,
				 struct vmw_sw_context *sw_context,
				 SVGAMobId *id,
				 struct vmw_dma_buffer **vmw_bo_p);
static int vmw_bo_to_validate_list(struct vmw_sw_context *sw_context,
				   struct vmw_dma_buffer *vbo,
				   bool validate_as_mob,
				   uint32_t *p_val_node);


/**
 * vmw_resources_unreserve - unreserve resources previously reserved for
 * command submission.
 *
 * @sw_context: pointer to the software context
 * @backoff: Whether command submission failed.
 */
static void vmw_resources_unreserve(struct vmw_sw_context *sw_context,
				    bool backoff)
{
	struct vmw_resource_val_node *val;
	struct list_head *list = &sw_context->resource_list;

	if (sw_context->dx_query_mob && !backoff)
		vmw_context_bind_dx_query(sw_context->dx_query_ctx,
					  sw_context->dx_query_mob);

	list_for_each_entry(val, list, head) {
		struct vmw_resource *res = val->res;
		bool switch_backup =
			(backoff) ? false : val->switching_backup;

		/*
		 * Transfer staged context bindings to the
		 * persistent context binding tracker.
		 */
		if (unlikely(val->staged_bindings)) {
			if (!backoff) {
				vmw_binding_state_commit
					(vmw_context_binding_state(val->res),
					 val->staged_bindings);
			}

			if (val->staged_bindings != sw_context->staged_bindings)
				vmw_binding_state_free(val->staged_bindings);
			else
				sw_context->staged_bindings_inuse = false;
			val->staged_bindings = NULL;
		}
		vmw_resource_unreserve(res, switch_backup, val->new_backup,
				       val->new_backup_offset);
		vmw_dmabuf_unreference(&val->new_backup);
	}
}

/**
 * vmw_cmd_ctx_first_setup - Perform the setup needed when a context is
 * added to the validate list.
 *
 * @dev_priv: Pointer to the device private:
 * @sw_context: The validation context:
 * @node: The validation node holding this context.
 */
static int vmw_cmd_ctx_first_setup(struct vmw_private *dev_priv,
				   struct vmw_sw_context *sw_context,
				   struct vmw_resource_val_node *node)
{
	int ret;

	ret = vmw_resource_context_res_add(dev_priv, sw_context, node->res);
	if (unlikely(ret != 0))
		goto out_err;

	if (!sw_context->staged_bindings) {
		sw_context->staged_bindings =
			vmw_binding_state_alloc(dev_priv);
		if (IS_ERR(sw_context->staged_bindings)) {
			DRM_ERROR("Failed to allocate context binding "
				  "information.\n");
			ret = PTR_ERR(sw_context->staged_bindings);
			sw_context->staged_bindings = NULL;
			goto out_err;
		}
	}

	if (sw_context->staged_bindings_inuse) {
		node->staged_bindings = vmw_binding_state_alloc(dev_priv);
		if (IS_ERR(node->staged_bindings)) {
			DRM_ERROR("Failed to allocate context binding "
				  "information.\n");
			ret = PTR_ERR(node->staged_bindings);
			node->staged_bindings = NULL;
			goto out_err;
		}
	} else {
		node->staged_bindings = sw_context->staged_bindings;
		sw_context->staged_bindings_inuse = true;
	}

	return 0;
out_err:
	return ret;
}

/**
 * vmw_resource_val_add - Add a resource to the software context's
 * resource list if it's not already on it.
 *
 * @sw_context: Pointer to the software context.
 * @res: Pointer to the resource.
 * @p_node On successful return points to a valid pointer to a
 * struct vmw_resource_val_node, if non-NULL on entry.
 */
static int vmw_resource_val_add(struct vmw_sw_context *sw_context,
				struct vmw_resource *res,
				struct vmw_resource_val_node **p_node)
{
	struct vmw_private *dev_priv = res->dev_priv;
	struct vmw_resource_val_node *node;
	struct drm_hash_item *hash;
	int ret;

	if (likely(drm_ht_find_item(&sw_context->res_ht, (unsigned long) res,
				    &hash) == 0)) {
		node = container_of(hash, struct vmw_resource_val_node, hash);
		node->first_usage = false;
		if (unlikely(p_node != NULL))
			*p_node = node;
		return 0;
	}

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (unlikely(node == NULL)) {
		DRM_ERROR("Failed to allocate a resource validation "
			  "entry.\n");
		return -ENOMEM;
	}

	node->hash.key = (unsigned long) res;
	ret = drm_ht_insert_item(&sw_context->res_ht, &node->hash);
	if (unlikely(ret != 0)) {
		DRM_ERROR("Failed to initialize a resource validation "
			  "entry.\n");
		kfree(node);
		return ret;
	}
	node->res = vmw_resource_reference(res);
	node->first_usage = true;
	if (unlikely(p_node != NULL))
		*p_node = node;

	if (!dev_priv->has_mob) {
		list_add_tail(&node->head, &sw_context->resource_list);
		return 0;
	}

	switch (vmw_res_type(res)) {
	case vmw_res_context:
	case vmw_res_dx_context:
		list_add(&node->head, &sw_context->ctx_resource_list);
		ret = vmw_cmd_ctx_first_setup(dev_priv, sw_context, node);
		break;
	case vmw_res_cotable:
		list_add_tail(&node->head, &sw_context->ctx_resource_list);
		break;
	default:
		list_add_tail(&node->head, &sw_context->resource_list);
		break;
	}

	return ret;
}

/**
 * vmw_view_res_val_add - Add a view and the surface it's pointing to
 * to the validation list
 *
 * @sw_context: The software context holding the validation list.
 * @view: Pointer to the view resource.
 *
 * Returns 0 if success, negative error code otherwise.
 */
static int vmw_view_res_val_add(struct vmw_sw_context *sw_context,
				struct vmw_resource *view)
{
	int ret;

	/*
	 * First add the resource the view is pointing to, otherwise
	 * it may be swapped out when the view is validated.
	 */
	ret = vmw_resource_val_add(sw_context, vmw_view_srf(view), NULL);
	if (ret)
		return ret;

	return vmw_resource_val_add(sw_context, view, NULL);
}

/**
 * vmw_view_id_val_add - Look up a view and add it and the surface it's
 * pointing to to the validation list.
 *
 * @sw_context: The software context holding the validation list.
 * @view_type: The view type to look up.
 * @id: view id of the view.
 *
 * The view is represented by a view id and the DX context it's created on,
 * or scheduled for creation on. If there is no DX context set, the function
 * will return -EINVAL. Otherwise returns 0 on success and -EINVAL on failure.
 */
static int vmw_view_id_val_add(struct vmw_sw_context *sw_context,
			       enum vmw_view_type view_type, u32 id)
{
	struct vmw_resource_val_node *ctx_node = sw_context->dx_ctx_node;
	struct vmw_resource *view;
	int ret;

	if (!ctx_node) {
		DRM_ERROR("DX Context not set.\n");
		return -EINVAL;
	}

	view = vmw_view_lookup(sw_context->man, view_type, id);
	if (IS_ERR(view))
		return PTR_ERR(view);

	ret = vmw_view_res_val_add(sw_context, view);
	vmw_resource_unreference(&view);

	return ret;
}

/**
 * vmw_resource_context_res_add - Put resources previously bound to a context on
 * the validation list
 *
 * @dev_priv: Pointer to a device private structure
 * @sw_context: Pointer to a software context used for this command submission
 * @ctx: Pointer to the context resource
 *
 * This function puts all resources that were previously bound to @ctx on
 * the resource validation list. This is part of the context state reemission
 */
static int vmw_resource_context_res_add(struct vmw_private *dev_priv,
					struct vmw_sw_context *sw_context,
					struct vmw_resource *ctx)
{
	struct list_head *binding_list;
	struct vmw_ctx_bindinfo *entry;
	int ret = 0;
	struct vmw_resource *res;
	u32 i;

	/* Add all cotables to the validation list. */
	if (dev_priv->has_dx && vmw_res_type(ctx) == vmw_res_dx_context) {
		for (i = 0; i < SVGA_COTABLE_DX10_MAX; ++i) {
			res = vmw_context_cotable(ctx, i);
			if (IS_ERR(res))
				continue;

			ret = vmw_resource_val_add(sw_context, res, NULL);
			vmw_resource_unreference(&res);
			if (unlikely(ret != 0))
				return ret;
		}
	}


	/* Add all resources bound to the context to the validation list */
	mutex_lock(&dev_priv->binding_mutex);
	binding_list = vmw_context_binding_list(ctx);

	list_for_each_entry(entry, binding_list, ctx_list) {
		/* entry->res is not refcounted */
		res = vmw_resource_reference_unless_doomed(entry->res);
		if (unlikely(res == NULL))
			continue;

		if (vmw_res_type(entry->res) == vmw_res_view)
			ret = vmw_view_res_val_add(sw_context, entry->res);
		else
			ret = vmw_resource_val_add(sw_context, entry->res,
						   NULL);
		vmw_resource_unreference(&res);
		if (unlikely(ret != 0))
			break;
	}

	if (dev_priv->has_dx && vmw_res_type(ctx) == vmw_res_dx_context) {
		struct vmw_dma_buffer *dx_query_mob;

		dx_query_mob = vmw_context_get_dx_query_mob(ctx);
		if (dx_query_mob)
			ret = vmw_bo_to_validate_list(sw_context,
						      dx_query_mob,
						      true, NULL);
	}

	mutex_unlock(&dev_priv->binding_mutex);
	return ret;
}

/**
 * vmw_resource_relocation_add - Add a relocation to the relocation list
 *
 * @list: Pointer to head of relocation list.
 * @res: The resource.
 * @offset: Offset into the command buffer currently being parsed where the
 * id that needs fixup is located. Granularity is 4 bytes.
 */
static int vmw_resource_relocation_add(struct list_head *list,
				       const struct vmw_resource *res,
				       unsigned long offset)
{
	struct vmw_resource_relocation *rel;

	rel = kmalloc(sizeof(*rel), GFP_KERNEL);
	if (unlikely(rel == NULL)) {
		DRM_ERROR("Failed to allocate a resource relocation.\n");
		return -ENOMEM;
	}

	rel->res = res;
	rel->offset = offset;
	list_add_tail(&rel->head, list);

	return 0;
}

/**
 * vmw_resource_relocations_free - Free all relocations on a list
 *
 * @list: Pointer to the head of the relocation list.
 */
static void vmw_resource_relocations_free(struct list_head *list)
{
	struct vmw_resource_relocation *rel, *n;

	list_for_each_entry_safe(rel, n, list, head) {
		list_del(&rel->head);
		kfree(rel);
	}
}

/**
 * vmw_resource_relocations_apply - Apply all relocations on a list
 *
 * @cb: Pointer to the start of the command buffer bein patch. This need
 * not be the same buffer as the one being parsed when the relocation
 * list was built, but the contents must be the same modulo the
 * resource ids.
 * @list: Pointer to the head of the relocation list.
 */
static void vmw_resource_relocations_apply(uint32_t *cb,
					   struct list_head *list)
{
	struct vmw_resource_relocation *rel;

	list_for_each_entry(rel, list, head) {
		if (likely(rel->res != NULL))
			cb[rel->offset] = rel->res->id;
		else
			cb[rel->offset] = SVGA_3D_CMD_NOP;
	}
}

static int vmw_cmd_invalid(struct vmw_private *dev_priv,
			   struct vmw_sw_context *sw_context,
			   SVGA3dCmdHeader *header)
{
	return -EINVAL;
}

static int vmw_cmd_ok(struct vmw_private *dev_priv,
		      struct vmw_sw_context *sw_context,
		      SVGA3dCmdHeader *header)
{
	return 0;
}

/**
 * vmw_bo_to_validate_list - add a bo to a validate list
 *
 * @sw_context: The software context used for this command submission batch.
 * @bo: The buffer object to add.
 * @validate_as_mob: Validate this buffer as a MOB.
 * @p_val_node: If non-NULL Will be updated with the validate node number
 * on return.
 *
 * Returns -EINVAL if the limit of number of buffer objects per command
 * submission is reached.
 */
static int vmw_bo_to_validate_list(struct vmw_sw_context *sw_context,
				   struct vmw_dma_buffer *vbo,
				   bool validate_as_mob,
				   uint32_t *p_val_node)
{
	uint32_t val_node;
	struct vmw_validate_buffer *vval_buf;
	struct ttm_validate_buffer *val_buf;
	struct drm_hash_item *hash;
	int ret;

	if (likely(drm_ht_find_item(&sw_context->res_ht, (unsigned long) vbo,
				    &hash) == 0)) {
		vval_buf = container_of(hash, struct vmw_validate_buffer,
					hash);
		if (unlikely(vval_buf->validate_as_mob != validate_as_mob)) {
			DRM_ERROR("Inconsistent buffer usage.\n");
			return -EINVAL;
		}
		val_buf = &vval_buf->base;
		val_node = vval_buf - sw_context->val_bufs;
	} else {
		val_node = sw_context->cur_val_buf;
		if (unlikely(val_node >= VMWGFX_MAX_VALIDATIONS)) {
			DRM_ERROR("Max number of DMA buffers per submission "
				  "exceeded.\n");
			return -EINVAL;
		}
		vval_buf = &sw_context->val_bufs[val_node];
		vval_buf->hash.key = (unsigned long) vbo;
		ret = drm_ht_insert_item(&sw_context->res_ht, &vval_buf->hash);
		if (unlikely(ret != 0)) {
			DRM_ERROR("Failed to initialize a buffer validation "
				  "entry.\n");
			return ret;
		}
		++sw_context->cur_val_buf;
		val_buf = &vval_buf->base;
		val_buf->bo = ttm_bo_reference(&vbo->base);
		val_buf->shared = false;
		list_add_tail(&val_buf->head, &sw_context->validate_nodes);
		vval_buf->validate_as_mob = validate_as_mob;
	}

	if (p_val_node)
		*p_val_node = val_node;

	return 0;
}

/**
 * vmw_resources_reserve - Reserve all resources on the sw_context's
 * resource list.
 *
 * @sw_context: Pointer to the software context.
 *
 * Note that since vmware's command submission currently is protected by
 * the cmdbuf mutex, no fancy deadlock avoidance is required for resources,
 * since only a single thread at once will attempt this.
 */
static int vmw_resources_reserve(struct vmw_sw_context *sw_context)
{
	struct vmw_resource_val_node *val;
	int ret = 0;

	list_for_each_entry(val, &sw_context->resource_list, head) {
		struct vmw_resource *res = val->res;

		ret = vmw_resource_reserve(res, true, val->no_buffer_needed);
		if (unlikely(ret != 0))
			return ret;

		if (res->backup) {
			struct vmw_dma_buffer *vbo = res->backup;

			ret = vmw_bo_to_validate_list
				(sw_context, vbo,
				 vmw_resource_needs_backup(res), NULL);

			if (unlikely(ret != 0))
				return ret;
		}
	}

	if (sw_context->dx_query_mob) {
		struct vmw_dma_buffer *expected_dx_query_mob;

		expected_dx_query_mob =
			vmw_context_get_dx_query_mob(sw_context->dx_query_ctx);
		if (expected_dx_query_mob &&
		    expected_dx_query_mob != sw_context->dx_query_mob) {
			ret = -EINVAL;
		}
	}

	return ret;
}

/**
 * vmw_resources_validate - Validate all resources on the sw_context's
 * resource list.
 *
 * @sw_context: Pointer to the software context.
 *
 * Before this function is called, all resource backup buffers must have
 * been validated.
 */
static int vmw_resources_validate(struct vmw_sw_context *sw_context)
{
	struct vmw_resource_val_node *val;
	int ret;

	list_for_each_entry(val, &sw_context->resource_list, head) {
		struct vmw_resource *res = val->res;
		struct vmw_dma_buffer *backup = res->backup;

		ret = vmw_resource_validate(res);
		if (unlikely(ret != 0)) {
			if (ret != -ERESTARTSYS)
				DRM_ERROR("Failed to validate resource.\n");
			return ret;
		}

		/* Check if the resource switched backup buffer */
		if (backup && res->backup && (backup != res->backup)) {
			struct vmw_dma_buffer *vbo = res->backup;

			ret = vmw_bo_to_validate_list
				(sw_context, vbo,
				 vmw_resource_needs_backup(res), NULL);
			if (ret) {
				ttm_bo_unreserve(&vbo->base);
				return ret;
			}
		}
	}
	return 0;
}

/**
 * vmw_cmd_res_reloc_add - Add a resource to a software context's
 * relocation- and validation lists.
 *
 * @dev_priv: Pointer to a struct vmw_private identifying the device.
 * @sw_context: Pointer to the software context.
 * @id_loc: Pointer to where the id that needs translation is located.
 * @res: Valid pointer to a struct vmw_resource.
 * @p_val: If non null, a pointer to the struct vmw_resource_validate_node
 * used for this resource is returned here.
 */
static int vmw_cmd_res_reloc_add(struct vmw_private *dev_priv,
				 struct vmw_sw_context *sw_context,
				 uint32_t *id_loc,
				 struct vmw_resource *res,
				 struct vmw_resource_val_node **p_val)
{
	int ret;
	struct vmw_reso