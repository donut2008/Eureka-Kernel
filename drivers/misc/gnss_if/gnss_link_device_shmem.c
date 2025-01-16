ed, but who's crtc changes it's
	 * configuration. This must be done before calling mode_fixup in case a
	 * crtc only changed its mode but has the same set of connectors.
	 */
	for_each_crtc_in_state(state, crtc, crtc_state, i) {
		int num_connectors;

		/*
		 * We must set ->active_changed after walking connectors for
		 * otherwise an update that only changes active would result in
		 * a full modeset because update_connector_routing force that.
		 */
		if (crtc->state->active != crtc_state->active) {
			DRM_DEBUG_ATOMIC("[CRTC:%d] active changed\n",
					 crtc->base.id);
			crtc_state->active_changed = true;
		}

		if (!drm_atomic_crtc_needs_modeset(crtc_state))
			continue;

		DRM_DEBUG_ATOMIC("[CRTC:%d] needs all connectors, enable: %c, active: %c\n",
				 crtc->base.id,
				 crtc_state->enable ? 'y' : 'n',
			      crtc_state->active ? 'y' : 'n');

		ret = drm_atomic_add_affected_connectors(state, crtc);
		if (ret != 0)
			return ret;

		ret = drm_atomic_add_affected_planes(state, crtc);
		if (ret != 0)
			return ret;

		num_connectors = drm_atomic_connectors_for_crtc(state,
								crtc);

		if (crtc_state->enable != !!num_connectors) {
			DRM_DEBUG_ATOMIC("[CRTC:%d] enabled/connectors mismatch\n",
					 crtc->base.id);

			return -EINVAL;
		}
	}

	return mode_fixup(state);
}
EXPORT_SYMBOL(drm_atomic_helper_check_modeset);

/**
 * drm_atomic_helper_check_planes - validate state object for planes changes
 * @dev: DRM device
 * @state: the driver state object
 *
 * Check the state object to see if the requested state is physically possible.
 * This does all the plane update related checks using by calling into the
 * ->atomic_check hooks provided by the driver.
 *
 * It also sets crtc_state->planes_changed to indicate that a crtc has
 * updated planes.
 *
 * RETURNS
 * Zero for success or -errno
 */
int
drm_atomic_helper_check_planes(struct drm_device *dev,
			       struct drm_atomic_state *state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	struct drm_plane *plane;
	struct drm_plane_state *plane_state;
	int i, ret = 0;

	for_each_plane_in_state(state, plane, plane_state, i) {
		const struct drm_plane_helper_funcs *funcs;

		funcs = plane->helper_private;

		drm_atomic_helper_plane_changed(state, plane_state, plane);

		if (!funcs || !funcs->atomic_check)
			continue;

		ret = funcs->atomic_check(plane, plane_state);
		if (ret) {
			DRM_DEBUG_ATOMIC("[PLANE:%d] atomic driver check failed\n",
					 plane->base.id);
			return ret;
		}
	}

	for_each_crtc_in_state(state, crtc, crtc_state, i) {
		const struct drm_crtc_helper_funcs *funcs;

		funcs = crtc->helper_private;

		if (!funcs || !funcs->atomic_check)
			continue;

		ret = funcs->atomic_check(crtc, state->crtc_states[i]);
		if (ret) {
			DRM_DEBUG_ATOMIC("[CRTC:%d] atomic driver check failed\n",
					 crtc->base.id);
			return ret;
		}
	}

	return ret;
}
EXPORT_SYMBOL(drm_atomic_helper_check_planes);

/**
 * drm_atomic_helper_check - validate state object
 * @dev: DRM device
 * @state: the driver state object
 *
 * Check the state object to see if the requested state is physically possible.
 * Only crtcs and planes have check callbacks, so for any additional (global)
 * checking that a driver needs it can simply wrap that around this function.
 * Drivers without such needs can directly use this as their ->atomic_check()
 * callback.
 *
 * This just wraps the two parts of the state checking for planes and modeset
 * state in the default order: First it calls drm_atomic_helper_check_modeset()
 * and then drm_atomic_helper_check_planes(). The assumption is that the
 * ->atomic_check functions depend upon an updated adjusted_mode.clock to
 * e.g. properly compute watermarks.
 *
 * RETURNS
 * Zero for success or -errno
 */
