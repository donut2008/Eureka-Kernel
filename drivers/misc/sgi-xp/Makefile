= __intel_get_crtc_scanline(intel_crtc);
	} else {
		/* Have access to pixelcount since start of frame.
		 * We can split this into vertical and horizontal
		 * scanout position.
		 */
		position = (__raw_i915_read32(dev_priv, PIPEFRAMEPIXEL(pipe)) & PIPE_PIXEL_MASK) >> PIPE_PIXEL_SHIFT;

		/* convert to pixel counts */
		vbl_start *= htotal;
		vbl_end *= htotal;
		vtotal *= htotal;

		/*
		 * In interlaced modes, the pixel counter counts all pixels,
		 * so one field will have htotal more pixels. In order to avoid
		 * the r