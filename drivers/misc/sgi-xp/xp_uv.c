we get a low value that's stable across two reads of the high
	 * register.
	 */
	do {
		high1 = I915_READ(high_frame) & PIPE_FRAME_HIGH_MASK;
		low   = I915_READ(low_frame);
		high2 = I915_READ(high_frame) & PIPE_FRAME_HIGH_MASK;
	} while (high1 != high2);

	high1 >>= PIPE_FRAME_HIGH_SHIFT;
	pixel = low & PIPE_PIXEL_MASK;
	low >>= PIPE_FRAME_LOW_SHIFT;

	/*
	 * The frame counter increments at beginning of active.
	 * Cook up a vblank counter by also checking the pixel
	 * counter against vblank start.
	 */
	return (((high1 << 8) | low) + (pixel >= vbl_start)) & 0xffffff;
}

static u32 g4x_get_vblank_counter(struct drm_device *dev, unsigned int pipe)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	return I915_READ(PIPE_FRMCOUNT_G4X(pipe));
}

/* raw reads, only for fast reads of display block, no need for forcewake etc. */
#define __raw_i915_read32(dev_priv__, reg__) readl((dev_priv__)->regs + (reg__))

static int __intel_get_crtc_scanline(struct intel_crtc *crtc)
{
	struct drm_device *dev = crtc->base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	const struct drm_display_mode *mode = &crtc->base.hwmode;
	enum pipe pipe = crtc->pipe;
	int position, vtotal;

	vtotal = mode->crtc_vtotal;
	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		vtotal /= 2;

	if (IS_GEN2(dev))
		position = __raw_i915_read32(dev_priv, PIPEDSL(pipe)) & DSL_LINEMASK_GEN2;
	else
		position = __raw_i915_read32(dev_priv, PIPEDSL(pipe)) & DSL_LINEMASK_GEN3;

	/*
	 * On HSW, the DSL reg (0x70000) appears to return 0 if we
	 * read it just before the start of vblank.  So try it again
	 * so we don't accidentally end up spanning a vblank frame
	 * increment, causing the pipe_update_end() code to squak at us.
	 *
	 * The nature of this problem means we can't simply check the ISR
	 * bit and return the vblank start value; nor can we use the scanline
	 * debug register in the transcoder as it appears to have the same
	 * problem.  We may need to extend this to include other platforms,
	 * but so far testing only shows the problem on HSW.
	 */
	if (HAS_DDI(dev) && !position) {
		int i, temp;

		for (i = 0; i < 100; i++) {
			udelay(1);
			temp = __raw_i915_read32(dev_priv, PIPEDSL(pipe)) &
				DSL_LINEMASK_GEN3;
			if (temp != position) {
				position = temp;
				break;
			}
		}
	}

	/*
	 * See update_scanline_offset() for the details on the
	 * scanline_offset adjustment.
	 */
	return (position + crtc->scanline_offset) % vtotal;
}

static int i915_get_crtc_scanoutpos(struct drm_device *dev, unsigned int pipe,
				    unsigned int flags, int *vpos, int *hpos,
				    ktime_t *stime, ktime_t *etime,
				    const struct drm_display_mode *mode)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_crtc *crtc = dev_priv->pipe_to_crtc_mapping[pipe];
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	int position;
	int vbl_start, vbl_end, hsync_start, htotal, vtotal;
	bool in_vbl = true;
	int ret = 0;
	unsigned long irqflags;

	if (WARN_ON(!mode->crtc_clock)) {
		DRM_DEBUG_DRIVER("trying to get scanoutpos for disabled "
				 "pipe %c\n", pipe_name(pipe));
		return 0;
	}

	htotal = mode->crtc_htotal;
	hsync_start = mode->crtc_hsync_start;
	vtotal = mode->crtc_vtotal;
	vbl_start = mode->crtc_vblank_start;
	vbl_end = mode->crtc_vblank_end;

	if (mode->flags & DRM_MODE_FLAG_INTERLACE) {
		vbl_start = DIV_ROUND_UP(vbl_start, 2);
		vbl_end /= 2;
		vtotal /= 2;
	}

	ret |= DRM_SCANOUTPOS_VALID | DRM_SCANOUTPOS_ACCURATE;

	/*
	 * Lock uncore.lock, as we will do multiple timing critical raw
	 * register reads, potentially with preemption disabled, so the
	 * following code must not block on uncore.lock.
	 */
	spin_lock_irqsave(&dev_priv->uncore.lock, irqflags);

	/* preempt_disable_rt() should go right here in PREEMPT_RT patchset. */
	preempt_disable_rt();

	/* Get optional system timestamp before query. */
	if (stime)
		*stime = ktime_get();

	if (IS_GEN2(dev) || IS_G4X(dev) || INTEL_INFO(dev)->gen >= 5) {
		/* No obvious pi