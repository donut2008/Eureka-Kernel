te, i) {
		const struct drm_plane_helper_funcs *funcs;

		funcs = plane->helper_private;

		if (funcs->cleanup_fb)
			funcs->cleanup_fb(plane, plane_state);
	}
}
EXPORT_SYMBOL(drm_atomic_helper_cleanup_planes);

/**
 * drm_atomic_helper_swap_state - store atomic state into current sw state
 * @dev: DRM device
 * @state: atomic state
 *
 * This function stores the atomic state into the current state pointers in all
 * driver objects. It should be called after all failing steps have been done
 * and succeeded, but before the actual hardware state is committed.
 *
 * For cleanup and error recovery the current state for all changed objects will
 * be swaped into @state.
 *
 * With that sequence it fits perfectly into the plane prepare/cleanup sequence:
 *
 * 1. Call drm_atomic_helper_prepare_planes() with the staged atomic state.
 *
 * 2. Do any other steps that might fail.
 *
 * 3. Put the staged state into the current state pointers with this function.
 *
 * 4. Actually commit the hardware state.
 *
 * 5. Call drm_atomic_helper_cleanup_planes() with @state, which since step 3
 * contains the old state. Also do any other cleanup required with that state.
 */
void drm_atomic_helper_swap_state(struct drm_device *dev,
				  struct drm_atomic_state *state)
{
	int i;

	for (i = 0; i < dev->mode_config.num_connector; i++) {
		struct drm_connector *connector = state->connectors[i];

		if (!connector)
			continue;

		connector->state->state = state;
		swap(state->connector_states[i], connector->state);
		connector->state->state = NULL;
	}

	for (i = 0; i < dev->mode_config.num_crtc; i++) {
		struct drm_crtc *crtc = state->crtcs[i];

		if (!crtc)
			continue;

		crtc->state->state = state;
		swap(state->crtc_states[i], crtc->state);
		crtc->state->state = NULL;
	}

	for (i = 0; i < dev->mode_config.num_total_plane; i++) {
		struct drm_plane *plane = state->planes[i];

		if (!plane)
			continue;

		plane->state->state = state;
		swap(state->plane_states[i], plane->state);
		plane->state->state = NULL;
	}
}
EXPORT_SYMBOL(drm_atomic_helper_swap_state);

/**
 * drm_atomic_helper_update_plane - Helper for primary plane update using atomic
 * @plane: plane object to update
 * @crtc: owning CRTC of owning plane
 * @fb: framebuffer to flip onto plane
 * @crtc_x: x offset of primary plane on crtc
 * @crtc_y: y offset of primary plane on crtc
 * @crtc_w: width of primary plane rectangle on crtc
 * @crtc_h: height of primary plane rectangle on crtc
 * @src_x: x offset of @fb for panning
 * @src_y: y offset of @fb for panning
 * @src_w: width of source rectangle in @fb
 * @src_h: height of source rectangle in @fb
 *
 * Provides a default plane update handler using the atomic driver interface.
 *
 * RETURNS:
 * Zero on success, error code on failure
 */
int drm_atomic_helper_update_plane(struct drm_plane *plane,
				   struct drm_crtc *crtc,
				   struct drm_framebuffer *fb,
				   int crtc_x, int crtc_y,
				   unsigned int crtc_w, unsigned int crtc_h,
				   uint32_t src_x, uint32_t src_y,
				   uint32_t src_w, uint32_t src_h)
{
	struct drm_atomic_state *state;
	struct drm_plane_state *plane_state;
	int ret = 0;

	state = drm_atomic_state_alloc(plane->dev);
	if (!state)
		return -ENOMEM;

	state->acquire_ctx = drm_modeset_legacy_acquire_ctx(crtc);
retry:
	plane_state = drm_atomic_get_plane_state(state, plane);
	if (IS_ERR(plane_state)) {
		ret = PTR_ERR(plane_state);
		goto fail;
	}

	ret = drm_atomic_set_crtc_for_plane(plane_state, crtc);
	if (ret != 0)
		goto fail;
	drm_atomic_set_fb_for_plane(plane_state, fb);
	plane_state->crtc_x = crtc_x;
	plane_state->crtc_y = crtc_y;
	plane_state->crtc_h = crtc_h;
	plane_state->crtc_w = crtc_w;
	plane_state->src_x = src_x;
	plane_state->src_y = src_y;
	plane_state->src_h = src_h;
	plane_state->src_w = src_w;

	if (plane == crtc->cursor)
		state->legacy_cursor_update = true;

	ret = drm_atomic_commit(state);
	if (ret != 0)
		goto fail;

	/* Driver takes ownership of state on successful commit. */
	return 0;
fail:
	if (ret == -EDEADLK)
		goto backoff;

	drm_atomic_state_free(state);

	return ret;
backoff:
	drm_atomic_state_clear(state);
	drm_atomic_legacy_backoff(state);

	/*
	 * Someone might have exchanged the framebuffer while we dropped locks
	 * in the backoff code. We need to fix up the fb refcount tracking the
	 * core does for us.
	 */
	plane->old_fb = plane->fb;

	goto retry;
}
EXPORT_SYMBOL(drm_atomic_helper_update_plane);