int drm_atomic_helper_check(struct drm_device *dev,
			    struct drm_atomic_state *state)
{
	int ret;

	ret = drm_atomic_helper_check_modeset(dev, state);
	if (ret)
		return ret;

	ret = drm_atomic_helper_check_planes(dev, state);
	if (ret)
		return ret;

	return ret;
}
EXPORT_SYMBOL(drm_atomic_helper_check);

static void
disable_outputs(struct drm_device *dev, struct drm_atomic_state *old_state)
{
	struct drm_connector *connector;
	struct drm_connector_state *old_conn_state;
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state;
	int i;

	for_each_connector_in_state(old_state, connector, old_conn_state, i) {
		const struct drm_encoder_helper_funcs *funcs;
		struct drm_encoder *encoder;
		struct drm_crtc_state *old_crtc_state;

		/* Shut down everything that's in the changeset and currently
		 * still on. So need to check the old, saved state. */
		if (!old_conn_state->crtc)
			continue;

		old_crtc_state = old_state->crtc_states[drm_crtc_index(old_conn_state->crtc)];

		if (!old_crtc_state->active ||
		    !drm_atomic_crtc_needs_modeset(old_conn_state->crtc->state))
			continue;

		encoder = old_conn_state->best_encoder;

		/* We shouldn't get this far if we didn't previously have
		 * an encoder.. but WARN_ON() rather than explode.
		 */
		if (WARN_ON(!encoder))
			continue;

		funcs = encoder->helper_private;

		DRM_DEBUG_ATOMIC("disabling [ENCODER:%d:%s]\n",
				 encoder->base.id, encoder->name);

		/*
		 * Each encoder has at most one connector (since we always steal
		 * it away), so we won't call disable hooks twice.
		 */
		drm_bridge_disable(encoder->bridge);

		/* Right function depends upon target state. */
		if (connector->state->crtc && funcs->prepare)
			funcs->prepare(encoder);
		else if (funcs->disable)
			funcs->disable(encoder);
		else
			funcs->dpms(encoder, DRM_MODE_DPMS_OFF);

		drm_bridge_post_disable(encoder->bridge);
	}

	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		const struct drm_crtc_helper_funcs *funcs;

		/* Shut down everything that needs a full modeset. */
		if (!drm_atomic_crtc_needs_modeset(crtc->state))
			continue;

		if (!old_crtc_state->active)
			continue;

		funcs = crtc->helper_private;

		DRM_DEBUG_ATOMIC("disabling [CRTC:%d]\n",
				 crtc->base.id);


		/* Right function depends upon target state. */
		if (crtc->state->enable && funcs->prepare)
			funcs->prepare(crtc);
		else if (funcs->disable)
			funcs->disable(crtc);
		else
			funcs->dpms(crtc, DRM_MODE_DPMS_OFF);
	}
}

/**
 * drm_atomic_helper_update_legacy_modeset_state - update legacy modeset state
 * @dev: DRM device
 * @old_state: atomic state object with old state structures
 *
 * This function updates all the various legacy modeset state pointers in
 * connectors, encoders and crtcs. It also updates the timestamping constants
 * used for precise vblank timestamps by calling
 * drm_calc_timestamping_constants().
 *
 * Drivers can use this for building their own atomic commit if they don't have
 * a pure helper-based modeset implementation.
 */
void
drm_atomic_helper_update_legacy_modeset_state(struct drm_device *dev,
					      struct drm_atomic_state *old_state)
{
	struct drm_connector *connector;
	struct drm_connector_state *old_conn_state;
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state;
	int i;

