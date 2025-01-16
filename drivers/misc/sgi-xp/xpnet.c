v_priv->dev)->gen >= 8) {
		for_each_ring(signaller, dev_priv, i) {
			if (ring == signaller)
				continue;

			if (offset == signaller->semaphore.signal_ggtt[ring->id])
				return signaller;
		}
	} else {
		u32 sync_bits = ipehr & MI_SEMAPHORE_SYNC_MASK;

		for_each_ring(signaller, dev_priv, i) {
			if(ring == signaller)
				continue;

			if (sync_bits == signaller->semaphore.mbox.wait[ring->id])
				return signaller;
		}
	}

	DRM_ERROR("No signaller ring found for ring %i, ipehr 0x%08x, offset 0x%016llx\n",
		  ring->id, ipehr, offset);

	return NULL;
}

static struct intel_engine_cs *
semaphore_waits_for(struct intel_engine_cs *ring, u32 *seqno)
{
	struct drm_i915_private *dev_priv = ring->dev->dev_private;
	u32 cmd, ipehr, head;
	u64 offset = 0;
	int i, backwards;

	/*
	 * This function does not support execlist mode - any attempt to
	 * proceed further into this function will result in a kernel panic
	 * when dereferencing ring->buffer, which is not set up in execlist
	 * mode.
	 *
	 * The correct way of doing it would be to derive the currently
	 * executing ring buffer from the current context, which is derived
	 * from the currently running request. Unfortunately, to get the
	 * current request we would have to grab the struct_mutex before doing
	 * anything else, which would be ill-advised since some other thread
	 * might have grabbed it already and managed to hang itself, causing
	 * the hang checker to deadlock.
	 *
	 * Therefore, this function does not support execlist mode in its
	 * current form. Just return NULL and move on.
	 */
	if (ring->buffer == NULL)
		return NULL;

	ipehr = I915_READ(RING_IPEHR(ring->mmio_base));
	if (!ipehr_is_semaphore_wait(ring->dev, ipehr))
		return NULL;

	/*
	 * HEAD is likely pointing to the dword after the actual command,
	 * so scan backwards until we find the MBOX. But limit it to just 3
	 * or 4 dwords depending on the semaphore wait command size.
	 * Note that we don't care about ACTHD here since that might
	 * point at at batch, and semaphores are always emitted into the
	 * ringbuffer itself.
	 */
	head = I915_READ_HEAD(ring) & HEAD_ADDR;
	backwards = (INTEL_INFO(ring->dev)->gen >= 8) ? 5 : 4;

	for (i = backwards; i; --i) {
		/*
		 * Be paranoid and presume the hw has gone off into the wild -
		 * our ring is smaller than what the hardware (and hence
		 * HEAD_ADDR) allows. Also handles wrap-around.
		 */
		head &= ring->buffer->size - 1;

		/* This here seems to blow up */
		cmd = ioread32(ring->buffer->virtual_start + head);
		if (cmd == ipehr)
			break;

		head -= 4;
	}

	if (!i)
		return NULL;

	*seqno = ioread32(ring->buffer->virtual_start + head + 4) + 1;
	if (INTEL_INFO(ring->dev)->gen >= 8) {
		offset = ioread32(ring->buffer->virtual_start + head + 12);
		offset <<= 32;
		offset = ioread32(ring->buffer->virtual_start + head + 8);
	}
	return semaphore_wait_to_signaller_ring(ring, ipehr, offset);
}

static int semaphore_passed(struct intel_engine_cs *ring)
{
	struct drm_i915_private *dev_priv = ring->dev->dev_private;
	struct intel_engine_cs *signaller;
	u32 seqno;

	ring->hangcheck.deadlock++;

	signaller = semaphore_waits_for(ring, &seqno);
	if (signaller == NULL)
		return -1;

	/* Prevent pathological recursion due to driver bugs */
	if (signaller->hangcheck.deadlock >= I915_NUM_RINGS)
		return -1;

	if (i915_seqno_passed(signaller->get_seqno(signaller, false), seqno))
		return 1;

	/* cursory check for an unkickable deadlock */
	if (I915_READ_CTL(signaller) & RING_WAIT_SEMAPHORE &&
	    semaphore_passed(signaller) < 0)
		return -1;

	return 0;
}

