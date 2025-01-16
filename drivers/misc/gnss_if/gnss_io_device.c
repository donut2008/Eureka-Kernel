6,
	IMG_DATA_FORMAT_RESERVED_23                      = 0x17,
	IMG_DATA_FORMAT_RESERVED_24                      = 0x18,
	IMG_DATA_FORMAT_RESERVED_25                      = 0x19,
	IMG_DATA_FORMAT_RESERVED_26                      = 0x1a,
	IMG_DATA_FORMAT_RESERVED_27                      = 0x1b,
	IMG_DATA_FORMAT_RESERVED_28                      = 0x1c,
	IMG_DATA_FORMAT_RESERVED_29                      = 0x1d,
	IMG_DATA_FORMAT_RESERVED_30                      = 0x1e,
	IMG_DATA_FORMAT_RESERVED_31                      = 0x1f,
	IMG_DATA_FORMAT_GB_GR                            = 0x20,
	IMG_DATA_FORMAT_BG_RG                            = 0x21,
	IMG_DATA_FORMAT_5_9_9_9                          = 0x22,
	IMG_DATA_FORMAT_BC1                              = 0x23,
	IMG_DATA_FORMAT_BC2                              = 0x24,
	IMG_DATA_FORMAT_BC3                              = 0x25,
	IMG_DATA_FORMAT_BC4                              = 0x26,
	IMG_DATA_FORMAT_BC5                              = 0x27,
	IMG_DATA_FORMAT_BC6                              = 0x28,
	IMG_DATA_FORMAT_BC7                              = 0x29,
	IMG_DATA_FORMAT_RESERVED_42                      = 0x2a,
	IMG_DATA_FORMAT_RESERVED_43                      = 0x2b,
	IMG_DATA_FORMAT_FMASK8_S2_F1                     = 0x2c,
	IMG_DATA_FORMAT_FMASK8_S4_F1                     = 0x2d,
	IMG_DATA_FORMAT_FMASK8_S8_F1                     = 0x2e,
	IMG_DATA_FORMAT_FMASK8_S2_F2                     = 0x2f,
	IMG_DATA_FORMAT_FMASK8_S4_F2                     = 0x30,
	IMG_DATA_FORMAT_FMASK8_S4_F4                     = 0x31,
	IMG_DATA_FORMAT_FMASK16_S16_F1                   = 0x32,
	IMG_DATA_FORMAT_FMASK16_S8_F2                    = 0x33,
	IMG_DATA_FORMAT_FMASK32_S16_F2                   = 0x34,
	IMG_DATA_FORMAT_FMASK32_S8_F4                    = 0x35,
	IMG_DATA_FORMAT_FMASK32_S8_F8                    = 0x36,
	IMG_DATA_FORMAT_FMASK64_S16_F4                   = 0x37,
	IMG_DATA_FORMAT_FMASK64_S16_F8                   = 0x38,
	IMG_DATA_FORMAT_4_4                              = 0x39,
	IMG_DATA_FORMAT_6_5_5                            = 0x3a,
	IMG_DATA_FORMAT_1                                = 0x3b,
	IMG_DATA_FORMAT_1_REVERSED                       = 0x3c,
	IMG_DATA_FORMAT_32_AS_8                          = 0x3d,
	IMG_DATA_FORMAT_32_AS_8_8                        = 0x3e,
	IMG_DATA_FORMAT_32_AS_32_32_32_32                = 0x3f,
} IMG_DATA_FORMAT;
typedef enum BUF_NUM_FORMAT {
	BUF_NUM_FORMAT_UNORM                             = 0x0,
	BUF_NUM_FORMAT_SNORM                             = 0x1,
	BUF_NUM_FORMAT_USCALED                           = 0x2,
	BUF_NUM_FORMAT_SSCALED                           = 0x3,
	BUF_NUM_FORMAT_UINT                              = 0x4,
	BUF_NUM_FORMAT_SINT                              = 0x5,
	BUF_NUM_FORMAT_RESERVED_6                        = 0x6,
	BUF_NUM_FORMAT_FLOAT                             = 0x7,
} BUF_NUM_FORMAT;
typedef enum IMG_NUM_FORMAT {
	IMG_NUM_FORMAT_UNORM                             = 0x0,
	IMG_NUM_FORMAT_SNORM                             = 0x1,
	IMG_NUM_FORMAT_USCALED                           = 0x2,
	IMG_NUM_FORMAT_SSCALED                           = 0x3,
	IMG_NUM_FORMAT_UINT                              = 0x4,
	IMG_NUM_FORMAT_SINT                              = 0x5,
	IMG_NUM_FORMAT_RESERVED_6                        = 0x6,
	IMG_NUM_FORMAT_FLOAT                             = 0x7,
	IMG_NUM_FORMAT_RESERVED_8                        = 0x8,
	IMG_NUM_FORMAT_SRGB                              = 0x9,
	IMG_NUM_FORMAT_RESERVED_10                       = 0xa,
	IMG_NUM_FORMAT_RESERVED_11                       = 0xb,
	IMG_NUM_FORMAT_RESERVED_12                       = 0xc,
	IMG_NUM_FORMAT_RESERVED_13                       = 0xd,
	IMG_NUM_FORMAT_RESERVED_14                       = 0xe,
	IMG_NUM_FORMAT_RESERVED_15                       = 0xf,
} IMG_NUM_FORMAT;
typedef enum TileType {
	ARRAY_COLOR_TILE                                 = 0x0,
	ARRAY_DEPTH_TILE                                 = 0x1,
} TileType;
typedef enum NonDispTilingOrder {
	ADDR_SURF_MICRO_TILING_DISPLAY                   = 0x0,
	ADDR_SURF_MICRO_TILING_NON_DISPLAY               = 0x1,
} NonDispTilingOrder;
typedef enum MicroTileMode {
	ADDR_SURF_DISPLAY_MICRO_TILING                   = 0x0,
	ADDR_SURF_THIN_MICRO_TILING                      = 0x1,
	ADDR_SURF_DEPTH_MICRO_TILING                     = 0x2,
	ADDR_SURF_ROTATED_MICRO_TILING                   = 0x3,
	ADDR_SURF_THICK_MICRO_TILING                     = 0x4,
} MicroTileMode;
typedef enum TileSplit {
	ADDR_SURF_TILE_SPLIT_64B                         = 0x0,
	ADDR_SURF_TILE_SPLIT_128B                        = 0x1,
	ADDR_SURF_TILE_SPLIT_256B                        = 0x2,
	ADDR_SURF_TILE_SPLIT_512B                        = 0x3,
	ADDR_SURF_TILE_SPLIT_1KB                         = 0x4,
	ADDR_SURF_TILE_SPLIT_2KB                         = 0x5,
	ADDR_SURF_TILE_SPLIT_4KB                         = 0x6,
} TileSplit;
typedef enum SampleSplit {
	ADDR_SURF_SAMPLE_SPLIT_1                         = 0x0,
	ADDR_SURF_SAMPLE_SPLIT_2                         = 0x1,
	ADDR_SURF_SAMPLE_SPLIT_4                         = 0x2,
	ADDR_SURF_SAMPLE_SPLIT_8                         = 0x3,
} SampleSplit;
typedef enum PipeConfig {
	ADDR_SURF_P2                                     = 0x0,
	ADDR_SURF_P2_RESERVED0                           = 0x1,
	ADDR_SURF_P2_RESERVED1                           = 0x2,
	ADDR_SURF_P2_RESERVED2                           = 0x3,
	ADDR_SURF_P4_8x16                                = 0x4,
	ADDR_SURF_P4_16x16                               = 0x5,
	ADDR_SURF_P4_16x32                               = 0x6,
	ADDR_SURF_P4_32x32                               = 0x7,
	ADDR_SURF_P8_16x16_8x16                          = 0x8,
	ADDR_SURF_P8_16x32_8x16                          = 0x9,
	ADDR_SURF_P8_32x32_8x16                          = 0xa,
	ADDR_SURF_P8_16x32_16x16                         = 0xb,
	ADDR_SURF_P8_32x32_16x16                         = 0xc,
	ADDR_SURF_P8_32x32_16x32                         = 0xd,
	ADDR_SURF_P8_32x64_32x32                         = 0xe,
	ADDR_SURF_P8_RESERVED0                           = 0xf,
	ADDR_SURF_P16_32x32_8x16                         = 0x10,
	ADDR_SURF_P16_32x32_16x16                        = 0x11,
} PipeConfig;
typedef enum NumBanks {
	ADDR_SURF_2_BANK                                 = 0x0,
	ADDR_SURF_4_BANK                                 = 0x1,
	ADDR_SURF_8_BANK                                 = 0x2,
	ADDR_SURF_16_BANK                                = 0x3,
} NumBanks;
typedef enum BankWidth {
	ADDR_SURF_BANK_WIDTH_1                           = 0x0,
	ADDR_SURF_BANK_WIDTH_2                           = 0x1,
	ADDR_SURF_BANK_WIDTH_4                           = 0x2,
	ADDR_SURF_BANK_WIDTH_8                           = 0x3,
} BankWidth;
typedef enum BankHeight {
	ADDR_SURF_BANK_HEIGHT_1                          = 0x0,
	ADDR_SURF_BANK_HEIGHT_2                          = 0x1,
	ADDR_SURF_BANK_HEIGHT_4                          = 0x2,
	ADDR_SURF_BANK_HEIGHT_8                          = 0x3,
} BankHeight;
typedef enum BankWidthHeight {
	ADDR_SURF_BANK_WH_1                              = 0x0,
	ADDR_SURF_BANK_WH_2                              = 0x1,
	ADDR_SURF_BANK_WH_4                              = 0x2,
	ADDR_SURF_BANK_WH_8                              = 0x3,
} BankWidthHeight;
typedef enum MacroTileAspect {
	ADDR_SURF_MACRO_ASPECT_1                         = 0x0,
	ADDR_SURF_MACRO_ASPECT_2                         = 0x1,
	ADDR_SURF_MACRO_ASPECT_4                         = 0x2,
	ADDR_SURF_MACRO_ASPECT_8                         = 0x3,
} MacroTileAspect;
typedef enum GATCL1RequestType {
	GATCL1_TYPE_NORMAL                               = 0x0,
	GATCL1_TYPE_SHOOTDOWN                            = 0x1,
	GATCL1_TYPE_BYPASS                               = 0x2,
} GATCL1RequestType;
typedef enum TCC_CACHE_POLICIES {
	TCC_CACHE_POLICY_LRU                             = 0x0,
	TCC_CACHE_POLICY_STREAM                          = 0x1,
} TCC_CACHE_POLICIES;
typedef enum MTYPE {
	MTYPE_NC_NV                                      = 0x0,
	MTYPE_NC                                         = 0x1,
	MTYPE_CC                                         = 0x2,
	MTYPE_UC                                         = 0x3,
} MTYPE;
typedef enum PERFMON_COUNTER_MODE {
	PERFMON_COUNTER_MODE_ACCUM                       = 0x0,
	PERFMON_COUNTER_MODE_ACTIVE_CYCLES               = 0x1,
	PERFMON_COUNTER_MODE_MAX                         = 0x2,
	PERFMON_COUNTER_MODE_DIRTY                       = 0x3,
	PERFMON_COUNTER_MODE_SAMPLE                      = 0x4,
	PERFMON_COUNTER_MODE_CYCLES_SINCE_FIRST_EVENT    = 0x5,
	PERFMON_COUNTER_MODE_CYCLES_SINCE_LAST_EVENT     = 0x6,
	PERFMON_COUNTER_MODE_CYCLES_GE_HI                = 0x7,
	PERFMON_COUNTER_MODE_CYCLES_EQ_HI                = 0x8,
	PERFMON_COUNTER_MODE_INACTIVE_CYCLES             = 0x9,
	PERFMON_COUNTER_MODE_RESERVED                    = 0xf,
} PERFMON_COUNTER_MODE;
typedef enum PERFMON_SPM_MODE {
	PERFMON_SPM_MODE_OFF                             = 0x0,
	PERFMON_SPM_MODE_16BIT_CLAMP                     = 0x1,
	PERFMON_SPM_MODE_16BIT_NO_CLAMP                  = 0x2,
	PERFMON_SPM_MODE_32BIT_CLAMP                     = 0x3,
	PERFMON_SPM_MODE_32BIT_NO_CLAMP                  = 0x4,
	PERFMON_SPM_MODE_RESERVED_5                      = 0x5,
	PERFMON_SPM_MODE_RESERVED_6                      = 0x6,
	PERFMON_SPM_MODE_RESERVED_7                      = 0x7,
	PERFMON_SPM_MODE_TEST_MODE_0                     = 0x8,
	PERFMON_SPM_MODE_TEST_MODE_1                     = 0x9,
	PERFMON_SPM_MODE_TEST_MODE_2                     = 0xa,
} PERFMON_SPM_MODE;
typedef enum SurfaceTiling {
	ARRAY_LINEAR                                     = 0x0,
	ARRAY_TILED                                      = 0x1,
} SurfaceTiling;
typedef enum SurfaceArray {
	ARRAY_1D                                         = 0x0,
	ARRAY_2D                                         = 0x1,
	ARRAY_3D                                         = 0x2,
	ARRAY_3D_SLICE                                   = 0x3,
} SurfaceArray;
typedef enum ColorArray {
	ARRAY_2D_ALT_COLOR                               = 0x0,
	ARRAY_2D_COLOR                                   = 0x1,
	ARRAY_3D_SLICE_COLOR                             = 0x3,
} ColorArray;
typedef enum DepthArray {
	ARRAY_2D_ALT_DEPTH                               = 0x0,
	ARRAY_2D_DEPTH                                   = 0x1,
} DepthArray;
typedef enum ENUM_NUM_SIMD_PER_CU {
	NUM_SIMD_PER_CU                                  = 0x4,
} ENUM_NUM_SIMD_PER_CU;
typedef enum MEM_PWR_FORCE_CTRL {
	NO_FORCE_REQUEST                                 = 0x0,
	FORCE_LIGHT_SLEEP_REQUEST                        = 0x1,
	FORCE_DEEP_SLEEP_REQUEST                         = 0x2,
	FORCE_SHUT_DOWN_REQUEST                          = 0x3,
} MEM_PWR_FORCE_CTRL;
typedef enum MEM_PWR_FORCE_CTRL2 {
	NO_FORCE_REQ                                     = 0x0,
	FORCE_LIGHT_SLEEP_REQ                            = 0x1,
} MEM_PWR_FORCE_CTRL2;
typedef enum MEM_PWR_DIS_CTRL {
	ENABLE_MEM_PWR_CTRL                              = 0x0,
	DISABLE_MEM_PWR_CTRL                             = 0x1,
} MEM_PWR_DIS_CTRL;
typedef enum MEM_PWR_SEL_CTRL {
	DYNAMIC_SHUT_DOWN_ENABLE                         = 0x0,
	DYNAMIC_DEEP_SLEEP_ENABLE                        = 0x1,
	DYNAMIC_LIGHT_SLEEP_ENABLE                       = 0x2,
} MEM_PWR_SEL_CTRL;
typedef enum MEM_PWR_SEL_CTRL2 {
	DYNAMIC_DEEP_SLEEP_EN                            = 0x0,
	DYNAMIC_LIGHT_SLEEP_EN                           = 0x1,
} MEM_PWR_SEL_CTRL2;

