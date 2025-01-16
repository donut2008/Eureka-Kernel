mebuffer *fb,
				struct drm_pending_vblank_event *event,
				uint32_t flags)
{
	struct drm_plane *plane = crtc->primary;
	struct drm_atomic_state *state;
	struct drm_plane_state *plane_state;
	struct drm_crtc_state *crtc_state;
	int ret = 0;

	if (flags & DRM_MODE_PAGE_FLIP_ASYNC)
		return -EINVAL;

	state = drm_atomic_state_alloc(plane->dev);
	if (!state)
		return -ENOMEM;

	state->acquire_ctx = drm_modeset_legacy_acquire_ctx(crtc);
retry:
	crtc_state = drm_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(crtc_state)) {
		ret = PTR_ERR(crtc_state);
		goto fail;
	}
	crtc_state->event = event;

	plane_state = drm_atomic_get_plane_state(state, plane);
	if (IS_ERR(plane_state)) {
		ret = PTR_ERR(plane_state);
		goto fail;
	}

	ret = drm_atomic_set_crtc_for_plane(plane_state, crtc);
	if (ret != 0)
		goto fail;
	drm_atomic_set_fb_for_plane(plane_state, fb);

	ret = drm_atomic_async_commit(state);
	if (ret != 0)
		goto fail;

	/* Driver takes ownership of state on successful async commit. */
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
EXPORT_SYMBOL(drm_atomic_helper_page_flip);

/**
 * drm_atomic_helper_connector_dpms() - connector dpms helper implementation
 * @connector: affected connector
 * @mode: DPMS mode
 *
 * This is the main helper function provided by the atomic helper framework for
 * implementing the legacy DPMS connector interface. It computes the new desired
 * ->active state for the corresponding CRTC (if the connector is enabled) and
 *  updates it.
 *
 * Returns:
 * Returns 0 on success, negative errno numbers on failure.
 */
int drm_atomic_helper_connector_dpms(struct drm_connector *connector,
				     int mode)
{
	struct drm_mode_config *config = &connector->dev->mode_config;
	struct drm_atomic_state *state;
	struct drm_crtc_state *crtc_state;
	struct drm_crtc *crtc;
	struct drm_connector *tmp_connector;
	int ret;
	bool active = false;
	int old_mode = connector->dpms;

	if (mode != DRM_MODE_DPMS_ON)
		mode = DRM_MODE_DPMS_OFF;

	connector->dpms = mode;
	crtc = connector->state->crtc;

	if (!crtc)
		return 0;

	state = drm_atomic_state_alloc(connector->dev);
	if (!state)
		return -ENOMEM;

	state->acquire_ctx = drm_modeset_legacy_acquire_ctx(crtc);
retry:
	crtc_state = drm_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(crtc_state)) {
		ret = PTR_ERR(crtc_state);
		goto fail;
	}

	WARN_ON(!drm_modeset_is_locked(&config->connection_mutex));

	drm_for_each_connector(tmp_connector, connector->dev) {
		if (tmp_connector->state->crtc != crtc)
			continue;

		if (tmp_connector->dpms == DRM_MODE_DPMS_ON) {
			active = true;
			break;
		}
	}
	crtc_state->active = active;

	ret = drm_atomic_commit(state);
	if (ret != 0)
		goto fail;

	/* Driver takes ownership of state on successful commit. */
	return 0;
fail:
	if (ret == -EDEADLK)
		goto backoff;

	connector->dpms = old_mode;
	drm_atomic_state_free(state);

	return ret;
backoff:
	drm_atomic_state_clear(state);
	drm_atomic_legacy_backoff(state);

	goto retry;
}
EXPORT_SYMBOL(drm_atomic_helper_connector_dpms);

/**
 * DOC: atomic state reset and initialization
 *
 * Both the drm core and the atomic helpers assume that there is always the full
 * and correct atomic software state for all connectors, CRTCs and planes
 * available. Which is a bit a problem on driver load and also after system
 * suspend. One way to solve this is to have a hardware state read-out
 * infrastructure which reconstructs the full software state (e.g. the i915
 * driver).
 *
 * The simpler solution is to just reset the software state to everything off,
 * which is easiest to do by calling drm_mode_config_reset(). To facilitate this
 * the atomic helpers provide default reset implementations for all hooks.
 */

/**
 * drm_atomic_helper_crtc_reset - default ->reset hook for CRTCs
 * @crtc: drm CRTC
 *
 * Resets the atomic state for @crtc by freeing the state pointer (which might
 * be NULL, e.g. at driver load time) and allocating a new empty state object.
 */
void drm_atomic_helper_crtc_reset(struct drm_crtc *crtc)
{
	if (crtc->state && crtc->state->mode_blob)
		drm_property_unreference_blob(crtc->state->mode_blob);
	kfree(crtc->state);
	crtc->state = kzalloc(sizeof(*crtc->state), GFP_KERNEL);

	if (crtc->state)
		crtc->state->crtc = crtc;
}
EXPORT_SYMBOL(drm_atomic_helper_crtc_reset);

/**
 * __drm_atomic_helper_crtc_duplicate_state - copy atomic CRTC state
 * @crtc: CRTC object
 * @state: atomic CRTC state
 *
 * Copies atomic state from a CRTC's current state and resets inferred values.
 * This is useful for drivers that subclass the CRTC state.
 */
void __drm_atomic_helper_crtc_duplicate_state(struct drm_crtc *crtc,
					      struct drm_crtc_state *state)
{
	memcpy(state, crtc->state, sizeof(*state));

	if (st