n_avg) {
		if (dev_priv->ips.cur_delay != dev_priv->ips.min_delay)
			new_delay = dev_priv->ips.cur_delay + 1;
		if (new_delay > dev_priv->ips.min_delay)
			new_delay = dev_priv->ips.min_delay;
	}

	if (ironlake_set_drps(dev, new_delay))
		dev_priv->ips.cur_delay = new_delay;

	spin_unlock(&mchdev_lock);

	return;
}

static void notify_ring(struct intel_engine_cs *ring)
{
	if (!intel_ring_initialized(ring))
		return;

	trace_i915_gem_request_notify(ring);

	wake_up_all(&ring->irq_queue);
}

static void vlv_c0_read(struct drm_i915_private *dev_priv,
			struct intel_rps_ei *ei)
{
	ei->cz_clock = vlv_punit_read(dev_priv, PUNIT_REG_CZ_TIMESTAMP);
	ei->render_c0 = I915_READ(VLV_RENDER_C0_COUNT);
	ei->media_c0 = I915_READ(VLV_MEDIA_C0_COUNT);
}

void gen6_rps_reset_ei(struct drm_i915_private *dev_priv)
{
	memset(&dev_priv->rps.ei, 0, sizeof(dev_priv->rps.ei));
}

static u32 vlv_wa_c0_ei(struct drm_i915_private *dev_priv, u32 pm_iir)
{
	const struct intel_rps_ei *prev = &dev_priv->rps.ei;
	struct intel_rps_ei now;
	u32 events = 0;

	if ((pm_iir & GEN6_PM_RP_UP_EI_EXPIRED) == 0)
		return 0;

	vlv_c0_read(dev_priv, &now);
	if (now.cz_clock == 0)
		return 0;

	if (prev->cz_clock) {
		u64 time, c0;
		unsigned int mul;

		mul = VLV_CZ_CLOCK_TO_MILLI_SEC * 100; /* scale to threshold% */
		if (I915_READ(VLV_COUNTER_CONTROL) & VLV_COUNT_RANGE_HIGH)
			mul <<= 8;

		time = now.cz_clock - prev->cz_clock;
		time *= dev_priv->czclk_freq;

		/* Workload can be split between render + media,
		 * e.g. SwapBuffers being blitted in X after being rendered in
		 * mesa. To account for this we need to combine both engines
		 * into our activity counter.
		 */
		c0 = now.render_c0 - prev->render_c0;
		c0 += now.media_c0 - prev->media_c0;
		c0 *= mul;

		if (c0 > time * dev_priv->rps.up_threshold)
			events = GEN6_PM_RP_UP_THRESHOLD;
		else if (c0 < time * dev_priv->rps.down_threshold)
			events = GEN6_PM_RP_DOWN_THRESHOLD;
	}

	dev_priv->rps.ei = now;
	return events;
}

static bool any_waiters(struct drm_i915_private *dev_priv)
{
	struct intel_engine_cs *ring;
	int i;

	for_each_ring(ring, dev_priv, i)
		if (ring->irq_refcount)
			return true;

	return false;
}

static void gen6_pm_rps_work(struct work_struct *work)
{
	struct drm_i915_private *dev_priv =
		container_of(work, struct drm_i915_private, rps.work);
	bool client_boost;
	int new_delay, adj, min, max;
	u32 pm_iir;

	spin_lock_irq(&dev_priv->irq_lock);
	/* Speed up work cancelation during disabling rps interrupts. */
	if (!dev_priv->rps.interrupts_enabled) {
		spin_unlock_irq(&dev_priv->irq_lock);
		return;
	}
	pm_iir = dev_priv->rps.pm_iir;
	dev_priv->rps.pm_iir = 0;
	/* Make sure not to corrupt PMIMR state used by ringbuffer on GEN6 */
	gen6_enable_pm_irq(dev_priv, dev_priv->pm_rps_events);
	client_boost = dev_priv->rps.client_boost;
	dev_priv->rps.client_boost = false;
	spin_unlock_irq(&dev_priv->irq_lock);

	/* Make sure we didn't queue anything we're not going to process. */
	WARN_ON(pm_iir & ~dev_priv->pm_rps_events);

	if ((pm_iir & dev_priv->pm_rps_events) == 0 && !client_boost)
		return;

	mutex_lock(&dev_priv->rps.hw_lock);

	pm_iir |= vlv_wa_c0_ei(dev_priv, pm_iir);

	adj = dev_priv->rps.last_adj;
	new_delay = dev_priv->rps.cur_freq;
	min = dev_priv->rps.min_freq_softlimit;
	max = dev_priv->rps.max_freq_softlimit;

	if (client_boost) {
		new_delay = dev_priv->rps.max_freq_softlimit;
		adj = 0;
	} else if (pm_iir & GEN6_PM_RP_UP_THRESHOLD) {
		if (adj > 0)
			adj *= 2;
		else /* CHV needs even encode values */
			adj = IS_CHERRYVIEW(dev_priv) ? 2 : 1;
		/*
		 * For better performance, jump directly
		 * to RPe if we're below it.
		 */
		if (new_delay < dev_priv->rps.efficient_freq - adj) {
			new_delay = dev_priv->rps.efficient_freq;
			adj = 0;
		}
	} else if (any_waiters(dev_priv)) {
		adj = 0;
	} else if (pm_iir & GEN6_PM_RP_DOWN_TIMEOUT) {
		if (dev_priv->rps.cur_freq > dev_priv->rps.efficient_freq)
			new_delay = dev_priv->rps.efficient_freq;
		else
			new_delay = dev_priv->rps.min_freq_softlimit;
		adj = 0;
	} else if (pm_iir & GEN6_PM_RP_DOWN_THRESHOLD) {
		if (adj < 0)
			adj *= 2;
		else /* CHV needs even encode values */
			adj = IS_CHERRYVIEW(dev_priv) ? -2 : -1;
	} else { /* unknown event */
		adj = 0;
	}

	dev_priv->rps.last_adj = adj;

	/* sysfs frequency interfaces may have snuck in while servicing the
	 * interrupt
	 */
	new_delay += adj;
	new_delay = clamp_t(int, new_delay, min, max);

	intel_set_rps(dev_priv->dev, new_delay);

	mutex_unlock(&dev_priv->rps.hw_lock);
}


/**
 * ivybridge_parity_work - Workqueue called when a parity error interrupt
 * occurred.
 * @work: workqueue struct
 *
 * Doesn't actually do anything except notify userspace. As a consequence of
 * this event, userspace should try to remap the bad rows since statistically
 * it is likely the same row is more likely to go bad again.
 */