#endif /* SMU_7_1_3_ENUM_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * Copyright (C) 2014 Red Hat
 * Copyright (C) 2014 Intel Corp.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 * Rob Clark <robdclark@gmail.com>
 * Daniel Vetter <daniel.vetter@ffwll.ch>
 */

#include <drm/drmP.h>
#include <drm/drm_atomic.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <linux/fence.h>

/**
 * DOC: overview
 *
 * This helper library provides implementations of check and commit functions on
 * top of the CRTC modeset helper callbacks and the plane helper callbacks. It
 * also provides convenience implementations for the atomic state handling
 * callbacks for drivers which don't need to subclass the drm core structures to
 * add their own additional internal state.
 *
 * This library also provides default implementations for the check callback in
 * drm_atomic_helper_check() and for the commit callback with
 * drm_atomic_helper_commit(). But the individual stages and callbacks are
 * exposed to allow drivers to mix and match and e.g. use the plane helpers only
 * together with a driver private modeset implementation.
 *
 * This library also provides implementations for all the legacy driver
 * interfaces on top of the atomic interface. See drm_atomic_helper_set_config(),
 * drm_atomic_helper_disable_plane(), drm_atomic_helper_disable_plane() and the
 * various functions to implement set_property callbacks. New drivers must not
 * implement these functions themselves but must use the provided helpers.
 */