static void semaphore_clear_deadlocks(struct drm_i915_private *dev_priv)
{
	struct intel_engine_cs *ring;
	int i;

	for_each_ring(ring, dev_priv, i)
		ring->hangcheck.deadlock = 0;
}

static enum intel_ring_hangcheck_action
ring_stuck(struct intel_engine_cs *ring, u64 acthd)
{
	struct drm_device *dev = ring->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 tmp;

	if (acthd != ring->hangcheck.acthd) {
		if (acthd > ring->hangcheck.max_acthd) {
			ring->hangcheck.max_acthd = acthd;
			return HANGCHECK_ACTIVE;
		}

		return HANGCHECK_ACTIVE_LOOP;
	}

	if (IS_GEN2(dev))
		return HANGCHECK_HUNG;

	/* Is the chip hanging on a WAIT_FOR_EVENT?
	 * If so we can simply poke the RB_WAIT bit
	 * and break the hang. This should work on
	 * all but the second generation chipsets.
	 */
	tmp = I915_READ_CTL(ring);
	if (tmp & RING_WAIT) {
		i915_handle_error(dev, false,
				  "Kicking stuck wait on %s",
				  ring->name);
		I915_WRITE_CTL(ring, tmp);
		return HANGCHECK_KICK;
	}

	if (INTEL_INFO(dev)->gen >= 6 && tmp & RING_WAIT_SEMAPHORE) {
		switch (semaphore_passed(ring)) {
		default:
			return HANGCHECK_HUNG;
		case 1:
			i915_handle_error(dev, false,
					  "Kicking stuck semaphore on %s",
					  ring->name);
			I915_WRITE_CTL(ring, tmp);
			return HANGCHECK_KICK;
		case 0:
			return HANGCHECK_WAIT;
		}
	}

	return HANGCHECK_HUNG;
}

/*
 * This is called when the chip hasn't reported back with completed
 * batchbuffers in a long time. We keep track per ring seqno progress and
 * if there are no progress, hangcheck score for that ring is increased.
 * Further, acthd is inspected to see if the ring is stuck. On stuck case
 * we kick the ring. If we see no progress on three subsequent calls
 * we assume chip is wedged and try to fix it by resetting the chip.
 */