static void ivybridge_parity_work(struct work_struct *work)
{
	struct drm_i915_private *dev_priv =
		container_of(work, struct drm_i915_private, l3_parity.error_work);
	u32 error_status, row, bank, subbank;
	char *parity_event[6];
	uint32_t misccpctl;
	uint8_t slice = 0;

	/* We must turn off DOP level clock gating to access the L3 registers.
	 * In order to prevent a get/put style interface, acquire struct mutex
	 * any time we access those registers.
	 */
	mutex_lock(&dev_priv->dev->struct_mutex);

	/* If we've screwed up tracking, just let the interrupt fire again */
	if (WARN_ON(!dev_priv->l3_parity.which_slice))
		goto out;

	misccpctl = I915_READ(GEN7_MISCCPCTL);
	I915_WRITE(GEN7_MISCCPCTL, misccpctl & ~GEN7_DOP_CLOCK_GATE_ENABLE);
	POSTING_READ(GEN7_MISCCPCTL);

	while ((slice = ffs(dev_priv->l3_parity.which_slice)) != 0) {
		u32 reg;

		slice--;
		if (WARN_ON_ONCE(slice >= NUM_L3_SLICES(dev_priv->dev)))
			break;

		dev_priv->l3_parity.which_slice &= ~(1<<slice);

		reg = GEN7_L3CDERRST1 + (slice * 0x200);

		error_status = I915_READ(reg);
		row = GEN7_PARITY_ERROR_ROW(error_status);
		bank = GEN7_PARITY_ERROR_BANK(error_status);
		subbank = GEN7_PARITY_ERROR_SUBBANK(error_status);

		I915_WRITE(reg, GEN7_PARITY_ERROR_VALID | GEN7_L3CDERRST1_ENABLE);
		POSTING_READ(reg);

		parity_event[0] = I915_L3_PARITY_UEVENT "=1";
		parity_event[1] = kasprintf(GFP_KERNEL, "ROW=%d", row);
		parity_event[2] = kasprintf(GFP_KERNEL, "BANK=%d", bank);
		parity_event[3] = kasprintf(GFP_KERNEL, "SUBBANK=%d", subbank);
		parity_event[4] = kasprintf(GFP_KERNEL, "SLICE=%d", slice);
		parity_event[5] = NULL;

		kobject_uevent_env(&dev_priv->dev->primary->kdev->kobj,
				   KOBJ_CHANGE, parity_event);

		DRM_DEBUG("Parity error: Slice = %d, Row = %d, Bank = %d, Sub bank = %d.\n",
			  slice, row, bank, subbank);

		kfree(parity_event[4]);
		kfree(parity_event[3]);
		kfree(parity_event[2]);
		kfree(parity_event[1]);
	}

	I915_WRITE(GEN7_MISCCPCTL, misccpctl);

out:
	WARN_ON(dev_priv->l3_parity.which_slice);
	spin_lock_irq(&dev_priv->irq_lock);
	gen5_enable_gt_irq(dev_priv, GT_PARITY_ERROR(dev_priv->dev));
	spin_unlock_irq(&dev_priv->irq_lock);

	mutex_unlock(&dev_priv->dev->struct_mutex);
}

static void ivybridge_parity_error_irq_handler(struct drm_device *dev, u32 iir)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (!HAS_L3_DPF(dev))
		return;

	spin_lock(&dev_priv->irq_lock);
	gen5_disable_gt_irq(dev_priv, GT_PARITY_ERROR(dev));
	spin_unlock(&dev_priv->irq_lock);

	iir &= GT_PARITY_ERROR(dev);
	if (iir & GT_RENDER_L3_PARITY_ERROR_INTERRUPT_S1)
		dev_priv->l3_parity.which_slice |= 1 << 1;

	if (iir & GT_RENDER_L3_PARITY_ERROR_INTERRUPT)
		dev_priv->l3_parity.which_slice |= 1 << 0;

	queue_work(dev_priv->wq, &dev_priv->l3_parity.error_work);
}

static void ilk_gt_irq_handler(struct drm_device *dev,
			       struct drm_i915_private *dev_priv,
			       u32 gt_iir)
{
	if (gt_iir &
	    (GT_RENDER_USER_INTERRUPT | GT_RENDER_PIPECTL_NOTIFY_INTERRUPT))
		notify_ring(&dev_priv->ring[RCS]);
	if (gt_iir & ILK_BSD_USER_INTERRUPT)
		notify_ring(&dev_priv->ring[VCS]);
}

static void snb_gt_irq_handler(struct drm_device *dev,
			       struct drm_i915_private *dev_priv,
			       u32 gt_iir)
{

	if (gt_iir &
	    (GT_RENDER_USER_INTERRUPT | GT_RENDER_PIPECTL_NOTIFY_INTERRUPT))
		notify_ring(&dev_priv->ring[RCS]);
	if (gt_iir & GT_BSD_USER_INTERRUPT)
		notify_ring(&dev_priv->ring[VCS]);
	if (gt_iir & GT_BLT_USER_INTERRUPT)
		notify_ring(&dev_priv->ring[BCS]);

	if (gt_iir & (GT_BLT_CS_ERROR_INTERRUPT |
		      GT_BSD_CS_ERROR_INTERRUPT |
		      GT_RENDER_CS_MASTER_ERROR_INTERRUPT))
		DRM_DEBUG("Command parser error, gt_iir 0x%08x\n", gt_iir);

	if (gt_iir & GT_PARITY_ERROR(dev))
		ivybridge_parity_error_irq_handler(dev, gt_iir);
}