static void
drm_atomic_helper_plane_changed(struct drm_atomic_state *state,
				struct drm_plane_state *plane_state,
				struct drm_plane *plane)
{
	struct drm_crtc_state *crtc_state;

	if (plane->state->crtc) {
		crtc_state = state->crtc_states[drm_crtc_index(plane->state->crtc)];

		if (WARN_ON(!crtc_state))
			return;

		crtc_state->planes_changed = true;
	}

	if (plane_state->crtc) {
		crtc_state =
			state->crtc_states[drm_crtc_index(plane_state->crtc)];

		if (WARN_ON(!crtc_state))
			return;

		crtc_state->planes_changed = true;
	}
}

static struct drm_crtc *
get_current_crtc_for_encoder(struct drm_device *dev,
			     struct drm_encoder *encoder)
{
	struct drm_mode_config *config = &dev->mode_config;
	struct drm_connector *connector;

	WARN_ON(!drm_modeset_is_locked(&config->connection_mutex));

	drm_for_each_connector(connector, dev) {
		if (connector->state->best_encoder != encoder)
			continue;

		return connector->state->crtc;
	}

	return NULL;
}

static int
steal_encoder(struct drm_atomic_state *state,
	      struct drm_encoder *encoder,
	      struct drm_crtc *encoder_crtc)
{
	struct drm_mode_config *config = &state->dev->mode_config;
	struct drm_crtc_state *crtc_state;
	struct drm_connector *connector;
	struct drm_connector_state *connector_state;