static void i915_hangcheck_elapsed(struct work_struct *work)
{
	struct drm_i915_private *dev_priv =
		container_of(work, typeof(*dev_priv),
			     gpu_error.hangcheck_work.work);
	struct drm_device *dev = dev_priv->dev;
	struct intel_engine_cs *ring;
	int i;
	int busy_count = 0, rings_hung = 0;
	bool stuck[I915_NUM_RINGS] = { 0 };
#define BUSY 1
#define KICK 5
#define HUNG 20

	if (!i915.enable_hangcheck)
		return;

	for_each_ring(ring, dev_priv, i) {
		u64 acthd;
		u32 seqno;
		bool busy = true;

		semaphore_clear_deadlocks(dev_priv);

		seqno = ring->get_seqno(ring, false);
		acthd = intel_ring_get_active_head(ring);

		if (ring->hangcheck.seqno == seqno) {
			if (ring_idle(ring, seqno)) {
				ring->hangcheck.action = HANGCHECK_IDLE;

				if (waitqueue_active(&ring->irq_queue)) {
					/* Issue a wake-up to catch stuck h/w. */
					if (!test_and_set_bit(ring->id, &dev_priv->gpu_error.missed_irq_rings)) {
						if (!(dev_priv->gpu_error.test_irq_rings & intel_ring_flag(ring)))
							DRM_ERROR("Hangcheck timer elapsed... %s idle\n",
								  ring->name);
						else
							DRM_INFO("Fake missed irq on %s\n",
								 ring->name);
						wake_up_all(&ring->irq_queue);
					}
					/* Safeguard against driver failure */
					ring->hangcheck.score += BUSY;
				} else
					busy = false;
			} else {
				/* We always increment the hangcheck score
				 * if the ring is busy and still processing
				 * the same request, so that no single request
				 * can run indefinitely (such as a chain of
				 * batches). The only time we do not increment
				 * the hangcheck score on this ring, if this
				 * ring is in a legitimate wait for another
				 * ring. In that case the waiting ring is a
				 * victim and we want to be sure we catch the
				 * right culprit. Then every time we do kick
				 * the ring, add a small increment to the
				 * score so that we can catch a batch that is
				 * being repeatedly kicked and so responsible
				 * for stalling the machine.
				 */
				ring->hangcheck.action = ring_stuck(ring,
								    acthd);

				switch (ring->hangcheck.action) {
				case HANGCHECK_IDLE:
				case HANGCHECK_WAIT:
				case HANGCHECK_ACTIVE:
					break;
				case HANGCHECK_ACTIVE_LOOP:
					ring->hangcheck.score += BUSY;
					break;
				case HANGCHECK_KICK:
					ring->hangcheck.score += KICK;
					break;
				case HANGCHECK_HUNG:
					ring->hangcheck.score += HUNG;
					stuck[i] = true;
					break;
				}
			}
		} else {
			ring->hangcheck.action = HANGCHECK_ACTIVE;

			/* Gradually reduce the count so that we catch DoS
			 * attempts across multiple batches.
			 */
			if (ring->hangcheck.score > 0)
				ring->hangcheck.score--;

			ring->hangcheck.acthd = ring->hangcheck.max_acthd = 0;
		}

		ring->hangcheck.seqno = seqno;
		ring->hangcheck.acthd = acthd;
		busy_count += busy;
	}

	for_each_ring(ring, dev_priv, i) {
		if (ring->hangcheck.score >= HANGCHECK_SCORE_RING_HUNG) {
			DRM_INFO("%s on %s\n",
				 stuck[i] ? "stuck" : "no progress",
				 ring->name);
			rings_hung++;
		}
	}

	if (rings_hung)
		return i915_handle_error(dev, true, "Ring hung");

	if (busy_count)
		/* Reset timer case chip hangs without another request
		 * being added */
		i915_queue_hangcheck(dev);
}

void i915_queue_hangcheck(struct drm_device *dev)
{
	struct i915_gpu_error *e = &to_i915(dev)->gpu_error;

	if (!i915.enable_hangcheck)
		return;

	/* Don't continually defer the hangcheck so that it is always run at
	 * least once after work has been scheduled on any ring. Otherwise,
	 * we will ignore a hung ring if a second ring is kept busy.
	 */

	queue_delayed_work(e->hangcheck_wq, &e->hangcheck_work,
			   round_jiffies_up_relative(DRM_I915_HANGCHECK_JIFFIES));
}

static void ibx_irq_reset(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (HAS_PCH_NOP(dev))
		return;

	GEN5_IRQ_RESET(SDE);

	if (HAS_PCH_CPT(dev) || HAS_PCH_LPT(dev))
		I915_WRITE(SERR_INT, 0xffffffff);
}

/*
 * SDEIER is also touched by the interrupt handler to work around missed PCH
 * interrupts. Hence we can't update it after the interrupt handler is enabled -
 * instead we unconditionally enable all PCH interrupt sources here, but then
 * only unmask them as needed with SDEIMR.
 *
 * This function needs to be called before interrupts are enabled.
 */
static void ibx_irq_pre_postinstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (HAS_PCH_NOP(dev))
		return;

	WARN_ON(I915_READ(SDEIER) != 0);
	I915_WRITE(SDEIER, 0xffffffff);
	POSTING_READ(SDEIER);
}