static irqreturn_t gen8_gt_irq_handler(struct drm_i915_private *dev_priv,
				       u32 master_ctl)
{
	irqreturn_t ret = IRQ_NONE;

	if (master_ctl & (GEN8_GT_RCS_IRQ | GEN8_GT_BCS_IRQ)) {
		u32 tmp = I915_READ_FW(GEN8_GT_IIR(0));
		if (tmp) {
			I915_WRITE_FW(GEN8_GT_IIR(0), tmp);
			ret = IRQ_HANDLED;

			if (tmp & (GT_CONTEXT_SWITCH_INTERRUPT << GEN8_RCS_IRQ_SHIFT))
				intel_lrc_irq_handler(&dev_priv->ring[RCS]);
			if (tmp & (GT_RENDER_USER_INTERRUPT << GEN8_RCS_IRQ_SHIFT))
				notify_ring(&dev_priv->ring[RCS]);

			if (tmp & (GT_CONTEXT_SWITCH_INTERRUPT << GEN8_BCS_IRQ_SHIFT))
				intel_lrc_irq_handler(&dev_priv->ring[BCS]);
			if (tmp & (GT_RENDER_USER_INTERRUPT << GEN8_BCS_IRQ_SHIFT))
				notify_ring(&dev_priv->ring[BCS]);
		} else
			DRM_ERROR("The master control interrupt lied (GT0)!\n");
	}

	if (master_ctl & (GEN8_GT_VCS1_IRQ | GEN8_GT_VCS2_IRQ)) {
		u32 tmp = I915_READ_FW(GEN8_GT_IIR(1));
		if (tmp) {
			I915_WRITE_FW(GEN8_GT_IIR(1), tmp);
			ret = IRQ_HANDLED;

			if (tmp & (GT_CONTEXT_SWITCH_INTERRUPT << GEN8_VCS1_IRQ_SHIFT))
				intel_lrc_irq_handler(&dev_priv->ring[VCS]);
			if (tmp & (GT_RENDER_USER_INTERRUPT << GEN8_VCS1_IRQ_SHIFT))
				notify_ring(&dev_priv->ring[VCS]);

			if (tmp & (GT_CONTEXT_SWITCH_INTERRUPT << GEN8_VCS2_IRQ_SHIFT))
				intel_lrc_irq_handler(&dev_priv->ring[VCS2]);
			if (tmp & (GT_RENDER_USER_INTERRUPT << GEN8_VCS2_IRQ_SHIFT))
				notify_ring(&dev_priv->ring[VCS2]);
		} else
			DRM_ERROR("The master control interrupt lied (GT1)!\n");
	}

	if (master_ctl & GEN8_GT_VECS_IRQ) {
		u32 tmp = I915_READ_FW(GEN8_GT_IIR(3));
		if (tmp) {
			I915_WRITE_FW(GEN8_GT_IIR(3), tmp);
			ret = IRQ_HANDLED;

			if (tmp & (GT_CONTEXT_SWITCH_INTERRUPT << GEN8_VECS_IRQ_SHIFT))
				intel_lrc_irq_handler(&dev_priv->ring[VECS]);
			if (tmp & (GT_RENDER_USER_INTERRUPT << GEN8_VECS_IRQ_SHIFT))
				notify_ring(&dev_priv->ring[VECS]);
		} else
			DRM_ERROR("The master control interrupt lied (GT3)!\n");
	}

	if (master_ctl & GEN8_GT_PM_IRQ) {
		u32 tmp = I915_READ_FW(GEN8_GT_IIR(2));
		if (tmp & dev_priv->pm_rps_events) {
			I915_WRITE_FW(GEN8_GT_IIR(2),
				      tmp & dev_priv->pm_rps_events);
			ret = IRQ_HANDLED;
			gen6_rps_irq_handler(dev_priv, tmp);
		} else
			DRM_ERROR("The master control interrupt lied (PM)!\n");
	}

	return ret;
}

static bool bxt_port_hotplug_long_detect(enum port port, u32 val)
{
	switch (port) {
	case PORT_A:
		return val & PORTA_HOTPLUG_LONG_DETECT;
	case PORT_B:
		return val & PORTB_HOTPLUG_LONG_DETECT;
	case PORT_C:
		return val & PORTC_HOTPLUG_LONG_DETECT;
	default:
		return false;
	}
}

static bool spt_port_hotplug2_long_detect(enum port port, u32 val)
{
	switch (port) {
	case PORT_E:
		return val & PORTE_HOTPLUG_LONG_DETECT;
	default:
		return false;
	}
}

static bool spt_port_hotplug_long_detect(enum port port, u32 val)
{
	switch (port) {
	case PORT_A:
		return val & PORTA_HOTPLUG_LONG_DETECT;
	case PORT_B:
		return val & PORTB_HOTPLUG_LONG_DETECT;
	case PORT_C:
		return val & PORTC_HOTPLUG_LONG_DETECT;
	case PORT_D:
		return val & PORTD_HOTPLUG_LONG_DETECT;
	default:
		return false;
	}
}

static bool ilk_port_hotplug_long_detect(enum port port, u32 val)
{
	switch (port) {
	case PORT_A:
		return val & DIGITAL_PORTA_HOTPLUG_LONG_DETECT;
	default:
		return false;
	}
}

static bool pch_port_hotplug_long_detect(enum port port, u32 val)
{
	switch (port) {
	case PORT_B:
		return val & PORTB_HOTPLUG_LONG_DETECT;
	case PORT_C:
		return val & PORTC_HOTPLUG_LONG_DETECT;
	case PORT_D:
		return val & PORTD_HOTPLUG_LONG_DETECT;
	default:
		return false;
	}
}

static bool i9xx_port_hotplug_long_detect(enum port port, u32 val)
{
	switch (port) {
	case PORT_B:
		return val & PORTB_HOTPLUG_INT_LONG_PULSE;
	case PORT_C:
		return val & PORTC_HOTPLUG_INT_LONG_PULSE;
	case PORT_D:
		return val & PORTD_HOTPLUG_INT_LONG_PULSE;
	default:
		return false;
	}
}

/*
 * Get a bit mask of pins that have triggered, and which ones may be long.
 * This can be called multiple times with the same masks to accumulate
 * hotplug detection results from several registers.
 *
 * Note that the caller is expected to zero out the masks initially.
 */
static void intel_get_hpd_pins(u32 *pin_mask, u32 *long_mask,
			     u32 hotplug_trigger, u32 dig_hotplug_reg,
			     const u32 hpd[HPD_NUM_PINS],
			     bool long_pulse_detect(enum port port, u32 val))
{
	enum port port;
	int i;

	for_each_hpd_pin(i) {
		if ((hpd[i] & hotplug_trigger) == 0)
			continue;

		*pin_mask |= BIT(i);

		if (!intel_hpd_pin_to_port(i, &port))
			continue;

		if (long_pulse_detect(port, dig_hotplug_reg))
			*long_mask |= BIT(i);
	}

	DRM_DEBUG_DRIVER("hotplug event received, stat 0x%08x, dig 0x%08x, pins 0x%08x\n",
			 hotplug_trigger, dig_hotplug_reg, *pin_mask);

}

static void gmbus_irq_handler(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	wake_up_all(&dev_priv->gmbus_wait_queue);
}

static void dp_aux_irq_handler(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	wake_up_all(&dev_priv->gmbus_wait_queue);
}