	/* clear out existing links and update dpms */
	for_each_connector_in_state(old_state, connector, old_conn_state, i) {
		if (connector->encoder) {
			WARN_ON(!connector->encoder->crtc);

			connector->encoder->crtc = NULL;
			connector->encoder = NULL;
		}

		crtc = connector->state->crtc;
		if ((!crtc && old_conn_state->crtc) ||
		    (crtc && drm_atomic_crtc_needs_modeset(crtc->state))) {
			struct drm_property *dpms_prop =
				dev->mode_config.dpms_property;
			int mode = DRM_MODE_DPMS_OFF;

			if (crtc && crtc->state->active)
				mode = DRM_MODE_DPMS_ON;

			connector->dpms = mode;
			drm_object_property_set_value(&connector->base,
						      dpms_prop, mode);
		}
	}

	/* set new links */
	for_each_connector_in_state(old_state, connector, old_conn_state, i) {
		if (!connector->state->crtc)
			continue;

		if (WARN_ON(!connector->state->best_encoder))
			continue;

		connector->encoder = connector->state->best_encoder;
		connector->encoder->crtc = connector->state->crtc;
	}

	/* set legacy state in the crtc structure */
	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		struct drm_plane *primary = crtc->primary;

		crtc->mode = crtc->state->mode;
		crtc->enabled = crtc->state->enable;

		if (drm_atomic_get_existing_plane_state(old_state, primary) &&
		    primary->state->crtc == crtc) {
			crtc->x = primary->state->src_x >> 16;
			crtc->y = primary->state->src_y >> 16;
		}

		if (crtc->state->enable)
			drm_calc_timestamping_constants(crtc,
							&crtc->state->adjusted_mode);
	}
}
EXPORT_SYMBOL(drm_atomic_helper_update_legacy_modeset_state);

static void
crtc_set_mode(struct drm_device *dev, struct drm_atomic_state *old_state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state;
	struct drm_connector *connector;
	struct drm_connector_state *old_conn_state;
	int i;

	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		const struct drm_crtc_helper_funcs *funcs;

		if (!crtc->state->mode_changed)
			continue;

		funcs = crtc->helper_private;

		if (crtc->state->enable && funcs->mode_set_nofb) {
			DRM_DEBUG_ATOMIC("modeset on [CRTC:%d]\n",
					 crtc->base.id);

			funcs->mode_set_nofb(crtc);
		}
	}

	for_each_connector_in_state(old_state, connector, old_conn_state, i) {
		const struct drm_encoder_helper_funcs *funcs;
		struct drm_crtc_state *new_crtc_state;
		struct drm_encoder *encoder;
		struct drm_display_mode *mode, *adjusted_mode;

		if (!connector->state->best_encoder)
			continue;

		encoder = connector->state->best_encoder;
		funcs = encoder->helper_private;
		new_crtc_state = connector->state->crtc->state;
		mode = &new_crtc_state->mode;
		adjusted_mode = &new_crtc_state->adjusted_mode;

		if (!new_crtc_state->mode_changed)
			continue;

		DRM_DEBUG_ATOMIC("modeset on [ENCODER:%d:%s]\n",
				 encoder->base.id, encoder->name);

		/*
		 * Each encoder has at most one connector (since we always steal
		 * it away), so we won't call mode_set hooks twice.
		 */
		if (funcs->mode_set)
			funcs->mode_set(encoder, mode, adjusted_mode);

		drm_bridge_mode_set(encoder->bridge, mode, adjusted_mode);
	}
}

/**
 * drm_atomic_helper_commit_modeset_disables - modeset commit to disable outputs
 * @dev: DRM device
 * @old_state: atomic state object with old state structures
 *
 * This function shuts down all the outputs that need to be shut down and
 * prepares them (if required) with the new mode.
 *
 * For compatibility with legacy crtc helpers this should be called before
 * drm_atomic_helper_commit_planes(), which is what the default commit function
 * does. But drivers with different needs can group the modeset commits together
 * and do the plane commits at the end. This is useful for drivers doing runtime
 * PM since planes updates then only happen when the CRTC is actually enabled.
 */