static void gen5_gt_irq_reset(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	GEN5_IRQ_RESET(GT);
	if (INTEL_INFO(dev)->gen >= 6)
		GEN5_IRQ_RESET(GEN6_PM);
}

/* drm_dma.h hooks
*/
static void ironlake_irq_reset(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	I915_WRITE(HWSTAM, 0xffffffff);

	GEN5_IRQ_RESET(DE);
	if (IS_GEN7(dev))
		I915_WRITE(GEN7_ERR_INT, 0xffffffff);

	gen5_gt_irq_reset(dev);

	ibx_irq_reset(dev);
}

static void vlv_display_irq_reset(struct drm_i915_private *dev_priv)
{
	enum pipe pipe;

	i915_hotplug_interrupt_update(dev_priv, 0xFFFFFFFF, 0);
	I915_WRITE(PORT_HOTPLUG_STAT, I915_READ(PORT_HOTPLUG_STAT));

	for_each_pipe(dev_priv, pipe)
		I915_WRITE(PIPESTAT(pipe), 0xffff);

	GEN5_IRQ_RESET(VLV_);
}

static void valleyview_irq_preinstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	/* VLV magic */
	I915_WRITE(VLV_IMR, 0);
	I915_WRITE(RING_IMR(RENDER_RING_BASE), 0);
	I915_WRITE(RING_IMR(GEN6_BSD_RING_BASE), 0);
	I915_WRITE(RING_IMR(BLT_RING_BASE), 0);

	gen5_gt_irq_reset(dev);

	I915_WRITE(DPINVGTT, DPINVGTT_STATUS_MASK);

	vlv_display_irq_reset(dev_priv);
}

static void gen8_gt_irq_reset(struct drm_i915_private *dev_priv)
{
	GEN8_IRQ_RESET_NDX(GT, 0);
	GEN8_IRQ_RESET_NDX(GT, 1);
	GEN8_IRQ_RESET_NDX(GT, 2);
	GEN8_IRQ_RESET_NDX(GT, 3);
}

static void gen8_irq_reset(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int pipe;

	I915_WRITE(GEN8_MASTER_IRQ, 0);
	POSTING_READ(GEN8_MASTER_IRQ);

	gen8_gt_irq_reset(dev_priv);

	for_each_pipe(dev_priv, pipe)
		if (intel_display_power_is_enabled(dev_priv,
						   POWER_DOMAIN_PIPE(pipe)))
			GEN8_IRQ_RESET_NDX(DE_PIPE, pipe);

	GEN5_IRQ_RESET(GEN8_DE_PORT_);
	GEN5_IRQ_RESET(GEN8_DE_MISC_);
	GEN5_IRQ_RESET(GEN8_PCU_);

	if (HAS_PCH_SPLIT(dev))
		ibx_irq_reset(dev);
}

void gen8_irq_power_well_post_enable(struct drm_i915_private *dev_priv,
				     unsigned int pipe_mask)
{
	uint32_t extra_ier = GEN8_PIPE_VBLANK | GEN8_PIPE_FIFO_UNDERRUN;

	spin_lock_irq(&dev_priv->irq_lock);
	if (pipe_mask & 1 << PIPE_A)
		GEN8_IRQ_INIT_NDX(DE_PIPE, PIPE_A,
				  dev_priv->de_irq_mask[PIPE_A],
				  ~dev_priv->de_irq_mask[PIPE_A] | extra_ier);
	if (pipe_mask & 1 << PIPE_B)
		GEN8_IRQ_INIT_NDX(DE_PIPE, PIPE_B,
				  dev_priv->de_irq_mask[PIPE_B],
				  ~dev_priv->de_irq_mask[PIPE_B] | extra_ier);
	if (pipe_mask & 1 << PIPE_C)
		GEN8_IRQ_INIT_NDX(DE_PIPE, PIPE_C,
				  dev_priv->de_irq_mask[PIPE_C],
				  ~dev_priv->de_irq_mask[PIPE_C] | extra_ier);
	spin_unlock_irq(&dev_priv->irq_lock);
}