#if defined(CONFIG_DEBUG_FS)
static void display_pipe_crc_irq_handler(struct drm_device *dev, enum pipe pipe,
					 uint32_t crc0, uint32_t crc1,
					 uint32_t crc2, uint32_t crc3,
					 uint32_t crc4)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_pipe_crc *pipe_crc = &dev_priv->pipe_crc[pipe];
	struct intel_pipe_crc_entry *entry;
	int head, tail;

	spin_lock(&pipe_crc->lock);

	if (!pipe_crc->entries) {
		spin_unlock(&pipe_crc->lock);
		DRM_DEBUG_KMS("spurious interrupt\n");
		return;
	}

	head = pipe_crc->head;
	tail = pipe_crc->tail;

	if (CIRC_SPACE(head, tail, INTEL_PIPE_CRC_ENTRIES_NR) < 1) {
		spin_unlock(&pipe_crc->lock);
		DRM_ERROR("CRC buffer overflowing\n");
		return;
	}

	entry = &pipe_crc->entries[head];

	entry->frame = dev->driver->get_vblank_counter(dev, pipe);
	entry->crc[0] = crc0;
	entry->crc[1] = crc1;
	entry->crc[2] = crc2;
	entry->crc[3] = crc3;
	entry->crc[4] = crc4;

	head = (head + 1) & (INTEL_PIPE_CRC_ENTRIES_NR - 1);
	pipe_crc->head = head;

	spin_unlock(&pipe_crc->lock);

	wake_up_interruptible(&pipe_crc->wq);
}
#else
static inline void
display_pipe_crc_irq_handler(struct drm_device *dev, enum pipe pipe,
			     uint32_t crc0, uint32_t crc1,
			     uint32_t crc2, uint32_t crc3,
			     uint32_t crc4) {}
#endif


static void hsw_pipe_crc_irq_handler(struct drm_device *dev, enum pipe pipe)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	display_pipe_crc_irq_handler(dev, pipe,
				     I915_READ(PIPE_CRC_RES_1_IVB(pipe)),
				     0, 0, 0, 0);
}

static void ivb_pipe_crc_irq_handler(struct drm_device *dev, enum pipe pipe)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	display_pipe_crc_irq_handler(dev, pipe,
				     I915_READ(PIPE_CRC_RES_1_IVB(pipe)),
				     I915_READ(PIPE_CRC_RES_2_IVB(pipe)),
				     I915_READ(PIPE_CRC_RES_3_IVB(pipe)),
				     I915_READ(PIPE_CRC_RES_4_IVB(pipe)),
				     I915_READ(PIPE_CRC_RES_5_IVB(pipe)));
}

static void i9xx_pipe_crc_irq_handler(struct drm_device *dev, enum pipe pipe)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint32_t res1, res2;

	if (INTEL_INFO(dev)->gen >= 3)
		res1 = I915_READ(PIPE_CRC_RES_RES1_I915(pipe));
	else
		res1 = 0;

	if (INTEL_INFO(dev)->gen >= 5 || IS_G4X(dev))
		res2 = I915_READ(PIPE_CRC_RES_RES2_G4X(pipe));
	else
		res2 = 0;

	display_pipe_crc_irq_handler(dev, pipe,
				     I915_READ(PIPE_CRC_RES_RED(pipe)),
				     I915_READ(PIPE_CRC_RES_GREEN(pipe)),
				     I915_READ(PIPE_CRC_RES_BLUE(pipe)),
				     res1, res2);
}

/* The RPS events need forcewake, so we add them to a work queue and mask their
 * IMR bits until the work is done. Other interrupts can be processed without
 * the work queue. */
static void gen6_rps_irq_handler(struct drm_i915_private *dev_priv, u32 pm_iir)
{
	if (pm_iir & dev_priv->pm_rps_events) {
		spin_lock(&dev_priv->irq_lock);
		gen6_disable_pm_irq(dev_priv, pm_iir & dev_priv->pm_rps_events);
		if (dev_priv->rps.interrupts_enabled) {
			dev_priv->rps.pm_iir |= pm_iir & dev_priv->pm_rps_events;
			queue_work(dev_priv->wq, &dev_priv->rps.work);
		}
		spin_unlock(&dev_priv->irq_lock);
	}

	if (INTEL_INFO(dev_priv)->gen >= 8)
		return;

	if (HAS_VEBOX(dev_priv->dev)) {
		if (pm_iir & PM_VEBOX_USER_INTERRUPT)
			notify_ring(&dev_priv->ring[VECS]);

		if (pm_iir & PM_VEBOX_CS_ERROR_INTERRUPT)
			DRM_DEBUG("Command parser error, pm_iir 0x%08x\n", pm_iir);
	}
}

static bool intel_pipe_handle_vblank(struct drm_device *dev, enum pipe pipe)
{
	if (!drm_handle_vblank(dev, pipe))
		return false;

	return true;
}

static void valleyview_pipestat_irq_handler(struct drm_device *dev, u32 iir)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 pipe_stats[I915_MAX_PIPES] = { };
	int pipe;

	spin_lock(&dev_priv->irq_lock);
	for_each_pipe(dev_priv, pipe) {
		int reg;
		u32 mask, iir_bit = 0;

		/*
		 * PIPESTAT bits get signalled even when the interrupt is
		 * disabled with the mask bits, and some of the status bits do
		 * not generate interrupts at all (like the underrun bit). Hence
		 * we need to be careful that we only handle what we want to
		 * handle.
		 */

		/* fifo underruns are filterered in the underrun handler. */
		mask = PIPE_FIFO_UNDERRUN_STATUS;

		switch (pipe) {
		case PIPE_A:
			iir_bit = I915_DISPLAY_PIPE_A_EVENT_INTERRUPT;
			break;
		case PIPE_B:
			iir_bit = I915_DISPLAY_PIPE_B_EVENT_INTERRUPT;
			break;
		case PIPE_C:
			iir_bit = I915_DISPLAY_PIPE_C_EVENT_INTERRUPT;
			break;
		}
		if (iir & iir_bit)
			mask |= dev_priv->pipestat_irq_mask[pipe];

		if (!mask)
			continue;

		reg = PIPESTAT(pipe);
		mask |= PIPESTAT_INT_ENABLE_MASK;
		pipe_stats[pipe] = I915_READ(reg) & mask;

		/*
		 * Clear the PIPE*STAT regs before the IIR
		 */
		if (pipe_stats[pipe] & (PIPE_FIFO_UNDERRUN_STATUS |
					PIPESTAT_INT_STATUS_MASK))
			I915_WRITE(reg, pipe_stats[pipe]);
	}
	spin_unlock(&dev_priv->irq_lock);

	for_each_pipe(dev_priv, pipe) {
		if (pipe_stats[pipe] & PIPE_START_VBLANK_INTERRUPT_STATUS &&
		    intel_pipe_handle_vblank(dev, pipe))
			intel_check_page_flip(dev, pipe);

		if (pipe_stats[pipe] & PLANE_FLIP_DONE_INT_STATUS_VLV) {
			intel_prepare_page_flip(dev, pipe);
			intel_finish_page_flip(dev, pipe);
		}

		if (pipe_stats[pipe] & PIPE_CRC_DONE_INTERRUPT_STATUS)
			i9xx_pipe_crc_irq_handler(dev, pipe);

		if (pipe_stats[pipe] & PIPE_FIFO_UNDERRUN_STATUS)
			intel_cpu_fifo_underrun_irq_handler(dev_priv, pipe);
	}

	if (pipe_stats[0] & PIPE_GMBUS_INTERRUPT_STATUS)
		gmbus_irq_handler(dev);
}