void drm_atomic_helper_commit_modeset_disables(struct drm_device *dev,
					       struct drm_atomic_state *old_state)
{
	disable_outputs(dev, old_state);

	drm_atomic_helper_update_legacy_modeset_state(dev, old_state);

	crtc_set_mode(dev, old_state);
}
EXPORT_SYMBOL(drm_atomic_helper_commit_modeset_disables);

/**
 * drm_atomic_helper_commit_modeset_enables - modeset commit to enable outputs
 * @dev: DRM device
 * @old_state: atomic state object with old state structures
 *
 * This function enables all the outputs with the new configuration which had to
 * be turned off for the update.
 *
 * For compatibility with legacy crtc helpers this should be called after
 * drm_atomic_helper_commit_planes(), which is what the default commit function
 * does. But drivers with different needs can group the modeset commits together
 * and do the plane commits at the end. This is useful for drivers doing runtime
 * PM since planes updates then only happen when the CRTC is actually enabled.
 */
void drm_atomic_helper_commit_modeset_enables(struct drm_device *dev,
					      struct drm_atomic_state *old_state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state;
	struct drm_connector *connector;
	struct drm_connector_state *old_conn_state;
	int i;

	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		const struct drm_crtc_helper_funcs *funcs;

		/* Need to filter out CRTCs where only planes change. */
		if (!drm_atomic_crtc_needs_modeset(crtc->state))
			continue;

		if (!crtc->state->active)
			continue;

		funcs = crtc->helper_private;

		if (crtc->state->enable) {
			DRM_DEBUG_ATOMIC("enabling [CRTC:%d]\n",
					 crtc->base.id);

			if (funcs->enable)
				funcs->enable(crtc);
			else
				funcs->commit(crtc);
		}
	}

	for_each_connector_in_state(old_state, connector, old_conn_state, i) {
		const struct drm_encoder_helper_funcs *funcs;
		struct drm_encoder *encoder;

		if (!connector->state->best_encoder)
			continue;

		if (!connector->state->crtc->state->active ||
		    !drm_atomic_crtc_needs_modeset(connector->state->crtc->state))
			continue;

		encoder = connector->state->best_encoder;
		funcs = encoder->helper_private;

		DRM_DEBUG_ATOMIC("enabling [ENCODER:%d:%s]\n",
				 encoder->base.id, encoder->name);

		/*
		 * Each encoder has at most one connector (since we always steal
		 * it away), so we won't call enable hooks twice.
		 */
		drm_bridge_pre_enable(encoder->bridge);

		if (funcs->enable)
			funcs->enable(encoder);
		else
			funcs->commit(encoder);

		drm_bridge_enable(encoder->bridge);
	}
}
EXPORT_SYMBOL(drm_atomic_helper_commit_modeset_enables);

static void wait_for_fences(struct drm_device *dev,
			    struct drm_atomic_state *state)
{
	struct drm_plane *plane;
	struct drm_plane_state *plane_state;
	int i;

	for_each_plane_in_state(state, plane, plane_state, i) {
		if (!plane->state->fence)
			continue;

		WARN_ON(!plane->state->fb);

		fence_wait(plane->state->fence, false);
		fence_put(plane->state->fence);
		plane->state->fence = NULL;
	}
}

static bool framebuffer_changed(struct drm_device *dev,
				struct drm_atomic_state *old_state,
				struct drm_crtc *crtc)
{
	struct drm_plane *plane;
	struct drm_plane_state *old_plane_state;
	int i;

	for_each_plane_in_state(old_state, plane, old_plane_state, i) {
		if (plane->state->crtc != crtc &&
		    old_plane_state->crtc != crtc)
			continue;

		if (plane->state->fb != old_plane_state->fb)
			return true;
	}

	return false;
}