/**
 * drm_atomic_helper_disable_plane - Helper for primary plane disable using * atomic
 * @plane: plane to disable
 *
 * Provides a default plane disable handler using the atomic driver interface.
 *
 * RETURNS:
 * Zero on success, error code on failure
 */
int drm_atomic_helper_disable_plane(struct drm_plane *plane)
{
	struct drm_atomic_state *state;
	struct drm_plane_state *plane_state;
	int ret = 0;

	/*
	 * FIXME: Without plane->crtc set we can't get at the implicit legacy
	 * acquire context. The real fix will be to wire the acquire ctx through
	 * everywhere we need it, but meanwhile prevent chaos by just skipping
	 * this noop. The critical case is the cursor ioctls which a) only grab
	 * crtc/cursor-plane locks (so we need the crtc to get at the right
	 * acquire context) and b) can try to disable the plane multiple times.
	 */
	if (!plane->crtc)
		return 0;

	state = drm_atomic_state_alloc(plane->dev);
	if (!state)
		return -ENOMEM;

	state->acquire_ctx = drm_modeset_legacy_acquire_ctx(plane->crtc);
retry:
	plane_state = drm_atomic_get_plane_state(state, plane);
	if (IS_ERR(plane_state)) {
		ret = PTR_ERR(plane_state);
		goto fail;
	}

	if (plane_state->crtc && (plane == plane->crtc->cursor))
		plane_state->state->legacy_cursor_update = true;

	ret = __drm_atomic_helper_disable_plane(plane, plane_state);
	if (ret != 0)
		goto fail;

	ret = drm_atomic_commit(state);
	if (ret != 0)
		goto fail;

	/* Driver takes ownership of state on successful commit. */
	return 0;
fail:
	if (ret == -EDEADLK)
		goto backoff;

	drm_atomic_state_free(state);

	return ret;
backoff:
	drm_atomic_state_clear(state);
	drm_atomic_legacy_backoff(state);

	/*
	 * Someone might have exchanged the framebuffer while we dropped locks
	 * in the backoff code. We need to fix up the fb refcount tracking the
	 * core does for us.
	 */
	plane->old_fb = plane->fb;

	goto retry;
}
EXPORT_SYMBOL(drm_atomic_helper_disable_plane);

/* just used from fb-helper and atomic-helper: */
int __drm_atomic_helper_disable_plane(struct drm_plane *plane,
		struct drm_plane_state *plane_state)
{
	int ret;

	ret = drm_atomic_set_crtc_for_plane(plane_state, NULL);
	if (ret != 0)
		return ret;

	drm_atomic_set_fb_for_plane(plane_state, NULL);
	plane_state->crtc_x = 0;
	plane_state->crtc_y = 0;
	plane_state->crtc_h = 0;
	plane_state->crtc_w = 0;
	plane_state->src_x = 0;
	plane_state->src_y = 0;
	plane_state->src_h = 0;
	plane_state->src_w = 0;

	return 0;
}

static int update_output_state(struct drm_atomic_state *state,
			       struct drm_mode_set *set)
{
	struct drm_device *dev = set->crtc->dev;
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	struct drm_connector *connector;
	struct drm_connector_state *conn_state;
	int ret, i, j;