static void i9xx_hpd_irq_handler(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 hotplug_status = I915_READ(PORT_HOTPLUG_STAT);
	u32 pin_mask = 0, long_mask = 0;

	if (!hotplug_status)
		return;

	I915_WRITE(PORT_HOTPLUG_STAT, hotplug_status);
	/*
	 * Make sure hotplug status is cleared before we clear IIR, or else we
	 * may miss hotplug events.
	 */
	POSTING_READ(PORT_HOTPLUG_STAT);

	if (IS_G4X(dev) || IS_VALLEYVIEW(dev)) {
		u32 hotplug_trigger = hotplug_status & HOTPLUG_INT_STATUS_G4X;

		if (hotplug_trigger) {
			intel_get_hpd_pins(&pin_mask, &long_mask, hotplug_trigger,
					   hotplug_trigger, hpd_status_g4x,
					   i9xx_port_hotplug_long_detect);

			intel_hpd_irq_handler(dev, pin_mask, long_mask);
		}

		if (hotplug_status & DP_AUX_CHANNEL_MASK_INT_STATUS_G4X)
			dp_aux_irq_handler(dev);
	} else {
		u32 hotplug_trigger = hotplug_status & HOTPLUG_INT_STATUS_I915;

		if (hotplug_trigger) {
			intel_get_hpd_pins(&pin_mask, &long_mask, hotplug_trigger,
					   hotplug_trigger, hpd_status_i915,
					   i9xx_port_hotplug_long_detect);
			intel_hpd_irq_handler(dev, pin_mask, long_mask);
		}
	}
}

static irqreturn_t valleyview_irq_handler(int irq, void *arg)
{
	struct drm_device *dev = arg;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 iir, gt_iir, pm_iir;
	irqreturn_t ret = IRQ_NONE;

	if (!intel_irqs_enabled(dev_priv))
		return IRQ_NONE;

	while (true) {
		/* Find, clear, then process each source of interrupt */

		gt_iir = I915_READ(GTIIR);
		if (gt_iir)
			I915_WRITE(GTIIR, gt_iir);

		pm_iir = I915_READ(GEN6_PMIIR);
		if (pm_iir)
			I915_WRITE(GEN6_PMIIR, pm_iir);

		iir = I915_READ(VLV_IIR);
		if (iir) {
			/* Consume port before clearing IIR or we'll miss events */
			if (iir & I915_DISPLAY_PORT_INTERRUPT)
				i9xx_hpd_irq_handler(dev);
			I915_WRITE(VLV_IIR, iir);
		}

		if (gt_iir == 0 && pm_iir == 0 && iir == 0)
			goto out;

		ret = IRQ_HANDLED;

		if (gt_iir)
			snb_gt_irq_handler(dev, dev_priv, gt_iir);
		if (pm_iir)
			gen6_rps_irq_handler(dev_priv, pm_iir);
		/* Call regardless, as some status bits might not be
		 * signalled in iir */
		valleyview_pipestat_irq_handler(dev, iir);
	}

out:
	return ret;
}

static irqreturn_t cherryview_irq_handler(int irq, void *arg)
{
	struct drm_device *dev = arg;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 master_ctl, iir;
	irqreturn_t ret = IRQ_NONE;

	if (!intel_irqs_enabled(dev_priv))
		return IRQ_NONE;

	for (;;) {
		master_ctl = I915_READ(GEN8_MASTER_IRQ) & ~GEN8_MASTER_IRQ_CONTROL;
		iir = I915_READ(VLV_IIR);

		if (master_ctl == 0 && iir == 0)
			break;

		ret = IRQ_HANDLED;

		I915_WRITE(GEN8_MASTER_IRQ, 0);

		/* Find, clear, then process each source of interrupt */

		if (iir) {
			/* Consume port before clearing IIR or we'll miss events */
			if (iir & I915_DISPLAY_PORT_INTERRUPT)
				i9xx_hpd_irq_handler(dev);
			I915_WRITE(VLV_IIR, iir);
		}

		gen8_gt_irq_handler(dev_priv, master_ctl);

		/* Call regardless, as some status bits might not be
		 * signalled in iir */
		valleyview_pipestat_irq_handler(dev, iir);

		I915_WRITE(GEN8_MASTER_IRQ, DE_MASTER_IRQ_CONTROL);
		POSTING_READ(GEN8_MASTER_IRQ);
	}

	return ret;
}

static void ibx_hpd_irq_handler(struct drm_device *dev, u32 hotplug_trigger,
				const u32 hpd[HPD_NUM_PINS])
{
	struct drm_i915_private *dev_priv = to_i915(dev);
	u32 dig_hotplug_reg, pin_mask = 0, long_mask = 0;

	dig_hotplug_reg = I915_READ(PCH_PORT_HOTPLUG);
	I915_WRITE(PCH_PORT_HOTPLUG, dig_hotplug_reg);

	intel_get_hpd_pins(&pin_mask, &long_mask, hotplug_trigger,
			   dig_hotplug_reg, hpd,
			   pch_port_hotplug_long_detect);

	intel_hpd_irq_handler(dev, pin_mask, long_mask);
}