/**
 * drm_atomic_helper_wait_for_vblanks - wait for vblank on crtcs
 * @dev: DRM device
 * @old_state: atomic state object with old state structures
 *
 * Helper to, after atomic commit, wait for vblanks on all effected
 * crtcs (ie. before cleaning up old framebuffers using
 * drm_atomic_helper_cleanup_planes()). It will only wait on crtcs where the
 * framebuffers have actually changed to optimize for the legacy cursor and
 * plane update use-case.
 */
void
drm_atomic_helper_wait_for_vblanks(struct drm_device *dev,
		struct drm_atomic_state *old_state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state;
	int i, ret;

	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		/* No one cares about the old state, so abuse it for tracking
		 * and store whether we hold a vblank reference (and should do a
		 * vblank wait) in the ->enable boolean. */
		old_crtc_state->enable = false;

		if (!crtc->state->enable)
			continue;

		/* Legacy cursor ioctls are completely unsynced, and userspace
		 * relies on that (by doing tons of cursor updates). */
		if (old_state->legacy_cursor_update)
			continue;

		if (!framebuffer_changed(dev, old_state, crtc))
			continue;

		ret = drm_crtc_vblank_get(crtc);
		if (ret != 0)
			continue;

		old_crtc_state->enable = true;
		old_crtc_state->last_vblank_count = drm_crtc_vblank_count(crtc);
	}

	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		if (!old_crtc_state->enable)
			continue;

		ret = wait_event_timeout(dev->vblank[i].queue,
				old_crtc_state->last_vblank_count !=
					drm_crtc_vblank_count(crtc),
				msecs_to_jiffies(50));

		drm_crtc_vblank_put(crtc);
	}
}
EXPORT_SYMBOL(drm_atomic_helper_wait_for_vblanks);

/**
 * drm_atomic_helper_commit - commit validated state object
 * @dev: DRM device
 * @state: the driver state object
 * @async: asynchronous commit
 *
 * This function commits a with drm_atomic_helper_check() pre-validated state
 * object. This can still fail when e.g. the framebuffer reservation fails. For
 * now this doesn't implement asynchronous commits.
 *
 * Note that right now this function does not support async commits, and hence
 * driver writers must implement their own version for now. Also note that the
 * default ordering of how the various stages are called is to match the legacy
 * modeset helper library closest. One peculiarity of that is that it doesn't
 * mesh well with runtime PM at all.
 *
 * For drivers supporting runtime PM the recommended sequence is
 *
 *     drm_atomic_helper_commit_modeset_disables(dev, state);
 *
 *     drm_atomic_helper_commit_modeset_enables(dev, state);
 *
 *     drm_atomic_helper_commit_planes(dev, state, true);
 *
 * See the kerneldoc entries for these three functions for more details.
 *
 * RETURNS
 * Zero for success or -errno.
 */
int drm_atomic_helper_commit(struct drm_device *dev,
			     struct drm_atomic_state *state,
			     bool async)
{
	int ret;

	if (async)
		return -EBUSY;

	ret = drm_atomic_helper_prepare_planes(dev, state);
	if (ret)
		return ret;

	/*
	 * This is the point of no return - everything below never fails except
	 * when the hw goes bonghits. Which means we can commit the new state on
	 * the software side now.
	 */

	drm_atomic_helper_swap_state(dev, state);

	/*
	 * Everything below can be run asynchronously without the need to grab
	 * any modeset locks at all under one condition: It must be guaranteed
	 * that the asynchronous work has either been cancelled (if the driver
	 * supports it, which at least requires that the framebuffers get
	 * cleaned up with drm_atomic_helper_cleanup_planes()) or completed
	 * before the new state gets committed on the software side with
	 * drm_atomic_helper_swap_state().
	 *
	 * This scheme allows new atomic state updates to be prepared and
	 * checked in parallel to the asynchronous completion of the previous
	 * update. Which is important since compositors need to figure out the
	 * composition of the next frame right after having submitted the
	 * current layout.
	 */

	wait_for_fences(dev, state);

	drm_atomic_helper_commit_modeset_disables(dev, state);

	drm_atomic_helper_commit_planes(dev, state, false);

	drm_atomic_helper_commit_modeset_enables(dev, state);

	drm_atomic_helper_wait_for_vblanks(dev, state);

	drm_atomic_helper_cleanup_planes(dev, state);

	drm_atomic_state_free(state);

	return 0;
}
EXPORT_SYMBOL(drm_atomic_helper_commit);

