/*
 *  Copyright (c) 2014 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <math.h>

#include "vp9/encoder/vp9_aq_cyclicrefresh.h"
#include "vp9/encoder/vp9_encoder.h"
#include "vp9/encoder/vp9_svc_layercontext.h"
#include "vp9/encoder/vp9_extend.h"
#include "vpx_dsp/vpx_dsp_common.h"

#define SMALL_FRAME_WIDTH 32
#define SMALL_FRAME_HEIGHT 16

void vp9_init_layer_context(VP9_COMP *const cpi) {
  SVC *const svc = &cpi->svc;
  const VP9EncoderConfig *const oxcf = &cpi->oxcf;
  int mi_rows = cpi->common.mi_rows;
  int mi_cols = cpi->common.mi_cols;
  int sl, tl, i;
  int alt_ref_idx = svc->number_spatial_layers;

  svc->spatial_layer_id = 0;
  svc->temporal_layer_id = 0;
  svc->first_spatial_layer_to_encode = 0;
  svc->force_zero_mode_spatial_ref = 0;
  svc->use_base_mv = 0;
  svc->use_partition_reuse = 0;
  svc->scaled_temp_is_alloc = 0;
  svc->scaled_one_half = 0;
  svc->current_superframe = 0;
  svc->non_reference_frame = 0;
  svc->skip_enhancement_layer = 0;
  svc->disable_inter_layer_pred = INTER_LAYER_PRED_ON;
  svc->framedrop_mode = CONSTRAINED_LAYER_DROP;

  for (i = 0; i < REF_FRAMES; ++i) {
    svc->fb_idx_spatial_layer_id[i] = -1;
    svc->fb_idx_temporal_layer_id[i] = -1;
  }
  for (sl = 0; sl < oxcf->ss_number_layers; ++sl) {
    svc->last_layer_dropped[sl] = 0;
    svc->drop_spatial_layer[sl] = 0;
    svc->ext_frame_flags[sl] = 0;
    svc->lst_fb_idx[sl] = 0;
    svc->gld_fb_idx[sl] = 1;
    svc->alt_fb_idx[sl] = 2;
    svc->downsample_filter_type[sl] = BILINEAR;
    svc->downsample_filter_phase[sl] = 8;  // Set to 8 for averaging filter.
    svc->framedrop_thresh[sl] = oxcf->drop_frames_water_mark;
    svc->fb_idx_upd_tl0[sl] = -1;
  }

  if (cpi->oxcf.error_resilient_mode == 0 && cpi->oxcf.pass == 2) {
    if (vpx_realloc_frame_buffer(&cpi->svc.empty_frame.img, SMALL_FRAME_WIDTH,
                                 SMALL_FRAME_HEIGHT, cpi->common.subsampling_x,
                                 cpi->common.subsampling_y,
#if CONFIG_VP9_HIGHBITDEPTH
                                 cpi->common.use_highbitdepth,
#endif
                                 VP9_ENC_BORDER_IN_PIXELS,
                                 cpi->common.byte_alignment, NULL, NULL, NULL))
      vpx_internal_error(&cpi->common.error, VPX_CODEC_MEM_ERROR,
                         "Failed to allocate empty frame for multiple frame "
                         "contexts");

    memset(cpi->svc.empty_frame.img.buffer_alloc, 0x80,
           cpi->svc.empty_frame.img.buffer_alloc_sz);
  }

  for (sl = 0; sl < oxcf->ss_number_layers; ++sl) {
    for (tl = 0; tl < oxcf->ts_number_layers; ++tl) {
      int layer = LAYER_IDS_TO_IDX(sl, tl, oxcf->ts_number_layers);
      LAYER_CONTEXT *const lc = &svc->layer_context[layer];
      RATE_CONTROL *const lrc = &lc->rc;
      int i;
      lc->current_video_frame_in_layer = 0;
      lc->layer_size = 0;
      lc->frames_from_key_frame = 0;
      lc->last_frame_type = FRAME_TYPES;
      lrc->ni_av_qi = oxcf->worst_allowed_q;
      lrc->total_actual_bits = 0;
      lrc->total_target_vs_actual = 0;
      lrc->ni_tot_qi = 0;
      lrc->tot_q = 0.0;
      lrc->avg_q = 0.0;
      lrc->ni_frames = 0;
      lrc->decimation_count = 0;
      lrc->decimation_factor = 0;

      for (i = 0; i < RATE_FACTOR_LEVELS; ++i) {
        lrc->rate_correction_factors[i] = 1.0;
      }

      if (cpi->oxcf.rc_mode == VPX_CBR) {
        lc->target_bandwidth = oxcf->layer_target_bitrate[layer];
        lrc->last_q[INTER_FRAME] = oxcf->worst_allowed_q;
        lrc->avg_frame_qindex[INTER_FRAME] = oxcf->worst_allowed_q;
        lrc->avg_frame_qindex[KEY_FRAME] = oxcf->worst_allowed_q;
      } else {
        lc->target_bandwidth = oxcf->layer_target_bitrate[layer];
        lrc->last_q[KEY_FRAME] = oxcf->best_allowed_q;
        lrc->last_q[INTER_FRAME] = oxcf->best_allowed_q;
        lrc->avg_frame_qindex[KEY_FRAME] =
            (oxcf->worst_allowed_q + oxcf->best_allowed_q) / 2;
        lrc->avg_frame_qindex[INTER_FRAME] =
            (oxcf->worst_allowed_q + oxcf->best_allowed_q) / 2;
        if (oxcf->ss_enable_auto_arf[sl])
          lc->alt_ref_idx = alt_ref_idx++;
        else
          lc->alt_ref_idx = INVALID_IDX;
        lc->gold_ref_idx = INVALID_IDX;
      }

      lrc->buffer_level =
          oxcf->starting_buffer_level_ms * lc->target_bandwidth / 1000;
      lrc->bits_off_target = lrc->buffer_level;

      // Initialize the cyclic refresh parameters. If spatial layers are used
      // (i.e., ss_number_layers > 1), these need to be updated per spatial
      // layer.
      // Cyclic refresh is only applied on base temporal layer.
      if (oxcf->ss_number_layers > 1 && tl == 0) {
        size_t last_coded_q_map_size;
        size_t consec_zero_mv_size;
        VP9_COMMON *const cm = &cpi->common;
        lc->sb_index = 0;
        CHECK_MEM_ERROR(cm, lc->map,
                        vpx_malloc(mi_rows * mi_cols * sizeof(*lc->map)));
        memset(lc->map, 0, mi_rows * mi_cols);
        last_coded_q_map_size =
            mi_rows * mi_cols * sizeof(*lc->last_coded_q_map);
        CHECK_MEM_ERROR(cm, lc->last_coded_q_map,
                        vpx_malloc(last_coded_q_map_size));
        assert(MAXQ <= 255);
        memset(lc->last_coded_q_map, MAXQ, last_coded_q_map_size);
        consec_zero_mv_size = mi_rows * mi_cols * sizeof(*lc->consec_zero_mv);
        CHECK_MEM_ERROR(cm, lc->consec_zero_mv,
                        vpx_malloc(consec_zero_mv_size));
        memset(lc->consec_zero_mv, 0, consec_zero_mv_size);
      }
    }
  }

  // Still have extra buffer for base layer golden frame
  if (!(svc->number_temporal_layers > 1 && cpi->oxcf.rc_mode == VPX_CBR) &&
      alt_ref_idx < REF_FRAMES)
    svc->layer_context[0].gold_ref_idx = alt_ref_idx;
}

// Update the layer context from a change_config() call.
void vp9_update_layer_context_change_config(VP9_COMP *const cpi,
                                            const int target_bandwidth) {
  SVC *const svc = &cpi->svc;
  const VP9EncoderConfig *const oxcf = &cpi->oxcf;
  const RATE_CONTROL *const rc = &cpi->rc;
  int sl, tl, layer = 0, spatial_layer_target;
  float bitrate_alloc = 1.0;

  cpi->svc.temporal_layering_mode = oxcf->temporal_layering_mode;

  if (svc->temporal_layering_mode != VP9E_TEMPORAL_LAYERING_MODE_NOLAYERING) {
    for (sl = 0; sl < oxcf->ss_number_layers; ++sl) {
      for (tl = 0; tl < oxcf->ts_number_layers; ++tl) {
        layer = LAYER_IDS_TO_IDX(sl, tl, oxcf->ts_number_layers);
        svc->layer_context[layer].target_bandwidth =
            oxcf->layer_target_bitrate[layer];
      }

      layer = LAYER_IDS_TO_IDX(
          sl,
          ((oxcf->ts_number_layers - 1) < 0 ? 0 : (oxcf->ts_number_layers - 1)),
          oxcf->ts_number_layers);
      spatial_layer_target = svc->layer_context[layer].target_bandwidth =
          oxcf->layer_target_bitrate[layer];

      for (tl = 0; tl < oxcf->ts_number_layers; ++tl) {
        LAYER_CONTEXT *const lc =
            &svc->layer_context[sl * oxcf->ts_number_layers + tl];
        RATE_CONTROL *const lrc = &lc->rc;

        lc->spatial_layer_target_bandwidth = spatial_layer_target;
        bitrate_alloc = (float)lc->target_bandwidth / target_bandwidth;
        lrc->starting_buffer_level =
            (int64_t)(rc->starting_buffer_level * bitrate_alloc);
        lrc->optimal_buffer_level =
            (int64_t)(rc->optimal_buffer_level * bitrate_alloc);
        lrc->maximum_buffer_size =
            (int64_t)(rc->maximum_buffer_size * bitrate_alloc);
        lrc->bits_off_target =
            VPXMIN(lrc->bits_off_target, lrc->maximum_buffer_size);
        lrc->buffer_level = VPXMIN(lrc->buffer_level, lrc->maximum_buffer_size);
        lc->framerate = cpi->framerate / oxcf->ts_rate_decimator[tl];
        lrc->avg_frame_bandwidth = (int)(lc->target_bandwidth / lc->framerate);
        lrc->max_frame_bandwidth = rc->max_frame_bandwidth;
        lrc->worst_quality = rc->worst_quality;
        lrc->best_quality = rc->best_quality;
      }
    }
  } else {
    int layer_end;

    if (svc->number_temporal_layers > 1 && cpi->oxcf.rc_mode == VPX_CBR) {
      layer_end = svc->number_temporal_layers;
    } else {
      layer_end = svc->number_spatial_layers;
    }

    for (layer = 0; layer < layer_end; ++layer) {
      LAYER_CONTEXT *const lc = &svc->layer_context[layer];
      RATE_CONTROL *const lrc = &lc->rc;

      lc->target_bandwidth = oxcf->layer_target_bitrate[layer];

      bitrate_alloc = (float)lc->target_bandwidth / target_bandwidth;
      // Update buffer-related quantities.
      lrc->starting_buffer_level =
          (int64_t)(rc->starting_buffer_level * bitrate_alloc);
      lrc->optimal_buffer_level =
          (int64_t)(rc->optimal_buffer_level * bitrate_alloc);
      lrc->maximum_buffer_size =
          (int64_t)(rc->maximum_buffer_size * bitrate_alloc);
      lrc->bits_off_target =
          VPXMIN(lrc->bits_off_target, lrc->maximum_buffer_size);
      lrc->buffer_level = VPXMIN(lrc->buffer_level, lrc->maximum_buffer_size);
      // Update framerate-related quantities.
      if (svc->number_temporal_layers > 1 && cpi->oxcf.rc_mode == VPX_CBR) {
        lc->framerate = cpi->framerate / oxcf->ts_rate_decimator[layer];
      } else {
        lc->framerate = cpi->framerate;
      }
      lrc->avg_frame_bandwidth = (int)(lc->target_bandwidth / lc->framerate);
      lrc->max_frame_bandwidth = rc->max_frame_bandwidth;
      // Update qp-related quantities.
      lrc->worst_quality = rc->worst_quality;
      lrc->best_quality = rc->best_quality;
    }
  }
}

static LAYER_CONTEXT *get_layer_context(VP9_COMP *const cpi) {
  if (is_one_pass_cbr_svc(cpi))
    return &cpi->svc.layer_context[cpi->svc.spatial_layer_id *
                                       cpi->svc.number_temporal_layers +
                                   cpi->svc.temporal_layer_id];
  else
    return (cpi->svc.number_temporal_layers > 1 && cpi->oxcf.rc_mode == VPX_CBR)
               ? &cpi->svc.layer_context[cpi->svc.temporal_layer_id]
               : &cpi->svc.layer_context[cpi->svc.spatial_layer_id];
}

void vp9_update_temporal_layer_framerate(VP9_COMP *const cpi) {
  SVC *const svc = &cpi->svc;
  const VP9EncoderConfig *const oxcf = &cpi->oxcf;
  LAYER_CONTEXT *const lc = get_layer_context(cpi);
  RATE_CONTROL *const lrc = &lc->rc;
  // Index into spatial+temporal arrays.
  const int st_idx = svc->spatial_layer_id * svc->number_temporal_layers +
                     svc->temporal_layer_id;
  const int tl = svc->temporal_layer_id;

  lc->framerate = cpi->framerate / oxcf->ts_rate_decimator[tl];
  lrc->avg_frame_bandwidth = (int)(lc->target_bandwidth / lc->framerate);
  lrc->max_frame_bandwidth = cpi->rc.max_frame_bandwidth;
  // Update the average layer frame size (non-cumulative per-frame-bw).
  if (tl == 0) {
    lc->avg_frame_size = lrc->avg_frame_bandwidth;
  } else {
    const double prev_layer_framerate =
        cpi->framerate / oxcf->ts_rate_decimator[tl - 1];
    const int prev_layer_target_bandwidth =
        oxcf->layer_target_bitrate[st_idx - 1];
    lc->avg_frame_size =
        (int)((lc->target_bandwidth - prev_layer_target_bandwidth) /
              (lc->framerate - prev_layer_framerate));
  }
}

void vp9_update_spatial_layer_framerate(VP9_COMP *const cpi, double framerate) {
  const VP9EncoderConfig *const oxcf = &cpi->oxcf;
  LAYER_CONTEXT *const lc = get_layer_context(cpi);
  RATE_CONTROL *const lrc = &lc->rc;

  lc->framerate = framerate;
  lrc->avg_frame_bandwidth = (int)(lc->target_bandwidth / lc->framerate);
  lrc->min_frame_bandwidth =
      (int)(lrc->avg_frame_bandwidth * oxcf->two_pass_vbrmin_section / 100);
  lrc->max_frame_bandwidth = (int)(((int64_t)lrc->avg_frame_bandwidth *
                                    oxcf->two_pass_vbrmax_section) /
                                   100);
  vp9_rc_set_gf_interval_range(cpi, lrc);
}

void vp9_restore_layer_context(VP9_COMP *const cpi) {
  LAYER_CONTEXT *const lc = get_layer_context(cpi);
  const int old_frame_since_key = cpi->rc.frames_since_key;
  const int old_frame_to_key = cpi->rc.frames_to_key;

  cpi->rc = lc->rc;
  cpi->twopass = lc->twopass;
  cpi->oxcf.target_bandwidth = lc->target_bandwidth;
  cpi->alt_ref_source = lc->alt_ref_source;
  // Check if it is one_pass_cbr_svc mode and lc->speed > 0 (real-time mode
  // does not use speed = 0).
  if (is_one_pass_cbr_svc(cpi) && lc->speed > 0) {
    cpi->oxcf.speed = lc->speed;
  }
  // Reset the frames_since_key and frames_to_key counters to their values
  // before the layer restore. Keep these defined for the stream (not layer).
  if (cpi->svc.number_temporal_layers > 1 ||
      cpi->svc.number_spatial_layers > 1) {
    cpi->rc.frames_since_key = old_frame_since_key;
    cpi->rc.frames_to_key = old_frame_to_key;
  }

  // For spatial-svc, allow cyclic-refresh to be applied on the spatial layers,
  // for the base temporal layer.
  if (cpi->oxcf.aq_mode == CYCLIC_REFRESH_AQ &&
      cpi->svc.number_spatial_layers > 1 && cpi->svc.temporal_layer_id == 0) {
    CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
    signed char *temp = cr->map;
    uint8_t *temp2 = cr->last_coded_q_map;
    uint8_t *temp3 = cpi->consec_zero_mv;
    cr->map = lc->map;
    lc->map = temp;
    cr->last_coded_q_map = lc->last_coded_q_map;
    lc->last_coded_q_map = temp2;
    cpi->consec_zero_mv = lc->consec_zero_mv;
    lc->consec_zero_mv = temp3;
    cr->sb_index = lc->sb_index;
  }
}

void vp9_save_layer_context(VP9_COMP *const cpi) {
  const VP9EncoderConfig *const oxcf = &cpi->oxcf;
  LAYER_CONTEXT *const lc = get_layer_context(cpi);

  lc->rc = cpi->rc;
  lc->twopass = cpi->twopass;
  lc->target_bandwidth = (int)oxcf->target_bandwidth;
  lc->alt_ref_source = cpi->alt_ref_source;

  // For spatial-svc, allow cyclic-refresh to be applied on the spatial layers,
  // for the base temporal layer.
  if (cpi->oxcf.aq_mode == CYCLIC_REFRESH_AQ &&
      cpi->svc.number_spatial_layers > 1 && cpi->svc.temporal_layer_id == 0) {
    CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
    signed char *temp = lc->map;
    uint8_t *temp2 = lc->last_coded_q_map;
    uint8_t *temp3 = lc->consec_zero_mv;
    lc->map = cr->map;
    cr->map = temp;
    lc->last_coded_q_map = cr->last_coded_q_map;
    cr->last_coded_q_map = temp2;
    lc->consec_zero_mv = cpi->consec_zero_mv;
    cpi->consec_zero_mv = temp3;
    lc->sb_index = cr->sb_index;
  }
}

#if !CONFIG_REALTIME_ONLY
void vp9_init_second_pass_spatial_svc(VP9_COMP *cpi) {
  SVC *const svc = &cpi->svc;
  int i;

  for (i = 0; i < svc->number_spatial_layers; ++i) {
    TWO_PASS *const twopass = &svc->layer_context[i].twopass;

    svc->spatial_layer_id = i;
    vp9_init_second_pass(cpi);

    twopass->total_stats.spatial_layer_id = i;
    twopass->total_left_stats.spatial_layer_id = i;
  }
  svc->spatial_layer_id = 0;
}
#endif  // !CONFIG_REALTIME_ONLY

void vp9_inc_frame_in_layer(VP9_COMP *const cpi) {
  LAYER_CONTEXT *const lc =
      &cpi->svc.layer_context[cpi->svc.spatial_layer_id *
                              cpi->svc.number_temporal_layers];
  ++lc->current_video_frame_in_layer;
  ++lc->frames_from_key_frame;
  if (cpi->svc.spatial_layer_id == cpi->svc.number_spatial_layers - 1)
    ++cpi->svc.current_superframe;
}

void get_layer_resolution(const int width_org, const int height_org,
                          const int num, const int den, int *width_out,
                          int *height_out) {
  int w, h;

  if (width_out == NULL || height_out == NULL || den == 0) return;

  w = width_org * num / den;
  h = height_org * num / den;

  // make height and width even to make chrome player happy
  w += w % 2;
  h += h % 2;

  *width_out = w;
  *height_out = h;
}

void reset_fb_idx_unused(VP9_COMP *const cpi) {
  // If a reference frame is not referenced or refreshed, then set the
  // fb_idx for that reference to the first one used/referenced.
  // This is to avoid setting fb_idx for a reference to a slot that is not
  // used/needed (i.e., since that reference is not referenced or refreshed).
  static const int flag_list[4] = { 0, VP9_LAST_FLAG, VP9_GOLD_FLAG,
                                    VP9_ALT_FLAG };
  MV_REFERENCE_FRAME ref_frame;
  MV_REFERENCE_FRAME first_ref = 0;
  int first_fb_idx = 0;
  int fb_idx[3] = { cpi->lst_fb_idx, cpi->gld_fb_idx, cpi->alt_fb_idx };
  for (ref_frame = LAST_FRAME; ref_frame <= ALTREF_FRAME; ref_frame++) {
    if (cpi->ref_frame_flags & flag_list[ref_frame]) {
      first_ref = ref_frame;
      first_fb_idx = fb_idx[ref_frame - 1];
      break;
    }
  }
  if (first_ref > 0) {
    if (first_ref != LAST_FRAME &&
        !(cpi->ref_frame_flags & flag_list[LAST_FRAME]) &&
        !cpi->ext_refresh_last_frame)
      cpi->lst_fb_idx = first_fb_idx;
    else if (first_ref != GOLDEN_FRAME &&
             !(cpi->ref_frame_flags & flag_list[GOLDEN_FRAME]) &&
             !cpi->ext_refresh_golden_frame)
      cpi->gld_fb_idx = first_fb_idx;
    else if (first_ref != ALTREF_FRAME &&
             !(cpi->ref_frame_flags & flag_list[ALTREF_FRAME]) &&
             !cpi->ext_refresh_alt_ref_frame)
      cpi->alt_fb_idx = first_fb_idx;
  }
}

// The function sets proper ref_frame_flags, buffer indices, and buffer update
// variables for temporal layering mode 3 - that does 0-2-1-2 temporal layering
// scheme.
static void set_flags_and_fb_idx_for_temporal_mode3(VP9_COMP *const cpi) {
  int frame_num_within_temporal_struct = 0;
  int spatial_id, temporal_id;
  spatial_id = cpi->svc.spatial_layer_id = cpi->svc.spatial_layer_to_encode;
  frame_num_within_temporal_struct =
      cpi->svc
          .layer_context[cpi->svc.spatial_layer_id *
                         cpi->svc.number_temporal_layers]
          .current_video_frame_in_layer %
      4;
  temporal_id = cpi->svc.temporal_layer_id =
      (frame_num_within_temporal_struct & 1)
          ? 2
          : (frame_num_within_temporal_struct >> 1);
  cpi->ext_refresh_last_frame = cpi->ext_refresh_golden_frame =
      cpi->ext_refresh_alt_ref_frame = 0;
  if (!temporal_id) {
    cpi->ext_refresh_frame_flags_pending = 1;
    cpi->ext_refresh_last_frame = 1;
    if (!spatial_id) {
      cpi->ref_frame_flags = VP9_LAST_FLAG;
    } else if (cpi->svc.layer_context[temporal_id].is_key_frame) {
      // base layer is a key frame.
      cpi->ref_frame_flags = VP9_LAST_FLAG;
      cpi->ext_refresh_last_frame = 0;
      cpi->ext_refresh_golden_frame = 1;
    } else {
      cpi->ref_frame_flags = VP9_LAST_FLAG | VP9_GOLD_FLAG;
    }
  } else if (temporal_id == 1) {
    cpi->ext_refresh_frame_flags_pending = 1;
    cpi->ext_refresh_alt_ref_frame = 1;
    if (!spatial_id) {
      cpi->ref_frame_flags = VP9_LAST_FLAG;
    } else {
      cpi->ref_frame_flags = VP9_LAST_FLAG | VP9_GOLD_FLAG;
    }
  } else {
    if (frame_num_within_temporal_struct == 1) {
      // the first tl2 picture
      if (spatial_id == cpi->svc.number_spatial_layers - 1) {  // top layer
        cpi->ext_refresh_frame_flags_pending = 1;
        if (!spatial_id)
          cpi->ref_frame_flags = VP9_LAST_FLAG;
        else
          cpi->ref_frame_flags = VP9_LAST_FLAG | VP9_GOLD_FLAG;
      } else if (!spatial_id) {
        cpi->ext_refresh_frame_flags_pending = 1;
        cpi->ext_refresh_alt_ref_frame = 1;
        cpi->ref_frame_flags = VP9_LAST_FLAG;
      } else if (spatial_id < cpi->svc.number_spatial_layers - 1) {
        cpi->ext_refresh_frame_flags_pending = 1;
        cpi->ext_refresh_alt_ref_frame = 1;
        cpi->ref_frame_flags = VP9_LAST_FLAG | VP9_GOLD_FLAG;
      }
    } else {
      //  The second tl2 picture
      if (spatial_id == cpi->svc.number_spatial_layers - 1) {  // top layer
        cpi->ext_refresh_frame_flags_pending = 1;
        if (!spatial_id)
          cpi->ref_frame_flags = VP9_LAST_FLAG;
        else
          cpi->ref_frame_flags = VP9_LAST_FLAG | VP9_GOLD_FLAG;
      } else if (!spatial_id) {
        cpi->ext_refresh_frame_flags_pending = 1;
        cpi->ref_frame_flags = VP9_LAST_FLAG;
        cpi->ext_refresh_alt_ref_frame = 1;
      } else {  // top layer
        cpi->ext_refresh_frame_flags_pending = 1;
        cpi->ref_frame_flags = VP9_LAST_FLAG | VP9_GOLD_FLAG;
        cpi->ext_refresh_alt_ref_frame = 1;
      }
    }
  }
  if (temporal_id == 0) {
    cpi->lst_fb_idx = spatial_id;
    if (spatial_id) {
      if (cpi->svc.layer_context[temporal_id].is_key_frame) {
        cpi->lst_fb_idx = spatial_id - 1;
        cpi->gld_fb_idx = spatial_id;
      } else {
        cpi->gld_fb_idx = spatial_id - 1;
      }
    } else {
      cpi->gld_fb_idx = 0;
    }
    cpi->alt_fb_idx = 0;
  } else if (temporal_id == 1) {
    cpi->lst_fb_idx = spatial_id;
    cpi->gld_fb_idx = cpi->svc.number_spatial_layers + spatial_id - 1;
    cpi->alt_fb_idx = cpi->svc.number_spatial_layers + spatial_id;
  } else if (frame_num_within_temporal_struct == 1) {
    cpi->lst_fb_idx = spatial_id;
    cpi->gld_fb_idx = cpi->svc.number_spatial_layers + spatial_id - 1;
    cpi->alt_fb_idx = cpi->svc.number_spatial_layers + spatial_id;
  } else {
    cpi->lst_fb_idx = cpi->svc.number_spatial_layers + spatial_id;
    cpi->gld_fb_idx = cpi->svc.number_spatial_layers + spatial_id - 1;
    cpi->alt_fb_idx = cpi->svc.number_spatial_layers + spatial_id;
  }

  reset_fb_idx_unused(cpi);
}

// The function sets proper ref_frame_flags, buffer indices, and buffer update
// variables for temporal layering mode 2 - that does 0-1-0-1 temporal layering
// scheme.
static void set_flags_and_fb_idx_for_temporal_mode2(VP9_COMP *const cpi) {
  int spatial_id, temporal_id;
  spatial_id = cpi->svc.spatial_layer_id = cpi->svc.spatial_layer_to_encode;
  temporal_id = cpi->svc.temporal_layer_id =
      cpi->svc
          .layer_context[cpi->svc.spatial_layer_id *
                         cpi->svc.number_temporal_layers]
          .current_video_frame_in_layer &
      1;
  cpi->ext_refresh_last_frame = cpi->ext_refresh_golden_frame =
      cpi->ext_refresh_alt_ref_frame = 0;
  if (!temporal_id) {
    cpi->ext_refresh_frame_flags_pending = 1;
    cpi->ext_refresh_last_frame = 1;
    if (!spatial_id) {
      cpi->ref_frame_flags = VP9_LAST_FLAG;
    } else if (cpi->svc.layer_context[temporal_id].is_key_frame) {
      // base layer is a key frame.
      cpi->ref_frame_flags = VP9_LAST_FLAG;
      cpi->ext_refresh_last_frame = 0;
      cpi->ext_refresh_golden_frame = 1;
    } else {
      cpi->ref_frame_flags = VP9_LAST_FLAG | VP9_GOLD_FLAG;
    }
  } else if (temporal_id == 1) {
    cpi->ext_refresh_frame_flags_pending = 1;
    cpi->ext_refresh_alt_ref_frame = 1;
    if (!spatial_id) {
      cpi->ref_frame_flags = VP9_LAST_FLAG;
    } else {
      if (spatial_id == cpi->svc.number_spatial_layers - 1)
        cpi->ext_refresh_alt_ref_frame = 0;
      cpi->ref_frame_flags = VP9_LAST_FLAG | VP9_GOLD_FLAG;
    }
  }

  if (temporal_id == 0) {
    cpi->lst_fb_idx = spatial_id;
    if (spatial_id) {
      if (cpi->svc.layer_context[temporal_id].is_key_frame) {
        cpi->lst_fb_idx = spatial_id - 1;
        cpi->gld_fb_idx = spatial_id;
      } else {
        cpi->gld_fb_idx = spatial_id - 1;
      }
    } else {
      cpi->gld_fb_idx = 0;
    }
    cpi->alt_fb_idx = 0;
  } else if (temporal_id == 1) {
    cpi->lst_fb_idx = spatial_id;
    cpi->gld_fb_idx = cpi->svc.number_spatial_layers + spatial_id - 1;
    cpi->alt_fb_idx = cpi->svc.number_spatial_layers + spatial_id;
  }

  reset_fb_idx_unused(cpi);
}

// The function sets proper ref_frame_flags, buffer indices, and buffer update
// variables for temporal layering mode 0 - that has no temporal layering.
static void set_flags_and_fb_idx_for_temporal_mode_noLayering(
    VP9_COMP *const cpi) {
  int spatial_id;
  spatial_id = cpi->svc.spatial_layer_id = cpi->svc.spatial_layer_to_encode;
  cpi->ext_refresh_last_frame = cpi->ext_refresh_golden_frame =
      cpi->ext_refresh_alt_ref_frame = 0;
  cpi->ext_refresh_frame_flags_pending = 1;
  cpi->ext_refresh_last_frame = 1;
  if (!spatial_id) {
    cpi->ref_frame_flags = VP9_LAST_FLAG;
  } else if (cpi->svc.layer_context[0].is_key_frame) {
    cpi->ref_frame_flags = VP9_LAST_FLAG;
    cpi->ext_refresh_last_frame = 0;
    cpi->ext_refresh_golden_frame = 1;
  } else {
    cpi->ref_frame_flags = VP9_LAST_FLAG | VP9_GOLD_FLAG;
  }
  cpi->lst_fb_idx = spatial_id;
  if (spatial_id) {
    if (cpi->svc.layer_context[0].is_key_frame) {
      cpi->lst_fb_idx = spatial_id - 1;
      cpi->gld_fb_idx = spatial_id;
    } else {
      cpi->gld_fb_idx = spatial_id - 1;
    }
  } else {
    cpi->gld_fb_idx = 0;
  }

  reset_fb_idx_unused(cpi);
}

void vp9_copy_flags_ref_update_idx(VP9_COMP *const cpi) {
  SVC *const svc = &cpi->svc;
  static const int flag_list[4] = { 0, VP9_LAST_FLAG, VP9_GOLD_FLAG,
                                    VP9_ALT_FLAG };
  int sl = svc->spatial_layer_id;
  svc->lst_fb_idx[sl] = cpi->lst_fb_idx;
  svc->gld_fb_idx[sl] = cpi->gld_fb_idx;
  svc->alt_fb_idx[sl] = cpi->alt_fb_idx;

  svc->update_last[sl] = (uint8_t)cpi->refresh_last_frame;
  svc->update_golden[sl] = (uint8_t)cpi->refresh_golden_frame;
  svc->update_altref[sl] = (uint8_t)cpi->refresh_alt_ref_frame;
  svc->reference_last[sl] =
      (uint8_t)(cpi->ref_frame_flags & flag_list[LAST_FRAME]);
  svc->reference_golden[sl] =
      (uint8_t)(cpi->ref_frame_flags & flag_list[GOLDEN_FRAME]);
  svc->reference_altref[sl] =
      (uint8_t)(cpi->ref_frame_flags & flag_list[ALTREF_FRAME]);
}

int vp9_one_pass_cbr_svc_start_layer(VP9_COMP *const cpi) {
  int width = 0, height = 0;
  LAYER_CONTEXT *lc = NULL;
  cpi->svc.skip_enhancement_layer = 0;
  if (cpi->svc.number_spatial_layers > 1) {
    cpi->svc.use_base_mv = 1;
    cpi->svc.use_partition_reuse = 1;
  }
  cpi->svc.force_zero_mode_spatial_ref = 1;
  cpi->svc.mi_stride[cpi->svc.spatial_layer_id] = cpi->common.mi_stride;

  if (cpi->svc.temporal_layering_mode == VP9E_TEMPORAL_LAYERING_MODE_0212) {
    set_flags_and_fb_idx_for_temporal_mode3(cpi);
  } else if (cpi->svc.temporal_layering_mode ==
             VP9E_TEMPORAL_LAYERING_MODE_NOLAYERING) {
    set_flags_and_fb_idx_for_temporal_mode_noLayering(cpi);
  } else if (cpi->svc.temporal_layering_mode ==
             VP9E_TEMPORAL_LAYERING_MODE_0101) {
    set_flags_and_fb_idx_for_temporal_mode2(cpi);
  } else if (cpi->svc.temporal_layering_mode ==
             VP9E_TEMPORAL_LAYERING_MODE_BYPASS) {
    // In the BYPASS/flexible mode, the encoder is relying on the application
    // to specify, for each spatial layer, the flags and buffer indices for the
    // layering.
    // Note that the check (cpi->ext_refresh_frame_flags_pending == 0) is
    // needed to support the case where the frame flags may be passed in via
    // vpx_codec_encode(), which can be used for the temporal-only svc case.
    // TODO(marpan): Consider adding an enc_config parameter to better handle
    // this case.
    if (cpi->ext_refresh_frame_flags_pending == 0) {
      int sl;
      cpi->svc.spatial_layer_id = cpi->svc.spatial_layer_to_encode;
      sl = cpi->svc.spatial_layer_id;
      vp9_apply_encoding_flags(cpi, cpi->svc.ext_frame_flags[sl]);
      cpi->lst_fb_idx = cpi->svc.lst_fb_idx[sl];
      cpi->gld_fb_idx = cpi->svc.gld_fb_idx[sl];
      cpi->alt_fb_idx = cpi->svc.alt_fb_idx[sl];
    }
  }

  // Reset the drop flags for all spatial layers, on the base layer.
  if (cpi->svc.spatial_layer_id == 0) {
    vp9_zero(cpi->svc.drop_spatial_layer);
    // TODO(jianj/marpan): Investigate why setting cpi->svc.lst/gld/alt_fb_idx
    // causes an issue with frame dropping and temporal layers, when the frame
    // flags are passed via the encode call (bypass mode). Issue is that we're
    // resetting ext_refresh_frame_flags_pending to 0 on frame drops.
    if (cpi->svc.temporal_layering_mode != VP9E_TEMPORAL_LAYERING_MODE_BYPASS) {
      memset(&cpi->svc.lst_fb_idx, -1, sizeof(cpi->svc.lst_fb_idx));
      memset(&cpi->svc.gld_fb_idx, -1, sizeof(cpi->svc.lst_fb_idx));
      memset(&cpi->svc.alt_fb_idx, -1, sizeof(cpi->svc.lst_fb_idx));
    }
    vp9_zero(cpi->svc.update_last);
    vp9_zero(cpi->svc.update_golden);
    vp9_zero(cpi->svc.update_altref);
    vp9_zero(cpi->svc.reference_last);
    vp9_zero(cpi->svc.reference_golden);
    vp9_zero(cpi->svc.reference_altref);
  }

  lc = &cpi->svc.layer_context[cpi->svc.spatial_layer_id *
                                   cpi->svc.number_temporal_layers +
                               cpi->svc.temporal_layer_id];

  // Setting the worst/best_quality via the encoder control: SET_SVC_PARAMETERS,
  // only for non-BYPASS mode for now.
  if (cpi->svc.temporal_layering_mode != VP9E_TEMPORAL_LAYERING_MODE_BYPASS) {
    RATE_CONTROL *const lrc = &lc->rc;
    lrc->worst_quality = vp9_quantizer_to_qindex(lc->max_q);
    lrc->best_quality = vp9_quantizer_to_qindex(lc->min_q);
  }

  get_layer_resolution(cpi->oxcf.width, cpi->oxcf.height,
                       lc->scaling_factor_num, lc->scaling_factor_den, &width,
                       &height);

  // Use Eightap_smooth for low resolutions.
  if (width * height <= 320 * 240)
    cpi->svc.downsample_filter_type[cpi->svc.spatial_layer_id] =
        EIGHTTAP_SMOOTH;
  // For scale factors > 0.75, set the phase to 0 (aligns decimated pixel
  // to source pixel).
  lc = &cpi->svc.layer_context[cpi->svc.spatial_layer_id *
                                   cpi->svc.number_temporal_layers +
                               cpi->svc.temporal_layer_id];
  if (lc->scaling_factor_num > (3 * lc->scaling_factor_den) >> 2)
    cpi->svc.downsample_filter_phase[cpi->svc.spatial_layer_id] = 0;

  // The usage of use_base_mv or partition_reuse assumes down-scale of 2x2.
  // For now, turn off use of base motion vectors and partition reuse if the
  // spatial scale factors for any layers are not 2,
  // keep the case of 3 spatial layers with scale factor of 4x4 for base layer.
  // TODO(marpan): Fix this to allow for use_base_mv for scale factors != 2.
  if (cpi->svc.number_spatial_layers > 1) {
    int sl;
    for (sl = 0; sl < cpi->svc.number_spatial_layers - 1; ++sl) {
      lc = &cpi->svc.layer_context[sl * cpi->svc.number_temporal_layers +
                                   cpi->svc.temporal_layer_id];
      if ((lc->scaling_factor_num != lc->scaling_factor_den >> 1) &&
          !(lc->scaling_factor_num == lc->scaling_factor_den >> 2 && sl == 0 &&
            cpi->svc.number_spatial_layers == 3)) {
        cpi->svc.use_base_mv = 0;
        cpi->svc.use_partition_reuse = 0;
        break;
      }
    }
    // For non-zero spatial layers: if the previous spatial layer was dropped
    // disable the base_mv and partition_reuse features.
    if (cpi->svc.spatial_layer_id > 0 &&
        cpi->svc.drop_spatial_layer[cpi->svc.spatial_layer_id - 1]) {
      cpi->svc.use_base_mv = 0;
      cpi->svc.use_partition_reuse = 0;
    }
  }

  cpi->svc.non_reference_frame = 0;
  if (cpi->common.frame_type != KEY_FRAME && !cpi->ext_refresh_last_frame &&
      !cpi->ext_refresh_golden_frame && !cpi->ext_refresh_alt_ref_frame) {
    cpi->svc.non_reference_frame = 1;
  }

  if (cpi->svc.spatial_layer_id == 0) cpi->svc.high_source_sad_superframe = 0;

  if (cpi->svc.temporal_layering_mode != VP9E_TEMPORAL_LAYERING_MODE_BYPASS &&
      cpi->svc.last_layer_dropped[cpi->svc.spatial_layer_id] &&
      cpi->svc.fb_idx_upd_tl0[cpi->svc.spatial_layer_id] != -1 &&
      !cpi->svc.layer_context[cpi->svc.temporal_layer_id].is_key_frame) {
    // For fixed/non-flexible mode, if the previous frame (same spatial layer
    // from previous superframe) was dropped, make sure the lst_fb_idx
    // for this frame corresponds to the buffer index updated on (last) encoded
    // TL0 frame (with same spatial layer).
    cpi->lst_fb_idx = cpi->svc.fb_idx_upd_tl0[cpi->svc.spatial_layer_id];
  }

  if (vp9_set_size_literal(cpi, width, height) != 0)
    return VPX_CODEC_INVALID_PARAM;

  return 0;
}

struct lookahead_entry *vp9_svc_lookahead_pop(VP9_COMP *const cpi,
                                              struct lookahead_ctx *ctx,
                                              int drain) {
  struct lookahead_entry *buf = NULL;
  if (ctx->sz && (drain || ctx->sz == ctx->max_sz - MAX_PRE_FRAMES)) {
    buf = vp9_lookahead_peek(ctx, 0);
    if (buf != NULL) {
      // Only remove the buffer when pop the highest layer.
      if (cpi->svc.spatial_layer_id == cpi->svc.number_spatial_layers - 1) {
        vp9_lookahead_pop(ctx, drain);
      }
    }
  }
  return buf;
}

void vp9_free_svc_cyclic_refresh(VP9_COMP *const cpi) {
  int sl, tl;
  SVC *const svc = &cpi->svc;
  const VP9EncoderConfig *const oxcf = &cpi->oxcf;
  for (sl = 0; sl < oxcf->ss_number_layers; ++sl) {
    for (tl = 0; tl < oxcf->ts_number_layers; ++tl) {
      int layer = LAYER_IDS_TO_IDX(sl, tl, oxcf->ts_number_layers);
      LAYER_CONTEXT *const lc = &svc->layer_context[layer];
      if (lc->map) vpx_free(lc->map);
      if (lc->last_coded_q_map) vpx_free(lc->last_coded_q_map);
      if (lc->consec_zero_mv) vpx_free(lc->consec_zero_mv);
    }
  }
}

// Reset on key frame: reset counters, references and buffer updates.
void vp9_svc_reset_key_frame(VP9_COMP *const cpi) {
  int sl, tl;
  SVC *const svc = &cpi->svc;
  LAYER_CONTEXT *lc = NULL;
  for (sl = 0; sl < svc->number_spatial_layers; ++sl) {
    for (tl = 0; tl < svc->number_temporal_layers; ++tl) {
      lc = &cpi->svc.layer_context[sl * svc->number_temporal_layers + tl];
      lc->current_video_frame_in_layer = 0;
      lc->frames_from_key_frame = 0;
    }
  }
  if (svc->temporal_layering_mode == VP9E_TEMPORAL_LAYERING_MODE_0212) {
    set_flags_and_fb_idx_for_temporal_mode3(cpi);
  } else if (svc->temporal_layering_mode ==
             VP9E_TEMPORAL_LAYERING_MODE_NOLAYERING) {
    set_flags_and_fb_idx_for_temporal_mode_noLayering(cpi);
  } else if (svc->temporal_layering_mode == VP9E_TEMPORAL_LAYERING_MODE_0101) {
    set_flags_and_fb_idx_for_temporal_mode2(cpi);
  }
  vp9_update_temporal_layer_framerate(cpi);
  vp9_restore_layer_context(cpi);
}

void vp9_svc_check_reset_layer_rc_flag(VP9_COMP *const cpi) {
  SVC *svc = &cpi->svc;
  int sl, tl;
  for (sl = 0; sl < svc->number_spatial_layers; ++sl) {
    // Check for reset based on avg_frame_bandwidth for spatial layer sl.
    int layer = LAYER_IDS_TO_IDX(sl, svc->number_temporal_layers - 1,
                                 svc->number_temporal_layers);
    LAYER_CONTEXT *lc = &svc->layer_context[layer];
    RATE_CONTROL *lrc = &lc->rc;
    if (lrc->avg_frame_bandwidth > (3 * lrc->last_avg_frame_bandwidth >> 1) ||
        lrc->avg_frame_bandwidth < (lrc->last_avg_frame_bandwidth >> 1)) {
      // Reset for all temporal layers with spatial layer sl.
      for (tl = 0; tl < svc->number_temporal_layers; ++tl) {
        int layer = LAYER_IDS_TO_IDX(sl, tl, svc->number_temporal_layers);
        LAYER_CONTEXT *lc = &svc->layer_context[layer];
        RATE_CONTROL *lrc = &lc->rc;
        lrc->rc_1_frame = 0;
        lrc->rc_2_frame = 0;
        lrc->bits_off_target = lrc->optimal_buffer_level;
        lrc->buffer_level = lrc->optimal_buffer_level;
      }
    }
  }
}

void vp9_svc_constrain_inter_layer_pred(VP9_COMP *const cpi) {
  VP9_COMMON *const cm = &cpi->common;
  // Check for disabling inter-layer (spatial) prediction, if
  // svc.disable_inter_layer_pred is set. If the previous spatial layer was
  // dropped then disable the prediction from this (scaled) reference.
  if ((cpi->svc.disable_inter_layer_pred == INTER_LAYER_PRED_OFF_NONKEY &&
       !cpi->svc.layer_context[cpi->svc.temporal_layer_id].is_key_frame) ||
      cpi->svc.disable_inter_layer_pred == INTER_LAYER_PRED_OFF ||
      cpi->svc.drop_spatial_layer[cpi->svc.spatial_layer_id - 1]) {
    MV_REFERENCE_FRAME ref_frame;
    static const int flag_list[4] = { 0, VP9_LAST_FLAG, VP9_GOLD_FLAG,
                                      VP9_ALT_FLAG };
    for (ref_frame = LAST_FRAME; ref_frame <= ALTREF_FRAME; ++ref_frame) {
      const YV12_BUFFER_CONFIG *yv12 = get_ref_frame_buffer(cpi, ref_frame);
      if (yv12 != NULL && (cpi->ref_frame_flags & flag_list[ref_frame])) {
        const struct scale_factors *const scale_fac =
            &cm->frame_refs[ref_frame - 1].sf;
        if (vp9_is_scaled(scale_fac))
          cpi->ref_frame_flags &= (~flag_list[ref_frame]);
      }
    }
  }
  // Check for disabling inter-layer prediction if the reference for inter-layer
  // prediction (the reference that is scaled) is not the previous spatial layer
  // from the same superframe, then we disable inter-layer prediction.
  // Only need to check when inter_layer prediction is not set to OFF mode.
  if (cpi->svc.disable_inter_layer_pred != INTER_LAYER_PRED_OFF) {
    // We only use LAST and GOLDEN for prediction in real-time mode, so we
    // check both here.
    MV_REFERENCE_FRAME ref_frame;
    for (ref_frame = LAST_FRAME; ref_frame <= GOLDEN_FRAME; ref_frame++) {
      struct scale_factors *scale_fac = &cm->frame_refs[ref_frame - 1].sf;
      if (vp9_is_scaled(scale_fac)) {
        // If this reference  was updated on the previous spatial layer of the
        // current superframe, then we keep this reference (don't disable).
        // Otherwise we disable the inter-layer prediction.
        // This condition is verified by checking if the current frame buffer
        // index is equal to any of the slots for the previous spatial layer,
        // and if so, check if that slot was updated/refreshed. If that is the
        // case, then this reference is valid for inter-layer prediction under
        // the mode INTER_LAYER_PRED_ON_CONSTRAINED.
        int fb_idx =
            ref_frame == LAST_FRAME ? cpi->lst_fb_idx : cpi->gld_fb_idx;
        int ref_flag = ref_frame == LAST_FRAME ? VP9_LAST_FLAG : VP9_GOLD_FLAG;
        int sl = cpi->svc.spatial_layer_id;
        int disable = 1;
        if ((fb_idx == cpi->svc.lst_fb_idx[sl - 1] &&
             cpi->svc.update_last[sl - 1]) ||
            (fb_idx == cpi->svc.gld_fb_idx[sl - 1] &&
             cpi->svc.update_golden[sl - 1]) ||
            (fb_idx == cpi->svc.alt_fb_idx[sl - 1] &&
             cpi->svc.update_altref[sl - 1]))
          disable = 0;
        if (disable) cpi->ref_frame_flags &= (~ref_flag);
      }
    }
  }
}

void vp9_svc_assert_constraints_pattern(VP9_COMP *const cpi) {
  SVC *const svc = &cpi->svc;
  // For fixed/non-flexible mode, and with CONSTRAINED frame drop
  // mode (default), the folllowing constraint are expected, when
  // inter-layer prediciton is on (default).
  if (svc->temporal_layering_mode != VP9E_TEMPORAL_LAYERING_MODE_BYPASS &&
      svc->disable_inter_layer_pred == INTER_LAYER_PRED_ON &&
      svc->framedrop_mode == CONSTRAINED_LAYER_DROP) {
    if (!cpi->svc.layer_context[cpi->svc.temporal_layer_id].is_key_frame) {
      // On non-key frames: LAST is always temporal reference, GOLDEN is
      // spatial reference.
      if (svc->temporal_layer_id == 0)
        // Base temporal only predicts from base temporal.
        assert(svc->fb_idx_temporal_layer_id[cpi->lst_fb_idx] == 0);
      else
        // Non-base temporal only predicts from lower temporal layer.
        assert(svc->fb_idx_temporal_layer_id[cpi->lst_fb_idx] <
               svc->temporal_layer_id);
      if (svc->spatial_layer_id > 0) {
        // Non-base spatial only predicts from lower spatial layer with same
        // temporal_id.
        assert(svc->fb_idx_spatial_layer_id[cpi->gld_fb_idx] ==
               svc->spatial_layer_id - 1);
        assert(svc->fb_idx_temporal_layer_id[cpi->gld_fb_idx] ==
               svc->temporal_layer_id);
      }
    } else if (svc->spatial_layer_id > 0) {
      // Only 1 reference for frame whose base is key; reference may be LAST
      // or GOLDEN, so we check both.
      if (cpi->ref_frame_flags & VP9_LAST_FLAG) {
        assert(svc->fb_idx_spatial_layer_id[cpi->lst_fb_idx] ==
               svc->spatial_layer_id - 1);
        assert(svc->fb_idx_temporal_layer_id[cpi->lst_fb_idx] ==
               svc->temporal_layer_id);
      } else if (cpi->ref_frame_flags & VP9_GOLD_FLAG) {
        assert(svc->fb_idx_spatial_layer_id[cpi->gld_fb_idx] ==
               svc->spatial_layer_id - 1);
        assert(svc->fb_idx_temporal_layer_id[cpi->gld_fb_idx] ==
               svc->temporal_layer_id);
      }
    }
  }
}