static void ibx_irq_handler(struct drm_device *dev, u32 pch_iir)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int pipe;
	u32 hotplug_trigger = pch_iir & SDE_HOTPLUG_MASK;

	if (hotplug_trigger)
		ibx_hpd_irq_handler(dev, hotplug_trigger, hpd_ibx);

	if (pch_iir & SDE_AUDIO_POWER_MASK) {
		int port = ffs((pch_iir & SDE_AUDIO_POWER_MASK) >>
			       SDE_AUDIO_POWER_SHIFT);
		DRM_DEBUG_DRIVER("PCH audio power change on port %d\n",
				 port_name(port));
	}

	if (pch_iir & SDE_AUX_MASK)
		dp_aux_irq_handler(dev);

	if (pch_iir & SDE_GMBUS)
		gmbus_irq_handler(dev);

	if (pch_iir & SDE_AUDIO_HDCP_MASK)
		DRM_DEBUG_DRIVER("PCH HDCP audio interrupt\n");

	if (pch_iir & SDE_AUDIO_TRANS_MASK)
		DRM_DEBUG_DRIVER("PCH transcoder audio interrupt\n");

	if (pch_iir & SDE_POISON)
		DRM_ERROR("PCH poison interrupt\n");

	if (pch_iir & SDE_FDI_MASK)
		for_each_pipe(dev_priv, pipe)
			DRM_DEBUG_DRIVER("  pipe %c FDI IIR: 0x%08x\n",
					 pipe_name(pipe),
					 I915_READ(FDI_RX_IIR(pipe)));

	if (pch_iir & (SDE_TRANSB_CRC_DONE | SDE_TRANSA_CRC_DONE))
		DRM_DEBUG_DRIVER("PCH transcoder CRC done interrupt\n");

	if (pch_iir & (SDE_TRANSB_CRC_ERR | SDE_TRANSA_CRC_ERR))
		DRM_DEBUG_DRIVER("PCH transcoder CRC error interrupt\n");

	if (pch_iir & SDE_TRANSA_FIFO_UNDER)
		intel_pch_fifo_underrun_irq_handler(dev_priv, TRANSCODER_A);

	if (pch_iir & SDE_TRANSB_FIFO_UNDER)
		intel_pch_fifo_underrun_irq_handler(dev_priv, TRANSCODER_B);
}

static void ivb_err_int_handler(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 err_int = I915_READ(GEN7_ERR_INT);
	enum pipe pipe;

	if (err_int & ERR_INT_POISON)
		DRM_ERROR("Poison interrupt\n");

	for_each_pipe(dev_priv, pipe) {
		if (err_int & ERR_INT_FIFO_UNDERRUN(pipe))
			intel_cpu_fifo_underrun_irq_handler(dev_priv, pipe);

		if (err_int & ERR_INT_PIPE_CRC_DONE(pipe)) {
			if (IS_IVYBRIDGE(dev))
				ivb_pipe_crc_irq_handler(dev, pipe);
			else
				hsw_pipe_crc_irq_handler(dev, pipe);
		}
	}

	I915_WRITE(GEN7_ERR_INT, err_int);
}

static void cpt_serr_int_handler(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 serr_int = I915_READ(SERR_INT);

	if (serr_int & SERR_INT_POISON)
		DRM_ERROR("PCH poison interrupt\n");

	if (serr_int & SERR_INT_TRANS_A_FIFO_UNDERRUN)
		intel_pch_fifo_underrun_irq_handler(dev_priv, TRANSCODER_A);

	if (serr_int & SERR_INT_TRANS_B_FIFO_UNDERRUN)
		intel_pch_fifo_underrun_irq_handler(dev_priv, TRANSCODER_B);

	if (serr_int & SERR_INT_TRANS_C_FIFO_UNDERRUN)
		intel_pch_fifo_underrun_irq_handler(dev_priv, TRANSCODER_C);

	I915_WRITE(SERR_INT, serr_int);
}

static void cpt_irq_handler(struct drm_device *dev, u32 pch_iir)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int pipe;
	u32 hotplug_trigger = pch_iir & SDE_HOTPLUG_MASK_CPT;

	if (hotplug_trigger)
		ibx_hpd_irq_handler(dev, hotplug_trigger, hpd_cpt);

	if (pch_iir & SDE_AUDIO_POWER_MASK_CPT) {
		int port = ffs((pch_iir & SDE_AUDIO_POWER_MASK_CPT) >>
			       SDE_AUDIO_POWER_SHIFT_CPT);
		DRM_DEBUG_DRIVER("PCH audio power change on port %c\n",
				 port_name(port));
	}

	if (pch_iir & SDE_AUX_MASK_CPT)
		dp_aux_irq_handler(dev);

	if (pch_iir & SDE_GMBUS_CPT)
		gmbus_irq_handler(dev);

	if (pch_iir & SDE_AUDIO_CP_REQ_CPT)
		DRM_DEBUG_DRIVER("Audio CP request interrupt\n");

	if (pch_iir & SDE_AUDIO_CP_CHG_CPT)
		DRM_DEBUG_DRIVER("Audio CP change interrupt\n");

	if (pch_iir & SDE_FDI_MASK_CPT)
		for_each_pipe(dev_priv, pipe)
			DRM_DEBUG_DRIVER("  pipe %c FDI IIR: 0x%08x\n",
					 pipe_name(pipe),
					 I915_READ(FDI_RX_IIR(pipe)));

	if (pch_iir & SDE_ERROR_CPT)
		cpt_serr_int_handler(dev);
}

static void spt_irq_handler(struct drm_device *dev, u32 pch_iir)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 hotplug_trigger = pch_iir & SDE_HOTPLUG_MASK_SPT &
		~SDE_PORTE_HOTPLUG_SPT;
	u32 hotplug2_trigger = pch_iir & SDE_PORTE_HOTPLUG_SPT;
	u32 pin_mask = 0, long_mask = 0;

	if (hotplug_trigger) {
		u32 dig_hotplug_reg;

		dig_hotplug_reg = I915_READ(PCH_PORT_HOTPLUG);
		I915_WRITE(PCH_PORT_HOTPLUG, dig_hotplug_reg);

		intel_get_hpd_pins(&pin_mask, &long_mask, hotplug_trigger,
				   dig_hotplug_reg, hpd_spt,
				   spt_port_hotplug_long_detect);
	}

	if (hotplug2_trigger) {
		u32 dig_hotplug_reg;

		dig_hotplug_reg = I915_READ(PCH_PORT_HOTPLUG2);
		I915_WRITE(PCH_PORT_HOTPLUG2, dig_hotplug_reg);

		intel_get_hpd_pins(&pin_mask, &long_mask, hotplug2_trigger,
				   dig_hotplug_reg, hpd_spt,
				   spt_port_hotplug2_long_detect);
	}

	if (pin_mask)
		intel_hpd_irq_handler(dev, pin_mask, long_mask);

	if (pch_iir & SDE_GMBUS_CPT)
		gmbus_irq_handler(dev);
}

static void ilk_hpd_irq_handler(struct drm_device *dev, u32 hotplug_trigger,
				const u32 hpd[HPD_NUM_PINS])
{
	struct drm_i915_private *dev_priv = to_i915(dev);
	u32 dig_hotplug_reg, pin_mask = 0, long_mask = 0;

	dig_hotplug_reg = I915_READ(DIGITAL_PORT_HOTPLUG_CNTRL);
	I915_WRITE(DIGITAL_PORT_HOTPLUG_CNTRL, dig_hotplug_reg);

	intel_get_hpd_pins(&pin_mask, &long_mask, hotplug_trigger,
			   dig_hotplug_reg, hpd,
			   ilk_port_hotplug_long_detect);

	intel_hpd_irq_handler(dev, pin_mask, long_mask);
}