/**
 * DOC: implementing async commit
 *
 * For now the atomic helpers don't support async commit directly. If there is
 * real need it could be added though, using the dma-buf fence infrastructure
 * for generic synchronization with outstanding rendering.
 *
 * For now drivers have to implement async commit themselves, with the following
 * sequence being the recommended one:
 *
 * 1. Run drm_atomic_helper_prepare_planes() first. This is the only function
 * which commit needs to call which can fail, so we want to run it first and
 * synchronously.
 *
 * 2. Synchronize with any outstanding asynchronous commit worker threads which
 * might be affected the new state update. This can be done by either cancelling
 * or flushing the work items, depending upon whether the driver can deal with
 * cancelled updates. Note that it is important to ensure that the framebuffer
 * cleanup is still done when cancelling.
 *
 * For sufficient parallelism it is recommended to have a work item per crtc
 * (for updates which don't touch global state) and a global one. Then we only
 * need to synchronize with the crtc work items for changed crtcs and the global
 * work item, which allows nice concurrent updates on disjoint sets of crtcs.
 *
 * 3. The software state is updated synchronously with
 * drm_atomic_helper_swap_state(). Doing this under the protection of all modeset
 * locks means concurrent callers never see inconsistent state. And doing this
 * while it's guaranteed that no relevant async worker runs means that async
 * workers do not need grab any locks. Actually they must not grab locks, for
 * otherwise the work flushing will deadlock.
 *
 * 4. Schedule a work item to do all subsequent steps, using the split-out
 * commit helpers: a) pre-plane commit b) plane commit c) post-plane commit and
 * then cleaning up the framebuffers after the old framebuffer is no longer
 * being displayed.
 */

/**
 * drm_atomic_helper_prepare_planes - prepare plane resources before commit
 * @dev: DRM device
 * @state: atomic state object with new state structures
 *
 * This function prepares plane state, specifically framebuffers, for the new
 * configuration. If any failure is encountered this function will call
 * ->cleanup_fb on any already successfully prepared framebuffer.
 *
 * Returns:
 * 0 on success, negative error code on failure.
 */
int drm_atomic_helper_prepare_planes(struct drm_device *dev,
				     struct drm_atomic_state *state)
{
	int nplanes = dev->mode_config.num_total_plane;
	int ret, i;

	for (i = 0; i < nplanes; i++) {
		const struct drm_plane_helper_funcs *funcs;
		struct drm_plane *plane = state->planes[i];
		struct drm_plane_state *plane_state = state->plane_states[i];

		if (!plane)
			continue;

		funcs = plane->helper_private;

		if (funcs->prepare_fb) {
			ret = funcs->prepare_fb(plane, plane_state);
			if (ret)
				goto fail;
		}
	}

	return 0;

fail:
	for (i--; i >= 0; i--) {
		const struct drm_plane_helper_funcs *funcs;
		struct drm_plane *plane = state->planes[i];
		struct drm_plane_state *plane_state = state->plane_states[i];

		if (!plane)
			continue;

		funcs = plane->helper_private;

		if (funcs->cleanup_fb)
			funcs->cleanup_fb(plane, plane_state);

	}

	return ret;
}
EXPORT_SYMBOL(drm_atomic_helper_prepare_planes);

bool plane_crtc_active(struct drm_plane_state *state)
{
	return state->crtc && state->crtc->state->active;
}