static void cherryview_irq_preinstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	I915_WRITE(GEN8_MASTER_IRQ, 0);
	POSTING_READ(GEN8_MASTER_IRQ);

	gen8_gt_irq_reset(dev_priv);

	GEN5_IRQ_RESET(GEN8_PCU_);

	I915_WRITE(DPINVGTT, DPINVGTT_STATUS_MASK_CHV);

	vlv_display_irq_reset(dev_priv);
}

static u32 intel_hpd_enabled_irqs(struct drm_device *dev,
				  const u32 hpd[HPD_NUM_PINS])
{
	struct drm_i915_private *dev_priv = to_i915(dev);
	struct intel_encoder *encoder;
	u32 enabled_irqs = 0;

	for_each_intel_encoder(dev, encoder)
		if (dev_priv->hotplug.stats[encoder->hpd_pin].state == HPD_ENABLED)
			enabled_irqs |= hpd[encoder->hpd_pin];

	return enabled_irqs;
}

static void ibx_hpd_irq_setup(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 hotplug_irqs, hotplug, enabled_irqs;

	if (HAS_PCH_IBX(dev)) {
		hotplug_irqs = SDE_HOTPLUG_MASK;
		enabled_irqs = intel_hpd_enabled_irqs(dev, hpd_ibx);
	} else {
		hotplug_irqs = SDE_HOTPLUG_MASK_CPT;
		enabled_irqs = intel_hpd_enabled_irqs(dev, hpd_cpt);
	}

	ibx_display_interrupt_update(dev_priv, hotplug_irqs, enabled_irqs);

	/*
	 * Enable digital hotplug on the PCH, and configure the DP short pulse
	 * duration to 2ms (which is the minimum in the Display Port spec).
	 * The pulse duration bits are reserved on LPT+.
	 */
	hotplug = I915_READ(PCH_PORT_HOTPLUG);
	hotplug &= ~(PORTD_PULSE_DURATION_MASK|PORTC_PULSE_DURATION_MASK|PORTB_PULSE_DURATION_MASK);
	hotplug |= PORTD_HOTPLUG_ENABLE | PORTD_PULSE_DURATION_2ms;
	hotplug |= PORTC_HOTPLUG_ENABLE | PORTC_PULSE_DURATION_2ms;
	hotplug |= PORTB_HOTPLUG_ENABLE | PORTB_PULSE_DURATION_2ms;
	/*
	 * When CPU and PCH are on the same package, port A
	 * HPD must be enabled in both north and south.
	 */
	if (HAS_PCH_LPT_LP(dev))
		hotplug |= PORTA_HOTPLUG_ENABLE;
	I915_WRITE(PCH_PORT_HOTPLUG, hotplug);
}

static void spt_hpd_irq_setup(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 hotplug_irqs, hotplug, enabled_irqs;

	hotplug_irqs = SDE_HOTPLUG_MASK_SPT;
	enabled_irqs = intel_hpd_enabled_irqs(dev, hpd_spt);

	ibx_display_interrupt_update(dev_priv, hotplug_irqs, enabled_irqs);

	/* Enable digital hotplug on the PCH */
	hotplug = I915_READ(PCH_PORT_HOTPLUG);
	hotplug |= PORTD_HOTPLUG_ENABLE | PORTC_HOTPLUG_ENABLE |
		PORTB_HOTPLUG_ENABLE | PORTA_HOTPLUG_ENABLE;
	I915_WRITE(PCH_PORT_HOTPLUG, hotplug);

	hotplug = I915_READ(PCH_PORT_HOTPLUG2);
	hotplug |= PORTE_HOTPLUG_ENABLE;
	I915_WRITE(PCH_PORT_HOTPLUG2, hotplug);
}