	ret = drm_modeset_lock(&dev->mode_config.connection_mutex,
			       state->acquire_ctx);
	if (ret)
		return ret;

	/* First grab all affected connector/crtc states. */
	for (i = 0; i < set->num_connectors; i++) {
		conn_state = drm_atomic_get_connector_state(state,
							    set->connectors[i]);
		if (IS_ERR(conn_state))
			return PTR_ERR(conn_state);
	}

	for_each_crtc_in_state(state, crtc, crtc_state, i) {
		ret = drm_atomic_add_affected_connectors(state, crtc);
		if (ret)
			return ret;
	}

	/* Then recompute connector->crtc links and crtc enabling state. */
	for_each_connector_in_state(state, connector, conn_state, i) {
		if (conn_state->crtc == set->crtc) {
			ret = drm_atomic_set_crtc_for_connector(conn_state,
								NULL);
			if (ret)
				return ret;
		}

		for (j = 0; j < set->num_connectors; j++) {
			if (set->connectors[j] == connector) {
				ret = drm_atomic_set_crtc_for_connector(conn_state,
									set->crtc);
				if (ret)
					return ret;
				break;
			}
		}
	}

	for_each_crtc_in_state(state, crtc, crtc_state, i) {
		/* Don't update ->enable for the CRTC in the set_config request,
		 * since a mismatch would indicate a bug in the upper layers.
		 * The actual modeset code later on will catch any
		 * inconsistencies here. */
		if (crtc == set->crtc)
			continue;

		if (!drm_atomic_connectors_for_crtc(state, crtc)) {
			ret = drm_atomic_set_mode_prop_for_crtc(crtc_state,
								NULL);
			if (ret < 0)
				return ret;

			crtc_state->active = false;
		}
	}

	return 0;
}

/**
 * drm_atomic_helper_set_config - set a new config from userspace
 * @set: mode set configuration
 *
 * Provides a default crtc set_config handler using the atomic driver interface.
 *
 * Returns:
 * Returns 0 on success, negative errno numbers on failure.
 */
int drm_atomic_helper_set_config(struct drm_mode_set *set)
{
	struct drm_atomic_state *state;
	struct drm_crtc *crtc = set->crtc;
	int ret = 0;

	state = drm_atomic_state_alloc(crtc->dev);
	if (!state)
		return -ENOMEM;

	state->acquire_ctx = drm_modeset_legacy_acquire_ctx(crtc);
retry:
	ret = __drm_atomic_helper_set_config(set, state);
	if (ret != 0)
		goto fail;

	ret = drm_atomic_commit(state);
	if (ret != 0)
		goto fail;

	/* Driver takes ownership of state on successful commit. */
	return 0;
fail:
	if (ret == -EDEADLK)
		goto backoff;

	drm_atomic_state_free(state);

	return ret;
backoff:
	drm_atomic_state_clear(state);
	drm_atomic_legacy_backoff(state);

	/*
	 * Someone might have exchanged the framebuffer while we dropped locks
	 * in the backoff code. We need to fix up the fb refcount tracking the
	 * core does for us.
	 */
	crtc->primary->old_fb = crtc->primary->fb;

	goto retry;
}
EXPORT_SYMBOL(drm_atomic_helper_set_config);

/* just used from fb-helper and atomic-helper: */
int __drm_atomic_helper_set_config(struct drm_mode_set *set,
		struct drm_atomic_state *state)
{
	struct drm_crtc_state *crtc_state;
	struct drm_plane_state *primary_state;
	struct drm_crtc *crtc = set->crtc;
	int hdisplay, vdisplay;
	int ret;

	crtc_state = drm_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	primary_state = drm_atomic_get_plane_state(state, crtc->primary);
	if (IS_ERR(primary_state))
		return PTR_ERR(primary_state);

	if (!set->mode) {
		WARN_ON(set->fb);
		WARN_ON(set->num_connectors);

		ret = drm_atomic_set_mode_for_crtc(crtc_state, NULL);
		if (ret != 0)
			return ret;

		crtc_state->active = false;

		ret = drm_atomic_set_crtc_for_plane(primary_state, NULL);
		if (ret != 0)
			return ret;

		drm_atomic_set_fb_for_plane(primary_state, NULL);

		goto commit;
	}