/**
 * drm_atomic_helper_commit_planes - commit plane state
 * @dev: DRM device
 * @old_state: atomic state object with old state structures
 * @active_only: Only commit on active CRTC if set
 *
 * This function commits the new plane state using the plane and atomic helper
 * functions for planes and crtcs. It assumes that the atomic state has already
 * been pushed into the relevant object state pointers, since this step can no
 * longer fail.
 *
 * It still requires the global state object @old_state to know which planes and
 * crtcs need to be updated though.
 *
 * Note that this function does all plane updates across all CRTCs in one step.
 * If the hardware can't support this approach look at
 * drm_atomic_helper_commit_planes_on_crtc() instead.
 *
 * Plane parameters can be updated by applications while the associated CRTC is
 * disabled. The DRM/KMS core will store the parameters in the plane state,
 * which will be available to the driver when the CRTC is turned on. As a result
 * most drivers don't need to be immediately notified of plane updates for a
 * disabled CRTC.
 *
 * Unless otherwise needed, drivers are advised to set the @active_only
 * parameters to true in order not to receive plane update notifications related
 * to a disabled CRTC. This avoids the need to manually ignore plane updates in
 * driver code when the driver and/or hardware can't or just don't need to deal
 * with updates on disabled CRTCs, for example when supporting runtime PM.
 *
 * The drm_atomic_helper_commit() default implementation only sets @active_only
 * to false to most closely match the behaviour of the legacy helpers. This should
 * not be copied blindly by drivers.
 */
void drm_atomic_helper_commit_planes(struct drm_device *dev,
				     struct drm_atomic_state *old_state,
				     bool active_only)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state;
	struct drm_plane *plane;
	struct drm_plane_state *old_plane_state;
	int i;

	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		const struct drm_crtc_helper_funcs *funcs;

		funcs = crtc->helper_private;

		if (!funcs || !funcs->atomic_begin)
			continue;

		if (active_only && !crtc->state->active)
			continue;

		funcs->atomic_begin(crtc, old_crtc_state);
	}

	for_each_plane_in_state(old_state, plane, old_plane_state, i) {
		const struct drm_plane_helper_funcs *funcs;
		bool disabling;

		funcs = plane->helper_private;

		if (!funcs)
			continue;

		disabling = drm_atomic_plane_disabling(plane, old_plane_state);

		if (active_only) {
			/*
			 * Skip planes related to inactive CRTCs. If the plane
			 * is enabled use the state of the current CRTC. If the
			 * plane is being disabled use the state of the old
			 * CRTC to avoid skipping planes being disabled on an
			 * active CRTC.
			 */
			if (!disabling && !plane_crtc_active(plane->state))
				continue;
			if (disabling && !plane_crtc_active(old_plane_state))
				continue;
		}

		/*
		 * Special-case disabling the plane if drivers support it.
		 */
		if (disabling && funcs->atomic_disable)
			funcs->atomic_disable(plane, old_plane_state);
		else if (plane->state->crtc || disabling)
			funcs->atomic_update(plane, old_plane_state);
	}

	for_each_crtc_in_state(old_state, crtc, old_crtc_state, i) {
		const struct drm_crtc_helper_funcs *funcs;

		funcs = crtc->helper_private;

		if (!funcs || !funcs->atomic_flush)
			continue;

		if (active_only && !crtc->state->active)
			continue;

		funcs->atomic_flush(crtc, old_crtc_state);
	}
}
EXPORT_SYMBOL(drm_atomic_helper_commit_planes);

/**
 * drm_atomic_helper_commit_planes_on_crtc - commit plane state for a crtc
 * @old_crtc_state: atomic state object with the old crtc state
 *
 * This function commits the new plane state using the plane and atomic helper
 * functions for planes on the specific crtc. It assumes that the atomic state
 * has already been pushed into the relevant object state pointers, since this
 * step can no longer fail.
 *
 * This function is useful when plane updates should be done crtc-by-crtc
 * instead of one global step like drm_atomic_helper_commit_planes() does.
 *
 * This function can only be savely used when planes are not allowed to move
 * between different CRTCs because this function doesn't handle inter-CRTC
 * depencies. Call