static void ilk_display_irq_handler(struct drm_device *dev, u32 de_iir)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	enum pipe pipe;
	u32 hotplug_trigger = de_iir & DE_DP_A_HOTPLUG;

	if (hotplug_trigger)
		ilk_hpd_irq_handler(dev, hotplug_trigger, hpd_ilk);

	if (de_iir & DE_AUX_CHANNEL_A)
		dp_aux_irq_handler(dev);

	if (de_iir & DE_GSE)
		intel_opregion_asle_intr(dev);

	if (de_iir & DE_POISON)
		DRM_ERROR("Poison interrupt\n");

	for_each_pipe(dev_priv, pipe) {
		if (de_iir & DE_PIPE_VBLANK(pipe) &&
		    intel_pipe_handle_vblank(dev, pipe))
			intel_check_page_flip(dev, pipe);

		if (de_iir & DE_PIPE_FIFO_UNDERRUN(pipe))
			intel_cpu_fifo_underrun_irq_handler(dev_priv, pipe);

		if (de_iir & DE_PIPE_CRC_DONE(pipe))
			i9xx_pipe_crc_irq_handler(dev, pipe);

		/* plane/pipes map 1:1 on ilk+ */
		if (de_iir & DE_PLANE_FLIP_DONE(pipe)) {
			intel_prepare_page_flip(dev, pipe);
			intel_finish_page_flip_plane(dev, pipe);
		}
	}

	/* check event from PCH */
	if (de_iir & DE_PCH_EVENT) {
		u32 pch_iir = I915_READ(SDEIIR);

		if (HAS_PCH_CPT(dev))
			cpt_irq_handler(dev, pch_iir);
		else
			ibx_irq_handler(dev, pch_iir);

		/* should clear PCH hotplug event before clear CPU irq */
		I915_WRITE(SDEIIR, pch_iir);
	}

	if (IS_GEN5(dev) && de_iir & DE_PCU_EVENT)
		ironlake_rps_change_irq_handler(dev);
}

static void ivb_display_irq_handler(struct drm_device *dev, u32 de_iir)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	enum pipe pipe;
	u32 hotplug_trigger = de_iir & DE_DP_A_HOTPLUG_IVB;

	if (hotplug_trigger)
		ilk_hpd_irq_handler(dev, hotplug_trigger, hpd_ivb);

	if (de_iir & DE_ERR_INT_IVB)
		ivb_err_int_handler(dev);

	if (de_iir & DE_AUX_CHANNEL_A_IVB)
		dp_aux_irq_handler(dev);

	if (de_iir & DE_GSE_IVB)
		intel_opregion_asle_intr(dev);

	for_each_pipe(dev_priv, pipe) {
		if (de_iir & (DE_PIPE_VBLANK_IVB(pipe)) &&
		    intel_pipe_handle_vblank(dev, pipe))
			intel_check_page_flip(dev, pipe);

		/* plane/pipes map 1:1 on ilk+ */
		if (de_iir & DE_PLANE_FLIP_DONE_IVB(pipe)) {
			intel_prepare_page_flip(dev, pipe);
			intel_finish_page_flip_plane(dev, pipe);
		}
	}

	/* check event from PCH */
	if (!HAS_PCH_NOP(dev) && (de_iir & DE_PCH_EVENT_IVB)) {
		u32 pch_iir = I915_READ(SDEIIR);

		cpt_irq_handler(dev, pch_iir);

		/* clear PCH hotplug event before clear CPU irq */
		I915_WRITE(SDEIIR, pch_iir);
	}
}

/*
 * To handle irqs with the minimum potential races with fresh interrupts, we:
 * 1 - Disable Master Interrupt Control.
 * 2 - Find the source(s) of the interrupt.
 * 3 - Clear the Interrupt Identity bits (IIR).
 * 4 - Process the interrupt(s) that had bits set in the IIRs.
 * 5 - Re-enable Master Interrupt Control.
 */
static irqreturn_t ironlake_irq_handler(int irq, void *arg)
{
	struct drm_device *dev = arg;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 de_iir, gt_iir, de_ier, sde_ier = 0;
	irqreturn_t ret = IRQ_NONE;

	if (!intel_irqs_enabled(dev_priv))
		return IRQ_NONE;

	/* We get interrupts on unclaimed registers, so check for this before we
	 * do any I915_{READ,WRITE}. */
	intel_uncore_check_errors(dev);

	/* disable master interrupt before clearing iir  */
	de_ier = I915_READ(DEIER);
	I915_WRITE(DEIER, de_ier & ~DE_MASTER_IRQ_CONTROL);
	POSTING_READ(DEIER);

	/* Disable south interrupts. We'll only write to SDEIIR once, so further
	 * interrupts will will be stored on its back queue, and then we'll be
	 * able to process them after we restore SDEIER (as soon as we restore
	 * it, we'll get an interrupt if SDEIIR still has something to process
	 * due to its back queue). */
	if (!HAS_PCH_NOP(dev)) {
		sde_ier = I915_READ(SDEIER);
		I915_WRITE(SDEIER, 0);
		POSTING_READ(SDEIER);
	}

	/* Find, clear, then process each source of interrupt */

	gt_iir = I915_READ(GTIIR);
	if (gt_iir) {
		I915_WRITE(GTIIR, gt_iir);
		ret = IRQ_HANDLED;
		if (INTEL_INFO(dev)->gen >= 6)
			snb_gt_irq_handler(dev, dev_priv, gt_iir);
		else
			ilk_gt_irq_handler(dev, dev_priv, gt_iir);
	}

	de_iir = I915_READ(DEIIR);
	if (de_iir) {
		I915_WRITE(DEIIR, de_iir);
		ret = IRQ_HANDLED;
		if (INTEL_INFO(dev)->gen >= 7)
			ivb_display_irq_handler(dev, de_iir);
		else
			ilk_display_irq_handler(dev, de_iir);
	}

	if (INTEL_INFO(dev)->gen >= 6) {
		u32 pm_iir = I915_READ(GEN6_PMIIR);
		if (pm_iir) {
			I915_WRITE(GEN6_PMIIR, pm_iir);
			ret = IRQ_HANDLED;
			gen6_rps_irq_handler(dev_priv, pm_iir);
		}
	}

	I915_WRITE(DEIER, de_ier);
	POSTING_READ(DEIER);
	if (!HAS_PCH_NOP(dev)) {
		I915_WRITE(SDEIER, sde_ier);
		POSTING_READ(SDEIER);
	}

	return ret;
}

