 \
	POSTING_READ(GEN8_##type##_IIR(which)); \
	I915_WRITE(GEN8_##type##_IIR(which), 0xffffffff); \
	POSTING_READ(GEN8_##type##_IIR(which)); \
} while (0)

#define GEN5_IRQ_RESET(type) do { \
	I915_WRITE(type##IMR, 0xffffffff); \
	POSTING_READ(type##IMR); \
	I915_WRITE(type##IER, 0); \
	I915_WRITE(type##IIR, 0xffffffff); \
	POSTING_READ(type##IIR); \
	I915_WRITE(type##IIR, 0xffffffff); \
	POSTING_READ(type##IIR); \
} while (0)

/*
 * We should clear IMR at preinstall/uninstall, and just check at postinstall.
 */
static void gen5_assert_iir_is_zero(struct drm_i915_private *dev_priv, u32 reg)
{
	u32 val = I915_READ(reg);

	if (val == 0)
		return;

	WARN(1, "Interrupt register 0x%x is not zero: 0x%08x\n",
	     reg, val);
	I915_WRITE(reg, 0xffffffff);
	POSTING_READ(reg);
	I915_WRITE(reg, 0xffffffff);
	POSTING_READ(reg);
}

#define GEN8_IRQ_INIT_NDX(type, which, imr_val, ier_val) do { \
	gen5_assert_iir_is_zero(dev_priv, GEN8_##type##_IIR(which)); \
	I915_WRITE(GEN8_##type##_IER(which), (ier_val)); \
	I915_WRITE(GEN8_##type##_IMR(which), (imr_val)); \
	POSTING_READ(GEN8_##type##_IMR(which)); \
} while (0)

#define GEN5_IRQ_INIT(type, imr_val, ier_val) do { \
	gen5_assert_iir_is_zero(dev_priv, type##IIR); \
	I915_WRITE(type##IER, (ier_val)); \
	I915_WRITE(type##IMR, (imr_val)); \
	POSTING_READ(type##IMR); \
} while (0)

static void gen6_rps_irq_handler(struct drm_i915_private *dev_priv, u32 pm_iir);

/* For display hotplug interrupt */
static inline void
i915_hotplug_interrupt_update_locked(struct drm_i915_private *dev_priv,
				     uint32_t mask,
				     uint32_t bits)
{
	uint32_t val;

	assert_spin_locked(&dev_priv->irq_lock);
	WARN_ON(bits & ~mask);

	val = I915_READ(PORT_HOTPLUG_EN);
	val &= ~mask;
	val |= bits;
	I915_WRITE(PORT_HOTPLUG_EN, val);
}

/**
 * i915_hotplug_interrupt_update - update hotplug interrupt enable
 * @dev_priv: driver private
 * @mask: bits to update
 * @bits: bits to enable
 * NOTE: the HPD enable bits are modified both inside and outside
 * of an interrupt context. To avoid that read-modify-write cycles
 * interfer, these bits are protected by a spinlock. Since this
 * function is usually not called from a context where the lock is
 * held already, this function acquires the lock itself. A non-locking
 * version is also available.
 */
void i915_hotplug_interrupt_update(struct drm_i915_private *dev_priv,
				   uint32_t mask,
				   uint32_t bits)
{
	spin_lock_irq(&dev_priv->irq_lock);
	i915_hotplug_interrupt_update_locked(dev_priv, mask, bits);
	spin_unlock_irq(&dev_priv->irq_lock);
}

/**
 * ilk_update_display_irq - update DEIMR
 * @dev_priv: driver private
 * @interrupt_mask: mask of interrupt bits to update
 * @enabled_irq_mask: mask of interrupt bits to enable
 */
static void ilk_update_display_irq(struct drm_i915_private *dev_priv,
				   uint32_t interrupt_mask,
				   uint32_t enabled_irq_mask)
{
	uint32_t new_val;

	assert_spin_locked(&dev_priv->irq_lock);

	WARN_ON(enabled_irq_mask & ~interrupt_mask);

	if (WARN_ON(!intel_irqs_enabled(dev_priv)))
		return;

	new_val = dev_priv->irq_mask;
	new_val &= ~interrupt_mask;
	new_val |= (~enabled_irq_mask & interrupt_mask);

	if (new_val != dev_priv->irq_mask) {
		dev_priv->irq_mask = new_val;
		I915_WRITE(DEIMR, dev_priv->irq_mask);
		POSTING_READ(DEIMR);
	}
}

void
ironlake_enable_display_irq(struct drm_i915_private *dev_priv, u32 mask)
{
	ilk_update_display_irq(dev_priv, mask, mask);
}

void
ironlake_disable_display_irq(struct drm_i915_private *dev_priv, u32 mask)
{
	ilk_update_display_irq(dev_priv, mask, 0);
}

/**
 * ilk_update_gt_irq - update GTIMR
 * @dev_priv: driver private
 * @interrupt_mask: mask of interrupt bits to update
 * @enabled_irq_mask: mask of interrupt bits to enable
 */
static void ilk_update_gt_irq(struct drm_i915_private *dev_priv,
			      uint32_t interrupt_mask,
			      uint32_t enabled_irq_mask)
{
	assert_spin_locked(&dev_priv->irq_lock);

	WARN_ON(enabled_irq_mask & ~interrupt_mask);

	if (WARN_ON(!intel_irqs_enabled(dev_priv)))
		return;

	dev_priv->gt_irq_mask &= ~interrupt_mask;
	dev_priv->gt_irq_mask |= (~enabled_irq_mask & interrupt_mask);
	I915_WRITE(GTIMR, dev_priv->gt_irq_mask);
	POSTING_READ(GTIMR);
}

void gen5_enable_gt_irq(struct drm_i915_private *dev_priv, uint32_t mask)
{
	ilk_update_gt_irq(dev_priv, mask, mask);
}

void gen5_disable_gt_irq(struct drm_i915_private *dev_priv, uint32_t mask)
{
	ilk_update_gt_irq(dev_priv, mask, 0);
}

static u32 gen6_pm_iir(struct drm_i915_private *dev_priv)
{
	return INTEL_INFO(dev_priv)->gen >= 8 ? GEN8_GT_IIR(2) : GEN6_PMIIR;
}

static u32 gen6_pm_imr(struct drm_i915_private *dev_priv)
{
	return INTEL_INFO(dev_priv)->gen >= 8 ? GEN8_GT_IMR(2) : GEN6_PMIMR;
}

static u32 gen6_pm_ier(struct drm_i915_private *dev_priv)
{
	return INTEL_INFO(dev_priv)->gen >= 8 ? GEN8_GT_IER(2) : GEN6_PMIER;
}

/**
  * snb_update_pm_irq - update GEN6_PMIMR
  * @dev_priv: driver private
  * @interrupt_mask: mask of interrupt bits to update
  * @enabled_irq_mask: mask of interrupt bits to enable
  */
static void snb_update_pm_irq(struct drm_i915_private *dev_priv,
			      uint32_t interrupt_mask,
			      uint32_t enabled_irq_mask)
{
	uint32_t new_val;

	WARN_ON(enabled_irq_mask & ~interrupt_mask);

	assert_spin_locked(&dev_priv->irq_lock);

	new_val = dev_priv->pm_irq_mask;
	new_val &= ~interrupt_mask;
	new_val |= (~enabled_irq_mask & interrupt_mask);

	if (new_val != dev_priv->pm_irq_mask) {
		dev_priv->pm_irq_mask = new_val;
		I915_WRITE(gen6_pm_imr(dev_priv), dev_priv->pm_irq_mask);
		POSTING_READ(gen6_pm_imr(dev_priv));
	}
}

void gen6_enable_pm_irq(struct drm_i915_private *dev_priv, uint32_t mask)
{
	if (WARN_ON(!intel_irqs_enabled(dev_priv)))
		return;

	snb_update_pm_irq(dev_priv, mask, mask);
}

static void __gen6_disable_pm_irq(struct drm_i915_private *dev_priv,
				  uint32_t mask)
{
	snb_update_pm_irq(dev_priv, mask, 0);
}

void gen6_disable_pm_irq(struct drm_i915_private *dev_priv, uint32_t mask)
{
	if (WARN_ON(!intel_irqs_enabled(dev_priv)))
		return;

	__gen6_disable_pm_irq(dev_priv, mask);
}

void gen6_reset_rps_interrupts(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint32_t reg = gen6_pm_iir(dev_priv);

	spin_lock_irq(&dev_priv->irq_lock);
	I915_WRITE(reg, dev_priv->pm_rps_events);
	I915_WRITE(reg, dev_priv->pm_rps_events);
	POSTING_READ(reg);
	dev_priv->rps.pm_iir = 0;
	spin_unlock_irq(&dev_priv->irq_lock);
}

void gen6_enable_rps_interrupts(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	spin_lock_irq(&dev_priv->irq_lock);

	WARN_ON(dev_priv->rps.pm_iir);
	WARN_ON(I915_READ(gen6_pm_iir(dev_priv)) & dev_priv->pm_rps_events);
	dev_priv->rps.interrupts_enabled = true;
	I915_WRITE(gen6_pm_ier(dev_priv), I915_READ(gen6_pm_ier(dev_priv)) |
				dev_priv->pm_rps_events);
	gen6_enable_pm_irq(dev_priv, dev_priv->pm_rps_events);

	spin_unlock_irq(&dev_priv->irq_lock);
}

u32 gen6_sanitize_rps_pm_mask(struct drm_i915_private *dev_priv, u32 mask)
{
	/*
	 * SNB,IVB can while VLV,CHV may hard hang on looping batchbuffer
	 * if GEN6_PM_UP_EI_EXPIRED is masked.
	 *
	 * TODO: verify if this can be reproduced on VLV,CHV.
	 */
	if (INTEL_INFO(dev_priv)->gen <= 7 && !IS_HASWELL(dev_priv))
		mask &= ~GEN6_PM_RP_UP_EI_EXPIRED;

	if (INTEL_INFO(dev_priv)->gen >= 8)
		mask &= ~GEN8_PMINTR_REDIRECT_TO_NON_DISP;

	return mask;
}

void gen6_disable_rps_interrupts(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	spin_lock_irq(&dev_priv->irq_lock);
	dev_priv->rps.interrupts_enabled = false;
	spin_unlock_irq(&dev_priv->irq_lock);

	cancel_work_sync(&dev_priv->rps.work);

	spin_lock_irq(&dev_priv->irq_lock);

	I915_WRITE(GEN6_PMINTRMSK, gen6_sanitize_rps_pm_mask(dev_priv, ~0));

	__gen6_disable_pm_irq(dev_priv, dev_priv->pm_rps_events);
	I915_WRITE(gen6_pm_ier(dev_priv), I915_READ(gen6_pm_ier(dev_priv)) &
				~dev_priv->pm_rps_events);

	spin_unlock_irq(&dev_priv->irq_lock);

	synchronize_irq(dev->irq);
}

/**
  * bdw_update_port_irq - update DE port interrupt
  * @dev_priv: driver private
  * @interrupt_mask: mask of interrupt bits to update
  * @enabled_irq_mask: mask of interrupt bits to enable
  */
static void bdw_update_port_irq(struct drm_i915_private *dev_priv,
				uint32_t interrupt_mask,
				uint32_t enabled_irq_mask)
{
	uint32_t new_val;
	uint32_t old_val;

	assert_spin_locked(&dev_priv->irq_lock);

	WARN_ON(enabled_irq_mask & ~interrupt_mask);

	if (WARN_ON(!intel_irqs_enabled(dev_priv)))
		return;

	old_val = I915_READ(GEN8_DE_PORT_IMR);

	new_val = old_val;
	new_val &= ~interrupt_mask;
	new_val |= (~enabled_irq_mask & interrupt_mask);

	if (new_val != old_val) {
		I915_WRITE(GEN8_DE_PORT_IMR, new_val);
		POSTING_READ(GEN8_DE_PORT_IMR);
	}
}

/**
 * ibx_display_interrupt_update - update SDEIMR
 * @dev_priv: driver private
 * @interrupt_mask: mask of interrupt bits to update
 * @enabled_irq_mask: mask of interrupt bits to enable
 */
void ibx_display_interrupt_update(struct drm_i915_private *dev_priv,
				  uint32_t interrupt_mask,
				  uint32_t enabled_irq_mask)
{
	uint32_t sdeimr = I915_READ(SDEIMR);
	sdeimr &= ~interrupt_mask;
	sdeimr |= (~enabled_irq_mask & interrupt_mask);

	WARN_ON(enabled_irq_mask & ~interrupt_mask);

	assert_spin_locked(&dev_priv->irq_lock);

	if (WARN_ON(!intel_irqs_enabled(dev_priv)))
		return;

	I915_WRITE(SDEIMR, sdeimr);
	POSTING_READ(SDEIMR);
}

static void
__i915_enable_pipestat(struct drm_i915_private *dev_priv, enum pipe pipe,
		       u32 enable_mask, u32 status_mask)
{
	u32 reg = PIPESTAT(pipe);
	u32 pipestat = I915_READ(reg) & PIPESTAT_INT_ENABLE_MASK;

	assert_spin_locked(&dev_priv->irq_lock);
	WARN_ON(!intel_irqs_enabled(dev_priv));

	if (WARN_ONCE(enable_mask & ~PIPESTAT_INT_ENABLE_MASK ||
		      status_mask & ~PIPESTAT_INT_STATUS_MASK,
		      "pipe %c: enable_mask=0x%x, status_mask=0x%x\n",
		      pipe_name(pipe), enable_mask, status_mask))
		return;

	if ((pipestat & enable_mask) == enable_mask)
		return;

	dev_priv->pipestat_irq_mask[pipe] |= status_mask;

	/* Enable the interrupt, clear any pending status */
	pipestat |= enable_mask | status_mask;
	I915_WRITE(reg, pipestat);
	POSTING_READ(reg);
}

static void
__i915_disable_pipestat(struct drm_i915_private *dev_priv, enum pipe pipe,
		        u32 enable_mask, u32 status_mask)
{
	u32 reg = PIPESTAT(pipe);
	u32 pipestat = I915_READ(reg) & PIPESTAT_INT_ENABLE_MASK;

	assert_spin_locked(&dev_priv->irq_lock);
	WARN_ON(!intel_irqs_enabled(dev_priv));

	if (WARN_ONCE(enable_mask & ~PIPESTAT_INT_ENABLE_MASK ||
		      status_mask & ~PIPESTAT_INT_STATUS_MASK,
		      "pipe %c: enable_mask=0x%x, status_mask=0x%x\n",
		      pipe_name(pipe), enable_mask, status_mask))
		return;

	if ((pipestat & enable_mask) == 0)
		return;

	dev_priv->pipestat_irq_mask[pipe] &= ~status_mask;

	pipestat &= ~enable_mask;
	I915_WRITE(reg, pipestat);
	POSTING_READ(reg);
}

static u32 vlv_get_pipestat_enable_mask(struct drm_device *dev, u32 status_mask)
{
	u32 enable_mask = status_mask << 16;

	/*
	 * On pipe A we don't support the PSR interrupt yet,
	 * on pipe B and C the same bit MBZ.
	 */
	if (WARN_ON_ONCE(status_mask & PIPE_A_PSR_STATUS_VLV))
		return 0;
	/*
	 * On pipe B and C we don't support the PSR interrupt yet, on pipe
	 * A the same bit is for perf counters which we don't use either.
	 */
	if (WARN_ON_ONCE(status_mask & PIPE_B_PSR_STATUS_VLV))
		return 0;

	enable_mask &= ~(PIPE_FIFO_UNDERRUN_STATUS |
			 SPRITE0_FLIP_DONE_INT_EN_VLV |
			 SPRITE1_FLIP_DONE_INT_EN_VLV);
	if (status_mask & SPRITE0_FLIP_DONE_INT_STATUS_VLV)
		enable_mask |= SPRITE0_FLIP_DONE_INT_EN_VLV;
	if (status_mask & SPRITE1_FLIP_DONE_INT_STATUS_VLV)
		enable_mask |= SPRITE1_FLIP_DONE_INT_EN_VLV;

	return enable_mask;
}

void
i915_enable_pipestat(struct drm_i915_private *dev_priv, enum pipe pipe,
		     u32 status_mask)
{
	u32 enable_mask;

	if (IS_VALLEYVIEW(dev_priv->dev))
		enable_mask = vlv_get_pipestat_enable_mask(dev_priv->dev,
							   status_mask);
	else
		enable_mask = status_mask << 16;
	__i915_enable_pipestat(dev_priv, pipe, enable_mask, status_mask);
}

void
i915_disable_pipestat(struct drm_i915_private *dev_priv, enum pipe pipe,
		      u32 status_mask)
{
	u32 enable_mask;

	if (IS_VALLEYVIEW(dev_priv->dev))
		enable_mask = vlv_get_pipestat_enable_mask(dev_priv->dev,
							   status_mask);
	else
		enable_mask = status_mask << 16;
	__i915_disable_pipestat(dev_priv, pipe, enable_mask, status_mask);
}

/**
 * i915_enable_asle_pipestat - enable ASLE pipestat for OpRegion
 * @dev: drm device
 */
static void i915_enable_asle_pipestat(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (!dev_priv->opregion.asle || !IS_MOBILE(dev))
		return;

	spin_lock_irq(&dev_priv->irq_lock);

	i915_enable_pipestat(dev_priv, PIPE_B, PIPE_LEGACY_BLC_EVENT_STATUS);
	if (INTEL_INFO(dev)->gen >= 4)
		i915_enable_pipestat(dev_priv, PIPE_A,
				     PIPE_LEGACY_BLC_EVENT_STATUS);

	spin_unlock_irq(&dev_priv->irq_lock);
}

/*
 * This timing diagram depicts the video signal in and
 * around the vertical blanking period.
 *
 * Assumptions about the fictitious mode used in this example:
 *  vblank_start >= 3
 *  vsync_start = vblank_start + 1
 *  vsync_end = vblank_start + 2
 *  vtotal = vblank_start + 3
 *
 *           start of vblank:
 *           latch double buffered registers
 *           increment frame counter (ctg+)
 *           generate start of vblank interrupt (gen4+)
 *           |
 *           |          frame start:
 *           |          generate frame start interrupt (aka. vblank interrupt) (gmch)
 *           |          may be shifted forward 1-3 extra lines via PIPECONF
 *           |          |
 *           |          |  start of vsync:
 *           |          |  generate vsync interrupt
 *           |          |  |
 * ___xxxx___    ___xxxx___    ___xxxx___    ___xxxx___    ___xxxx___    ___xxxx
 *       .   \hs/   .      \hs/          \hs/          \hs/   .      \hs/
 * ----va---> <-----------------vb--------------------> <------