	/*
	 * We can only steal an encoder coming from a connector, which means we
	 * must already hold the connection_mutex.
	 */
	WARN_ON(!drm_modeset_is_locked(&config->connection_mutex));

	DRM_DEBUG_ATOMIC("[ENCODER:%d:%s] in use on [CRTC:%d], stealing it\n",
			 encoder->base.id, encoder->name,
			 encoder_crtc->base.id);

	crtc_state = drm_atomic_get_crtc_state(state, encoder_crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	crtc_state->connectors_changed = true;

	list_for_each_entry(connector, &config->connector_list, head) {
		if (connector->state->best_encoder != encoder)
			continue;

		DRM_DEBUG_ATOMIC("Stealing encoder from [CONNECTOR:%d:%s]\n",
				 connector->base.id,
				 connector->name);

		connector_state = drm_atomic_get_connector_state(state,
								 connector);
		if (IS_ERR(connector_state))
			return PTR_ERR(connector_state);

		connector_state->best_encoder = NULL;
	}

	return 0;
}

static int
update_connector_routing(struct drm_atomic_state *state, int conn_idx)
{
	const struct drm_connector_helper_funcs *funcs;
	struct drm_encoder *new_encoder;
	struct drm_crtc *encoder_crtc;
	struct drm_connector *connector;
	struct drm_connector_state *connector_state;
	struct drm_crtc_state *crtc_state;
	int idx, ret;

	connector = state->connectors[conn_idx];
	connector_state = state->connector_states[conn_idx];

	if (!connector)
		return 0;

	DRM_DEBUG_ATOMIC("Updating routing for [CONNECTOR:%d:%s]\n",
			 connector->base.id,
			 connector->name);

	if (connector->state->crtc != connector_state->crtc) {
		if (connector->state->crtc) {
			idx = drm_crtc_index(connector->state->crtc);

			crtc_state = state->crtc_states[idx];
			crtc_state->connectors_changed = true;
		}

		if (connector_state->crtc) {
			idx = drm_crtc_index(connector_state->crtc);

			crtc_state = state->crtc_states[idx];
			crtc_state->connectors_changed = true;
		}
	}

	if (!connector_state->crtc) {
		DRM_DEBUG_ATOMIC("Disabling [CONNECTOR:%d:%s]\n",
				connector->base.id,
				connector->name);

		connector_state->best_encoder = NULL;

		return 0;
	}

	funcs = connector->helper_private;

	if (funcs->atomic_best_encoder)
		new_encoder = funcs->atomic_best_encoder(connector,
							 connector_state);
	else
		new_encoder = funcs->best_encoder(connector);

	if (!new_encoder) {
		DRM_DEBUG_ATOMIC("No suitable encoder found for [CONNECTOR:%d:%s]\n",
				 connector->base.id,
				 connector->name);
		return -EINVAL;
	}

	if (!drm_encoder_crtc_ok(new_encoder, connector_state->crtc)) {
		DRM_DEBUG_ATOMIC("[ENCODER:%d:%s] incompatible with [CRTC:%d]\n",
				 new_encoder->base.id,
				 new_encoder->name,
				 connector_state->crtc->base.id);
		return -EINVAL;
	}

	if (new_encoder == connector_state->best_encoder) {
		DRM_DEBUG_ATOMIC("[CONNECTOR:%d:%s] keeps [ENCODER:%d:%s], now on [CRTC:%d]\n",
				 connector->base.id,
				 connector->name,
				 new_encoder->base.id,
				 new_encoder->name,
				 connector_state->crtc->base.id);

		return 0;
	}

	encoder_crtc = get_current_crtc_for_encoder(state->dev,
						    new_encoder);

	if (encoder_crtc) {
		ret = steal_encoder(state, new_encoder, encoder_crtc);
		if (ret) {
			DRM_DEBUG_ATOMIC("Encoder stealing failed for [CONNECTOR:%d:%s]\n",
					 connector->base.id,
					 connector->name);
			return ret;
		}
	}

	if (WARN_ON(!connector_state->crtc))
		return -EINVAL;

	connector_state->best_encoder = new_encoder;
	idx = drm_crtc_index(connector_state->crtc);

	crtc_state = state->crtc_states[idx];
	crtc_state->connectors_changed = true;

	DRM_DEBUG_ATOMIC("[CONNECTOR:%d:%s] using [ENCODER:%d:%s] on [CRTC:%d]\n",
			 connector->base.id,
			 connector->name,
			 new_encoder->base.id,
			 new_encoder->name,
			 connector_state->crtc->base.id);

	return 0;
}

static int
mode_fixup(struct drm_atomic_state *state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	struct drm_connector *connector;
	struct drm_connector_state *conn_state;
	int i;
	int ret;

	for_each_crtc_in_state(state, crtc, crtc_state, i) {
		if (!crtc_state->mode_changed &&
		    !crtc_state->connectors_changed)
			continue;

		drm_mode_copy(&crtc_state->adjusted_mode, &crtc_state->mode);
	}

	for_each_connector_in_state(state, connector, conn_state, i) {
		const struct drm_encoder_helper_funcs *funcs;
		struct drm_encoder *encoder;

		WARN_ON(!!conn_state->best_encoder != !!conn_state->crtc);

		if (!conn_state->crtc || !conn_state->best_encoder)
			continue;

		crtc_state =
			state->crtc_states[drm_crtc_index(conn_state->crtc)];

		/*
		 * Each encoder has at most one connector (since we always steal
		 * it away), so we won't call ->mode_fixup twice.
		 */
		encoder = conn_state->best_encoder;
		funcs = encoder->helper_private;
		if (!funcs)
			continue;

		ret = drm_bridge_mode_fixup(encoder->bridge, &crtc_state->mode,
				&crtc_state->adjusted_mode);
		if (!ret) {
			DRM_DEBUG_ATOMIC("Bridge fixup failed\n");
			return -EINVAL;
		}

		if (funcs->atomic_check) {
			ret = funcs->atomic_check(encoder, crtc_state,
						  conn_state);
			if (ret) {
				DRM_DEBUG_ATOMIC("[ENCODER:%d:%s] check failed\n",
						 encoder->base.id, encoder->name);
				return ret;
			}
		} else if (funcs->mode_fixup) {
			ret = funcs->mode_fixup(encoder, &crtc_state->mode,
						&crtc_state->adjusted_mode);
			if (!ret) {
				DRM_DEBUG_ATOMIC("[ENCODER:%d:%s] fixup failed\n",
						 encoder->base.id, encoder->name);
				return -EINVAL;
			}
		}
	}

	for_each_crtc_in_state(state, crtc, crtc_state, i) {
		const struct drm_crtc_helper_funcs *funcs;

		if (!crtc_state->mode_changed &&
		    !crtc_state->connectors_changed)
			continue;

		funcs = crtc->helper_private