static void bxt_hpd_irq_handler(struct drm_device *dev, u32 hotplug_trigger,
				const u32 hpd[HPD_NUM_PINS])
{
	struct drm_i915_private *dev_priv = to_i915(dev);
	u32 dig_hotplug_reg, pin_mask = 0, long_mask = 0;

	dig_hotplug_reg = I915_READ(PCH_PORT_HOTPLUG);
	I915_WRITE(PCH_PORT_HOTPLUG, dig_hotplug_reg);

	intel_get_hpd_pins(&pin_mask, &long_mask, hotplug_trigger,
			   dig_hotplug_reg, hpd,
			   bxt_port_hotplug_long_detect);

	intel_hpd_irq_handler(dev, pin_mask, long_mask);
}

static irqreturn_t gen8_irq_handler(int irq, void *arg)
{
	struct drm_device *dev = arg;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 master_ctl;
	irqreturn_t ret = IRQ_NONE;
	uint32_t tmp = 0;
	enum pipe pipe;
	u32 aux_mask = GEN8_AUX_CHANNEL_A;

	if (!intel_irqs_enabled(dev_priv))
		return IRQ_NONE;

	if (INTEL_INFO(dev_priv)->gen >= 9)
		aux_mask |=  GEN9_AUX_CHANNEL_B | GEN9_AUX_CHANNEL_C |
			GEN9_AUX_CHANNEL_D;

	master_ctl = I915_READ_FW(GEN8_MASTER_IRQ);
	master_ctl &= ~GEN8_MASTER_IRQ_CONTROL;
	if (!master_ctl)
		return IRQ_NONE;

	I915_WRITE_FW(GEN8_MASTER_IRQ, 0);

	/* Find, clear, then process each source of interrupt */

	ret = gen8_gt_irq_handler(dev_priv, master_ctl);

	if (master_ctl & GEN8_DE_MISC_IRQ) {
		tmp = I915_READ(GEN8_DE_MISC_IIR);
		if (tmp) {
			I915_WRITE(GEN8_DE_MISC_IIR, tmp);
			ret = IRQ_HANDLED;
			if (tmp & GEN8_DE_MISC_GSE)
				intel_opregion_asle_intr(dev);
			else
				DRM_ERROR("Unexpected DE Misc interrupt\n");
		}
		else
			DRM_ERROR("The master control interrupt lied (DE MISC)!\n");
	}

	if (master_ctl & GEN8_DE_PORT_IRQ) {
		tmp = I915_READ(GEN8_DE_PORT_IIR);
		if (tmp) {
			bool found = false;
			u32 hotplug_trigger = 0;

			if (IS_BROXTON(dev_priv))
				hotplug_trigger = tmp & BXT_DE_PORT_HOTPLUG_MASK;
			else if (IS_BROADWELL(dev_priv))
				hotplug_trigger = tmp & GEN8_PORT_DP_A_HOTPLUG;

			I915_WRITE(GEN8_DE_PORT_IIR, tmp);
			ret = IRQ_HANDLED;

			if (tmp & aux_mask) {
				dp_aux_irq_handler(dev);
				found = true;
			}

			if (hotplug_trigger) {
				if (IS_BROXTON(dev))
					bxt_hpd_irq_handler(dev, hotplug_trigger, hpd_bxt);
				else
					ilk_hpd_irq_handler(dev, hotplug_trigger, hpd_bdw);
				found = true;
			}

			if (IS_BROXTON(dev) && (tmp & BXT_DE_PORT_GMBUS)) {
				gmbus_irq_handler(dev);
				found = true;
			}

			if (!found)
				DRM_ERROR("Unexpected DE Port interrupt\n");
		}
		else
			DRM_ERROR("The master control interrupt lied (DE PORT)!\n");
	}

	for_each_pipe(dev_priv, pipe) {
		uint32_t pipe_iir, flip_done = 0, fault_errors = 0;

		if (!(master_ctl & GEN8_DE_PIPE_IRQ(pipe)))
			continue;

		pipe_iir = I915_READ(GEN8_DE_PIPE_IIR(pipe));
		if (pipe_iir) {
			ret = IRQ_HANDLED;
			I915_WRITE(GEN8_DE_PIPE_IIR(pipe), pipe_iir);

			if (pipe_iir & GEN8_PIPE_VBLANK &&
			    intel_pipe_handle_vblank(dev, pipe))
				intel_check_page_flip(dev, pipe);

			if (INTEL_INFO(dev_priv)->gen >= 9)
				flip_done = pipe_iir & GEN9_PIPE_PLANE1_FLIP_DONE;
			else
				flip_done = pipe_iir & GEN8_PIPE_PRIMARY_FLIP_DONE;

			if (flip_done) {
				intel_prepare_page_flip(dev, pipe);
				intel_finish_page_flip_plane(dev, pipe);
			}

			if (pipe_iir & GEN8_PIPE_CDCLK_CRC_DONE)
				hsw_pipe_crc_irq_handler(dev, pipe);

			if (pipe_iir & GEN8_PIPE_FIFO_UNDERRUN)
				intel_cpu_fifo_underrun_irq_handler(dev_priv,
								    pipe);


			if (INTEL_INFO(dev_priv)->gen >= 9)
				fault_errors = pipe_iir & GEN9_DE_PIPE_IRQ_FAULT_ERRORS;
			else
				fault_errors = pipe_iir & GEN8_DE_PIPE_IRQ_FAULT_ERRORS;

			if (fault_errors)
				DRM_ERROR("Fault errors on pipe %c\n: 0x%08x",
					  pipe_name(pipe),
					  pipe_iir & GEN8_DE_PIPE_IRQ_FAULT_ERRORS);
		} else
			DRM_ERROR("The master control interrupt lied (DE PIPE)!\n");
	}

	if (HAS_PCH_SPLIT(dev) && !HAS_PCH_NOP(dev) &&
	    master_ctl & GEN8_DE_PCH_IRQ) {
		/*
		 * FIXME(BDW): Assume for now that the new interrupt handling
		 * scheme also closed the SDE interrupt handling race we've seen
		 * on older pch-split platforms. But this needs testing.
		 */
		u32 pch_iir = I915_READ(SDEIIR);
		if (pch_iir) {
			I915_WRITE(SDEIIR, pch_iir);
			ret = IRQ_HANDLED;

			if (HAS_PCH_SPT(dev_priv))
				spt_irq_handler(dev, pch_iir);
			else
				cpt_irq_handler(dev, pch_iir);
		} else {
			/*
			 * Like on previous PCH there seems to be something
			 * fishy going on with forwarding PCH interrupts.
			 */
			DRM_DEBUG_DRIVER("The