static void ilk_hpd_irq_setup(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 hotplug_irqs, hotplug, enabled_irqs;

	if (INTEL_INFO(dev)->gen >= 8) {
		hotplug_irqs = GEN8_PORT_DP_A_HOTPLUG;
		enabled_irqs = intel_hpd_enabled_irqs(dev, hpd_bdw);

		bdw_update_port_irq(dev_priv, hotplug_irqs, enabled_irqs);
	} else if (INTEL_INFO(dev)->gen >= 7) {
		hotplug_irqs = DE_DP_A_HOTPLUG_IVB;
		enabled_irqs = intel_hpd_enabled_irqs(dev, hpd_ivb);

		ilk_update_display_irq(dev_priv, hotplug_irqs, enabled_irqs);
	} else {
		hotplug_irqs = DE_DP_A_HOTPLUG;
		enabled_irqs = intel_hpd_enabled_irqs(dev, hpd_ilk);

		ilk_update_display_irq(dev_priv, hotplug_irqs, enabled_irqs);
	}

	/*
	 * Enable digital hotplug on the CPU, and configure the DP short pulse
	 * duration to 2ms (which is the minimum in the Display Port spec)
	 * The pulse duration bits are reserved on HSW+.
	 */
	hotplug = I915_READ(DIGITAL_PORT_HOTPLUG_CNTRL);
	hotplug &= ~DIGITAL_PORTA_PULSE_DURATION_MASK;
	hotplug |= DIGITAL_PORTA_HOTPLUG_ENABLE | DIGITAL_PORTA_PULSE_DURATION_2ms;
	I915_WRITE(DIGITAL_PORT_HOTPLUG_CNTRL, hotplug);

	ibx_hpd_irq_setup(dev);
}

static void bxt_hpd_irq_setup(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 hotplug_irqs, hotplug, enabled_irqs;

	enabled_irqs = intel_hpd_enabled_irqs(dev, hpd_bxt);
	hotplug_irqs = BXT_DE_PORT_HOTPLUG_MASK;

	bdw_update_port_irq(dev_priv, hotplug_irqs, enabled_irqs);

	hotplug = I915_READ(PCH_PORT_HOTPLUG);
	hotplug |= PORTC_HOTPLUG_ENABLE | PORTB_HOTPLUG_ENABLE |
		PORTA_HOTPLUG_ENABLE;
	I915_WRITE(PCH_PORT_HOTPLUG, hotplug);
}

static void ibx_irq_postinstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 mask;

	if (HAS_PCH_NOP(dev))
		return;

	if (HAS_PCH_IBX(dev))
		mask = SDE_GMBUS | SDE_AUX_MASK | SDE_POISON;
	else
		mask = SDE_GMBUS_CPT | SDE_AUX_MASK_CPT;

	gen5_assert_iir_is_zero(dev_priv, SDEIIR);
	I915_WRITE(SDEIMR, ~mask);
}

static void gen5_gt_irq_postinstall(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 pm_irqs, gt_irqs;

	pm_irqs = gt_irqs = 0;

	dev_priv->gt_irq_mask = ~0;
	if (HAS_L3_DPF(dev)) {
		/* L3 parity interrupt is always unmasked. */
		dev_priv->gt_irq_mask = ~GT_PARITY_ERROR(dev);
		gt_irqs |= GT_PARITY_ERROR(dev);
	}

	gt_irqs |= GT_RENDER_USER_INTERRUPT;
	if (IS_GEN5(dev)) {
		gt_irqs |= GT_RENDER_PIPECTL_NOTIFY_INTERRUPT |
			   ILK_BSD_USER_INTERRUPT;
	} else {
		gt_irqs |= GT_BLT_USER_INTERRUPT | GT_BSD_USER_INTERRUPT;
	}

	GEN5_IRQ_INIT(GT, dev_priv->gt_irq_mask, gt_irqs);

	if (INTEL_INFO(dev)->gen >= 6) {
		/*
		 * RPS interrupts will get enabled/disabled on demand when RPS
		 * itself is enabled/disabled.
		 */
		if (HAS_VEBOX(dev))
			pm_irqs |= PM_VEBOX_USER_INTERRUPT;

		dev_priv->pm_irq_mask = 0xffffffff;
		GEN5_