	WARN_ON(!set->fb);
	WARN_ON(!set->num_connectors);

	ret = drm_atomic_set_mode_for_crtc(crtc_state, set->mode);
	if (ret != 0)
		return ret;

	crtc_state->active = true;

	ret = drm_atomic_set_crtc_for_plane(primary_state, crtc);
	if (ret != 0)
		return ret;

	drm_crtc_get_hv_timing(set->mode, &hdisplay, &vdisplay);

	drm_atomic_set_fb_for_plane(primary_state, set->fb);
	primary_state->crtc_x = 0;
	primary_state->crtc_y = 0;
	primary_state->crtc_h = vdisplay;
	primary_state->crtc_w = hdisplay;
	primary_state->src_x = set->x << 16;
	primary_state->src_y = set->y << 16;
	if (primary_state->rotation & (BIT(DRM_ROTATE_90) | BIT(DRM_ROTATE_270))) {
		primary_state->src_h = hdisplay << 16;
		primary_state->src_w = vdisplay << 16;
	} else {
		primary_state->src_h = vdisplay << 16;
		primary_state->src_w = hdisplay << 16;
	}

commit:
	ret = update_output_state(state, set);
	if (ret)
		return ret;

	return 0;
}

/**
 * drm_atomic_helper_crtc_set_property - helper for crtc properties
 * @crtc: DRM crtc
 * @property: DRM property
 * @val: value of property
 *
 * Provides a default crtc set_property handler using the atomic driver
 * interface.
 *
 * RETURNS:
 * Zero on success, error code on failure
 */
int
drm_atomic_helper_crtc_set_property(struct drm_crtc *crtc,
				    struct drm_property *property,
				    uint64_t val)
{
	struct drm_atomic_state *state;
	struct drm_crtc_state *crtc_state;
	int ret = 0;

	state = drm_atomic_state_alloc(crtc->dev);
	if (!state)
		return -ENOMEM;

	/* ->set_property is always called with all locks held. */
	state->acquire_ctx = crtc->dev->mode_config.acquire_ctx;
retry:
	crtc_state = drm_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(crtc_state)) {
		ret = PTR_ERR(crtc_state);
		goto fail;
	}

	ret = drm_atomic_crtc_set_property(crtc, crtc_state,
			property, val);
	if (ret)
		goto fail;

	ret = drm_atomic_commit(state);
	if (ret != 0)
		goto fail;

	/* Driver takes ownership of state on successful commit. */
	return 0;
fail:
	if (ret == -EDEADLK)
		goto backoff;

	drm_atomic_state_free(state);

	return ret;
backoff:
	drm_atomic_state_clear(state);
	drm_atomic_legacy_backoff(state);

	goto retry;
}
EXPORT_SYMBOL(drm_atomic_helper_crtc_set_property);

/**
 * drm_atomic_helper_plane_set_property - helper for plane properties
 * @plane: DRM plane
 * @property: DRM property
 * @val: value of property
 *
 * Provides a default plane set_property handler using the atomic driver
 * interface.
 *
 * RETURNS:
 * Zero on success, error code on failure
 */
int
drm_atomic_helper_plane_set_property(struct drm_plane *plane,
				    struct drm_property *property,
				    uint64_t val)
{
	struct drm_atomic_state *state;
	struct drm_plane_state *plane_state;
	int ret = 0;

	state = drm_atomic_state_alloc(plane->dev);
	if (!state)
		return -ENOMEM;

	/* ->set_property is always called with all locks held. */
	state->acquire_ctx = plane->dev->mode_config.acquire_ctx;
retry:
	plane_state = drm_atomic_get_plane_state(state, plane);
	if (IS_ERR(plane_state)) {
		ret = PTR_ERR(plane_state);
		goto fail;
	}

	ret = drm_atomic_plane_set_property(plane, plane_state,
			property, val);
	if (ret)
		goto fail;

	ret = drm_atomic_commit(state);
	if (ret != 0)
		goto fail;

	/* Driver takes ownership of state on successful commit. */
	return 0;
fail:
	if (ret == -EDEADLK)
		goto backoff;

	drm_atomic_state_free(state);

	return ret;
backoff:
	drm_atomic_state_clear(state);
	drm_atom