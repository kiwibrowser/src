/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "./vpx_config.h"
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "test/codec_factory.h"
#include "test/encode_test_driver.h"
#include "test/i420_video_source.h"
#include "test/util.h"
#include "test/y4m_video_source.h"
#include "vpx/vpx_codec.h"
#include "vpx_ports/bitops.h"

namespace {

void AssignLayerBitrates(vpx_codec_enc_cfg_t *const enc_cfg,
                         const vpx_svc_extra_cfg_t *svc_params,
                         int spatial_layers, int temporal_layers,
                         int temporal_layering_mode,
                         int *layer_target_avg_bandwidth,
                         int64_t *bits_in_buffer_model) {
  int sl, spatial_layer_target;
  float total = 0;
  float alloc_ratio[VPX_MAX_LAYERS] = { 0 };
  float framerate = 30.0;
  for (sl = 0; sl < spatial_layers; ++sl) {
    if (svc_params->scaling_factor_den[sl] > 0) {
      alloc_ratio[sl] = (float)(svc_params->scaling_factor_num[sl] * 1.0 /
                                svc_params->scaling_factor_den[sl]);
      total += alloc_ratio[sl];
    }
  }
  for (sl = 0; sl < spatial_layers; ++sl) {
    enc_cfg->ss_target_bitrate[sl] = spatial_layer_target =
        (unsigned int)(enc_cfg->rc_target_bitrate * alloc_ratio[sl] / total);
    const int index = sl * temporal_layers;
    if (temporal_layering_mode == 3) {
      enc_cfg->layer_target_bitrate[index] = spatial_layer_target >> 1;
      enc_cfg->layer_target_bitrate[index + 1] =
          (spatial_layer_target >> 1) + (spatial_layer_target >> 2);
      enc_cfg->layer_target_bitrate[index + 2] = spatial_layer_target;
    } else if (temporal_layering_mode == 2) {
      enc_cfg->layer_target_bitrate[index] = spatial_layer_target * 2 / 3;
      enc_cfg->layer_target_bitrate[index + 1] = spatial_layer_target;
    } else if (temporal_layering_mode <= 1) {
      enc_cfg->layer_target_bitrate[index] = spatial_layer_target;
    }
  }
  for (sl = 0; sl < spatial_layers; ++sl) {
    for (int tl = 0; tl < temporal_layers; ++tl) {
      const int layer = sl * temporal_layers + tl;
      float layer_framerate = framerate;
      if (temporal_layers == 2 && tl == 0) layer_framerate = framerate / 2;
      if (temporal_layers == 3 && tl == 0) layer_framerate = framerate / 4;
      if (temporal_layers == 3 && tl == 1) layer_framerate = framerate / 2;
      layer_target_avg_bandwidth[layer] = static_cast<int>(
          enc_cfg->layer_target_bitrate[layer] * 1000.0 / layer_framerate);
      bits_in_buffer_model[layer] =
          enc_cfg->layer_target_bitrate[layer] * enc_cfg->rc_buf_initial_sz;
    }
  }
}

void CheckLayerRateTargeting(vpx_codec_enc_cfg_t *const cfg,
                             int number_spatial_layers,
                             int number_temporal_layers, double *file_datarate,
                             double thresh_overshoot,
                             double thresh_undershoot) {
  for (int sl = 0; sl < number_spatial_layers; ++sl)
    for (int tl = 0; tl < number_temporal_layers; ++tl) {
      const int layer = sl * number_temporal_layers + tl;
      ASSERT_GE(cfg->layer_target_bitrate[layer],
                file_datarate[layer] * thresh_overshoot)
          << " The datarate for the file exceeds the target by too much!";
      ASSERT_LE(cfg->layer_target_bitrate[layer],
                file_datarate[layer] * thresh_undershoot)
          << " The datarate for the file is lower than the target by too much!";
    }
}

class DatarateOnePassCbrSvc : public ::libvpx_test::EncoderTest {
 public:
  explicit DatarateOnePassCbrSvc(const ::libvpx_test::CodecFactory *codec)
      : EncoderTest(codec) {}

 protected:
  virtual ~DatarateOnePassCbrSvc() {}

  virtual void ResetModel() {
    last_pts_ = 0;
    duration_ = 0.0;
    mismatch_psnr_ = 0.0;
    mismatch_nframes_ = 0;
    denoiser_on_ = 0;
    tune_content_ = 0;
    base_speed_setting_ = 5;
    spatial_layer_id_ = 0;
    temporal_layer_id_ = 0;
    update_pattern_ = 0;
    memset(bits_in_buffer_model_, 0, sizeof(bits_in_buffer_model_));
    memset(bits_total_, 0, sizeof(bits_total_));
    memset(layer_target_avg_bandwidth_, 0, sizeof(layer_target_avg_bandwidth_));
    dynamic_drop_layer_ = false;
    change_bitrate_ = false;
    last_pts_ref_ = 0;
    middle_bitrate_ = 0;
    top_bitrate_ = 0;
    superframe_count_ = -1;
    key_frame_spacing_ = 9999;
    num_nonref_frames_ = 0;
    layer_framedrop_ = 0;
    force_key_ = 0;
    force_key_test_ = 0;
  }
  virtual void BeginPassHook(unsigned int /*pass*/) {}

  // Example pattern for spatial layers and 2 temporal layers used in the
  // bypass/flexible mode. The pattern corresponds to the pattern
  // VP9E_TEMPORAL_LAYERING_MODE_0101 (temporal_layering_mode == 2) used in
  // non-flexible mode, except that we disable inter-layer prediction.
  void set_frame_flags_bypass_mode(
      int tl, int num_spatial_layers, int is_key_frame,
      vpx_svc_ref_frame_config_t *ref_frame_config) {
    for (int sl = 0; sl < num_spatial_layers; ++sl) {
      if (!tl) {
        if (!sl) {
          ref_frame_config->frame_flags[sl] =
              VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_GF |
              VP8_EFLAG_NO_UPD_ARF;
        } else {
          if (is_key_frame) {
            ref_frame_config->frame_flags[sl] =
                VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_REF_ARF |
                VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_ARF;
          } else {
            ref_frame_config->frame_flags[sl] =
                VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_GF |
                VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_REF_GF;
          }
        }
      } else if (tl == 1) {
        if (!sl) {
          ref_frame_config->frame_flags[sl] =
              VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_REF_ARF |
              VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_GF;
        } else {
          ref_frame_config->frame_flags[sl] =
              VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_LAST |
              VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_REF_GF;
        }
      }
      if (tl == 0) {
        ref_frame_config->lst_fb_idx[sl] = sl;
        if (sl) {
          if (is_key_frame) {
            ref_frame_config->lst_fb_idx[sl] = sl - 1;
            ref_frame_config->gld_fb_idx[sl] = sl;
          } else {
            ref_frame_config->gld_fb_idx[sl] = sl - 1;
          }
        } else {
          ref_frame_config->gld_fb_idx[sl] = 0;
        }
        ref_frame_config->alt_fb_idx[sl] = 0;
      } else if (tl == 1) {
        ref_frame_config->lst_fb_idx[sl] = sl;
        ref_frame_config->gld_fb_idx[sl] = num_spatial_layers + sl - 1;
        ref_frame_config->alt_fb_idx[sl] = num_spatial_layers + sl;
      }
    }
  }

  virtual void PreEncodeFrameHook(::libvpx_test::VideoSource *video,
                                  ::libvpx_test::Encoder *encoder) {
    if (video->frame() == 0) {
      int i;
      for (i = 0; i < VPX_MAX_LAYERS; ++i) {
        svc_params_.max_quantizers[i] = 63;
        svc_params_.min_quantizers[i] = 0;
      }
      svc_params_.speed_per_layer[0] = base_speed_setting_;
      for (i = 1; i < VPX_SS_MAX_LAYERS; ++i) {
        svc_params_.speed_per_layer[i] = speed_setting_;
      }

      encoder->Control(VP9E_SET_NOISE_SENSITIVITY, denoiser_on_);
      encoder->Control(VP9E_SET_SVC, 1);
      encoder->Control(VP9E_SET_SVC_PARAMETERS, &svc_params_);
      encoder->Control(VP8E_SET_CPUUSED, speed_setting_);
      encoder->Control(VP9E_SET_TILE_COLUMNS, 0);
      encoder->Control(VP8E_SET_MAX_INTRA_BITRATE_PCT, 300);
      encoder->Control(VP9E_SET_TILE_COLUMNS, get_msb(cfg_.g_threads));
      encoder->Control(VP9E_SET_ROW_MT, 1);
      encoder->Control(VP8E_SET_STATIC_THRESHOLD, 1);
      encoder->Control(VP9E_SET_TUNE_CONTENT, tune_content_);

      if (layer_framedrop_) {
        vpx_svc_frame_drop_t svc_drop_frame;
        svc_drop_frame.framedrop_mode = LAYER_DROP;
        for (i = 0; i < number_spatial_layers_; i++)
          svc_drop_frame.framedrop_thresh[i] = 30;
        encoder->Control(VP9E_SET_SVC_FRAME_DROP_LAYER, &svc_drop_frame);
      }
    }

    superframe_count_++;
    temporal_layer_id_ = 0;
    if (number_temporal_layers_ == 2)
      temporal_layer_id_ = (superframe_count_ % 2 != 0);
    else if (number_temporal_layers_ == 3) {
      if (superframe_count_ % 2 != 0) temporal_layer_id_ = 2;
      if (superframe_count_ > 1) {
        if ((superframe_count_ - 2) % 4 == 0) temporal_layer_id_ = 1;
      }
    }

    if (update_pattern_ && video->frame() >= 100) {
      vpx_svc_layer_id_t layer_id;
      if (video->frame() == 100) {
        cfg_.temporal_layering_mode = VP9E_TEMPORAL_LAYERING_MODE_BYPASS;
        encoder->Config(&cfg_);
      }
      // Set layer id since the pattern changed.
      layer_id.spatial_layer_id = 0;
      layer_id.temporal_layer_id = (video->frame() % 2 != 0);
      temporal_layer_id_ = layer_id.temporal_layer_id;
      encoder->Control(VP9E_SET_SVC_LAYER_ID, &layer_id);
      set_frame_flags_bypass_mode(layer_id.temporal_layer_id,
                                  number_spatial_layers_, 0, &ref_frame_config);
      encoder->Control(VP9E_SET_SVC_REF_FRAME_CONFIG, &ref_frame_config);
    }

    if (change_bitrate_ && video->frame() == 200) {
      duration_ = (last_pts_ + 1) * timebase_;
      for (int sl = 0; sl < number_spatial_layers_; ++sl) {
        for (int tl = 0; tl < number_temporal_layers_; ++tl) {
          const int layer = sl * number_temporal_layers_ + tl;
          const double file_size_in_kb = bits_total_[layer] / 1000.;
          file_datarate_[layer] = file_size_in_kb / duration_;
        }
      }

      CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                              number_temporal_layers_, file_datarate_, 0.78,
                              1.15);

      memset(file_datarate_, 0, sizeof(file_datarate_));
      memset(bits_total_, 0, sizeof(bits_total_));
      int64_t bits_in_buffer_model_tmp[VPX_MAX_LAYERS];
      last_pts_ref_ = last_pts_;
      // Set new target bitarate.
      cfg_.rc_target_bitrate = cfg_.rc_target_bitrate >> 1;
      // Buffer level should not reset on dynamic bitrate change.
      memcpy(bits_in_buffer_model_tmp, bits_in_buffer_model_,
             sizeof(bits_in_buffer_model_));
      AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                          cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                          layer_target_avg_bandwidth_, bits_in_buffer_model_);
      memcpy(bits_in_buffer_model_, bits_in_buffer_model_tmp,
             sizeof(bits_in_buffer_model_));

      // Change config to update encoder with new bitrate configuration.
      encoder->Config(&cfg_);
    }

    if (dynamic_drop_layer_) {
      if (video->frame() == 0) {
        // Change layer bitrates to set top layers to 0. This will trigger skip
        // encoding/dropping of top two spatial layers.
        cfg_.rc_target_bitrate -=
            (cfg_.layer_target_bitrate[1] + cfg_.layer_target_bitrate[2]);
        middle_bitrate_ = cfg_.layer_target_bitrate[1];
        top_bitrate_ = cfg_.layer_target_bitrate[2];
        cfg_.layer_target_bitrate[1] = 0;
        cfg_.layer_target_bitrate[2] = 0;
        encoder->Config(&cfg_);
      } else if (video->frame() == 50) {
        // Change layer bitrates to non-zero on two top spatial layers.
        // This will trigger skip encoding of top two spatial layers.
        cfg_.layer_target_bitrate[1] = middle_bitrate_;
        cfg_.layer_target_bitrate[2] = top_bitrate_;
        cfg_.rc_target_bitrate +=
            cfg_.layer_target_bitrate[2] + cfg_.layer_target_bitrate[1];
        encoder->Config(&cfg_);
      } else if (video->frame() == 100) {
        // Change layer bitrates to set top layers to 0. This will trigger skip
        // encoding/dropping of top two spatial layers.
        cfg_.rc_target_bitrate -=
            (cfg_.layer_target_bitrate[1] + cfg_.layer_target_bitrate[2]);
        middle_bitrate_ = cfg_.layer_target_bitrate[1];
        top_bitrate_ = cfg_.layer_target_bitrate[2];
        cfg_.layer_target_bitrate[1] = 0;
        cfg_.layer_target_bitrate[2] = 0;
        encoder->Config(&cfg_);
      } else if (video->frame() == 150) {
        // Change layer bitrate on second layer to non-zero to start
        // encoding it again.
        cfg_.layer_target_bitrate[1] = middle_bitrate_;
        cfg_.rc_target_bitrate += cfg_.layer_target_bitrate[1];
        encoder->Config(&cfg_);
      } else if (video->frame() == 200) {
        // Change layer bitrate on top layer to non-zero to start
        // encoding it again.
        cfg_.layer_target_bitrate[2] = top_bitrate_;
        cfg_.rc_target_bitrate += cfg_.layer_target_bitrate[2];
        encoder->Config(&cfg_);
      }
    }

    if (force_key_test_ && force_key_)
      frame_flags_ = VPX_EFLAG_FORCE_KF;
    else
      frame_flags_ = 0;

    const vpx_rational_t tb = video->timebase();
    timebase_ = static_cast<double>(tb.num) / tb.den;
    duration_ = 0;
  }

  virtual void PostEncodeFrameHook(::libvpx_test::Encoder *encoder) {
    vpx_svc_layer_id_t layer_id;
    encoder->Control(VP9E_GET_SVC_LAYER_ID, &layer_id);
    temporal_layer_id_ = layer_id.temporal_layer_id;
    for (int sl = 0; sl < number_spatial_layers_; ++sl) {
      for (int tl = temporal_layer_id_; tl < number_temporal_layers_; ++tl) {
        const int layer = sl * number_temporal_layers_ + tl;
        bits_in_buffer_model_[layer] +=
            static_cast<int64_t>(layer_target_avg_bandwidth_[layer]);
      }
    }
  }

  vpx_codec_err_t parse_superframe_index(const uint8_t *data, size_t data_sz,
                                         uint32_t sizes[8], int *count) {
    uint8_t marker;
    marker = *(data + data_sz - 1);
    *count = 0;
    if ((marker & 0xe0) == 0xc0) {
      const uint32_t frames = (marker & 0x7) + 1;
      const uint32_t mag = ((marker >> 3) & 0x3) + 1;
      const size_t index_sz = 2 + mag * frames;
      // This chunk is marked as having a superframe index but doesn't have
      // enough data for it, thus it's an invalid superframe index.
      if (data_sz < index_sz) return VPX_CODEC_CORRUPT_FRAME;
      {
        const uint8_t marker2 = *(data + data_sz - index_sz);
        // This chunk is marked as having a superframe index but doesn't have
        // the matching marker byte at the front of the index therefore it's an
        // invalid chunk.
        if (marker != marker2) return VPX_CODEC_CORRUPT_FRAME;
      }
      {
        uint32_t i, j;
        const uint8_t *x = &data[data_sz - index_sz + 1];
        for (i = 0; i < frames; ++i) {
          uint32_t this_sz = 0;

          for (j = 0; j < mag; ++j) this_sz |= (*x++) << (j * 8);
          sizes[i] = this_sz;
        }
        *count = frames;
      }
    }
    return VPX_CODEC_OK;
  }

  virtual void FramePktHook(const vpx_codec_cx_pkt_t *pkt) {
    uint32_t sizes[8] = { 0 };
    uint32_t sizes_parsed[8] = { 0 };
    int count = 0;
    int num_layers_encoded = 0;
    last_pts_ = pkt->data.frame.pts;
    const bool key_frame =
        (pkt->data.frame.flags & VPX_FRAME_IS_KEY) ? true : false;
    if (key_frame) {
      temporal_layer_id_ = 0;
      superframe_count_ = 0;
    }
    parse_superframe_index(static_cast<const uint8_t *>(pkt->data.frame.buf),
                           pkt->data.frame.sz, sizes_parsed, &count);
    // Count may be less than number of spatial layers because of frame drops.
    for (int sl = 0; sl < number_spatial_layers_; ++sl) {
      if (pkt->data.frame.spatial_layer_encoded[sl]) {
        sizes[sl] = sizes_parsed[num_layers_encoded];
        num_layers_encoded++;
      }
    }
    ASSERT_EQ(count, num_layers_encoded);
    // In the constrained frame drop mode, if a given spatial is dropped all
    // upper layers must be dropped too.
    if (!layer_framedrop_) {
      int num_layers_dropped = 0;
      for (int sl = 0; sl < number_spatial_layers_; ++sl) {
        if (!pkt->data.frame.spatial_layer_encoded[sl]) {
          // Check that all upper layers are dropped.
          num_layers_dropped++;
          for (int sl2 = sl + 1; sl2 < number_spatial_layers_; ++sl2)
            ASSERT_EQ(pkt->data.frame.spatial_layer_encoded[sl2], 0);
        }
      }
      if (num_layers_dropped == number_spatial_layers_ - 1)
        force_key_ = 1;
      else
        force_key_ = 0;
    }
    // Keep track of number of non-reference frames, needed for mismatch check.
    // Non-reference frames are top spatial and temporal layer frames,
    // for TL > 0.
    if (temporal_layer_id_ == number_temporal_layers_ - 1 &&
        temporal_layer_id_ > 0 &&
        pkt->data.frame.spatial_layer_encoded[number_spatial_layers_ - 1])
      num_nonref_frames_++;
    for (int sl = 0; sl < number_spatial_layers_; ++sl) {
      sizes[sl] = sizes[sl] << 3;
      // Update the total encoded bits per layer.
      // For temporal layers, update the cumulative encoded bits per layer.
      for (int tl = temporal_layer_id_; tl < number_temporal_layers_; ++tl) {
        const int layer = sl * number_temporal_layers_ + tl;
        bits_total_[layer] += static_cast<int64_t>(sizes[sl]);
        // Update the per-layer buffer level with the encoded frame size.
        bits_in_buffer_model_[layer] -= static_cast<int64_t>(sizes[sl]);
        // There should be no buffer underrun, except on the base
        // temporal layer, since there may be key frames there.
        // Fo short key frame spacing, buffer can underrun on individual frames.
        if (!key_frame && tl > 0 && key_frame_spacing_ < 100) {
          ASSERT_GE(bits_in_buffer_model_[layer], 0)
              << "Buffer Underrun at frame " << pkt->data.frame.pts;
        }
      }

      ASSERT_EQ(pkt->data.frame.width[sl],
                top_sl_width_ * svc_params_.scaling_factor_num[sl] /
                    svc_params_.scaling_factor_den[sl]);

      ASSERT_EQ(pkt->data.frame.height[sl],
                top_sl_height_ * svc_params_.scaling_factor_num[sl] /
                    svc_params_.scaling_factor_den[sl]);
    }
  }

  virtual void EndPassHook(void) {
    if (change_bitrate_) last_pts_ = last_pts_ - last_pts_ref_;
    duration_ = (last_pts_ + 1) * timebase_;
    for (int sl = 0; sl < number_spatial_layers_; ++sl) {
      for (int tl = 0; tl < number_temporal_layers_; ++tl) {
        const int layer = sl * number_temporal_layers_ + tl;
        const double file_size_in_kb = bits_total_[layer] / 1000.;
        file_datarate_[layer] = file_size_in_kb / duration_;
      }
    }
  }

  virtual void MismatchHook(const vpx_image_t *img1, const vpx_image_t *img2) {
    double mismatch_psnr = compute_psnr(img1, img2);
    mismatch_psnr_ += mismatch_psnr;
    ++mismatch_nframes_;
  }

  unsigned int GetMismatchFrames() { return mismatch_nframes_; }

  vpx_codec_pts_t last_pts_;
  int64_t bits_in_buffer_model_[VPX_MAX_LAYERS];
  double timebase_;
  int64_t bits_total_[VPX_MAX_LAYERS];
  double duration_;
  double file_datarate_[VPX_MAX_LAYERS];
  size_t bits_in_last_frame_;
  vpx_svc_extra_cfg_t svc_params_;
  int speed_setting_;
  double mismatch_psnr_;
  int mismatch_nframes_;
  int denoiser_on_;
  int tune_content_;
  int base_speed_setting_;
  int spatial_layer_id_;
  int temporal_layer_id_;
  int number_spatial_layers_;
  int number_temporal_layers_;
  int layer_target_avg_bandwidth_[VPX_MAX_LAYERS];
  bool dynamic_drop_layer_;
  unsigned int top_sl_width_;
  unsigned int top_sl_height_;
  vpx_svc_ref_frame_config_t ref_frame_config;
  int update_pattern_;
  bool change_bitrate_;
  vpx_codec_pts_t last_pts_ref_;
  int middle_bitrate_;
  int top_bitrate_;
  int superframe_count_;
  int key_frame_spacing_;
  unsigned int num_nonref_frames_;
  int layer_framedrop_;
  int force_key_;
  int force_key_test_;
};

// Params: speed setting.
class DatarateOnePassCbrSvcSingleBR
    : public DatarateOnePassCbrSvc,
      public ::libvpx_test::CodecTestWithParam<int> {
 public:
  DatarateOnePassCbrSvcSingleBR() : DatarateOnePassCbrSvc(GET_PARAM(0)) {
    memset(&svc_params_, 0, sizeof(svc_params_));
  }
  virtual ~DatarateOnePassCbrSvcSingleBR() {}

 protected:
  virtual void SetUp() {
    InitializeConfig();
    SetMode(::libvpx_test::kRealTime);
    speed_setting_ = GET_PARAM(1);
    ResetModel();
  }
};

// Check basic rate targeting for 1 pass CBR SVC: 2 spatial layers and 1
// temporal layer, with screen content mode on and same speed setting for all
// layers.
TEST_P(DatarateOnePassCbrSvcSingleBR, OnePassCbrSvc2SL1TLScreenContent1) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 2;
  cfg_.ts_number_layers = 1;
  cfg_.ts_rate_decimator[0] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 1;
  cfg_.temporal_layering_mode = 0;
  svc_params_.scaling_factor_num[0] = 144;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 288;
  svc_params_.scaling_factor_den[1] = 288;
  cfg_.rc_dropframe_thresh = 10;
  cfg_.kf_max_dist = 9999;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ::libvpx_test::Y4mVideoSource video("niklas_1280_720_30.y4m", 0, 60);
  top_sl_width_ = 1280;
  top_sl_height_ = 720;
  cfg_.rc_target_bitrate = 500;
  ResetModel();
  tune_content_ = 1;
  base_speed_setting_ = speed_setting_;
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.78, 1.15);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

// Check basic rate targeting for 1 pass CBR SVC: 3 spatial layers and
// 3 temporal layers, with force key frame after frame drop
TEST_P(DatarateOnePassCbrSvcSingleBR, OnePassCbrSvc3SL3TLForceKey) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 3;
  cfg_.ts_number_layers = 3;
  cfg_.ts_rate_decimator[0] = 4;
  cfg_.ts_rate_decimator[1] = 2;
  cfg_.ts_rate_decimator[2] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 1;
  cfg_.temporal_layering_mode = 3;
  svc_params_.scaling_factor_num[0] = 72;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 144;
  svc_params_.scaling_factor_den[1] = 288;
  svc_params_.scaling_factor_num[2] = 288;
  svc_params_.scaling_factor_den[2] = 288;
  cfg_.rc_dropframe_thresh = 30;
  cfg_.kf_max_dist = 9999;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ::libvpx_test::I420VideoSource video("niklas_640_480_30.yuv", 640, 480, 30, 1,
                                       0, 400);
  top_sl_width_ = 640;
  top_sl_height_ = 480;
  cfg_.rc_target_bitrate = 100;
  ResetModel();
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.78, 1.25);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

// Check basic rate targeting for 1 pass CBR SVC: 3 spatial layers and
// 3 temporal layers. Run CIF clip with 1 thread.
TEST_P(DatarateOnePassCbrSvcSingleBR, OnePassCbrSvc3SL3TL) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 3;
  cfg_.ts_number_layers = 3;
  cfg_.ts_rate_decimator[0] = 4;
  cfg_.ts_rate_decimator[1] = 2;
  cfg_.ts_rate_decimator[2] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 1;
  cfg_.temporal_layering_mode = 3;
  svc_params_.scaling_factor_num[0] = 72;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 144;
  svc_params_.scaling_factor_den[1] = 288;
  svc_params_.scaling_factor_num[2] = 288;
  svc_params_.scaling_factor_den[2] = 288;
  cfg_.rc_dropframe_thresh = 30;
  cfg_.kf_max_dist = 9999;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ::libvpx_test::I420VideoSource video("niklas_640_480_30.yuv", 640, 480, 30, 1,
                                       0, 400);
  top_sl_width_ = 640;
  top_sl_height_ = 480;
  cfg_.rc_target_bitrate = 800;
  ResetModel();
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.78, 1.15);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

// Check rate targeting for 1 pass CBR SVC: 3 spatial layers and
// 3 temporal layers, changing the target bitrate at the middle of encoding.
TEST_P(DatarateOnePassCbrSvcSingleBR, OnePassCbrSvc3SL3TLDynamicBitrateChange) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 3;
  cfg_.ts_number_layers = 3;
  cfg_.ts_rate_decimator[0] = 4;
  cfg_.ts_rate_decimator[1] = 2;
  cfg_.ts_rate_decimator[2] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 1;
  cfg_.temporal_layering_mode = 3;
  svc_params_.scaling_factor_num[0] = 72;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 144;
  svc_params_.scaling_factor_den[1] = 288;
  svc_params_.scaling_factor_num[2] = 288;
  svc_params_.scaling_factor_den[2] = 288;
  cfg_.rc_dropframe_thresh = 30;
  cfg_.kf_max_dist = 9999;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ::libvpx_test::I420VideoSource video("niklas_640_480_30.yuv", 640, 480, 30, 1,
                                       0, 400);
  top_sl_width_ = 640;
  top_sl_height_ = 480;
  cfg_.rc_target_bitrate = 800;
  ResetModel();
  change_bitrate_ = true;
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.78, 1.15);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

// Check basic rate targeting for 1 pass CBR SVC: 3 spatial layers and
// 2 temporal layers, with a change on the fly from the fixed SVC pattern to one
// generate via SVC_SET_REF_FRAME_CONFIG. The new pattern also disables
// inter-layer prediction.
TEST_P(DatarateOnePassCbrSvcSingleBR, OnePassCbrSvc3SL2TLDynamicPatternChange) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 3;
  cfg_.ts_number_layers = 2;
  cfg_.ts_rate_decimator[0] = 2;
  cfg_.ts_rate_decimator[1] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 1;
  cfg_.temporal_layering_mode = 2;
  svc_params_.scaling_factor_num[0] = 72;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 144;
  svc_params_.scaling_factor_den[1] = 288;
  svc_params_.scaling_factor_num[2] = 288;
  svc_params_.scaling_factor_den[2] = 288;
  cfg_.rc_dropframe_thresh = 30;
  cfg_.kf_max_dist = 9999;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  // Change SVC pattern on the fly.
  update_pattern_ = 1;
  ::libvpx_test::I420VideoSource video("niklas_640_480_30.yuv", 640, 480, 30, 1,
                                       0, 400);
  top_sl_width_ = 640;
  top_sl_height_ = 480;
  cfg_.rc_target_bitrate = 800;
  ResetModel();
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.78, 1.15);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

// Check basic rate targeting for 1 pass CBR SVC with 3 spatial layers and on
// the fly switching to 1 and then 2 and back to 3 spatial layers. This switch
// is done by setting spatial layer bitrates to 0, and then back to non-zero,
// during the sequence.
TEST_P(DatarateOnePassCbrSvcSingleBR, OnePassCbrSvc3SL_DisableEnableLayers) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 3;
  cfg_.ts_number_layers = 1;
  cfg_.ts_rate_decimator[0] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 1;
  cfg_.temporal_layering_mode = 0;
  svc_params_.scaling_factor_num[0] = 72;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 144;
  svc_params_.scaling_factor_den[1] = 288;
  svc_params_.scaling_factor_num[2] = 288;
  svc_params_.scaling_factor_den[2] = 288;
  cfg_.rc_dropframe_thresh = 30;
  cfg_.kf_max_dist = 9999;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ::libvpx_test::I420VideoSource video("niklas_640_480_30.yuv", 640, 480, 30, 1,
                                       0, 400);
  top_sl_width_ = 640;
  top_sl_height_ = 480;
  cfg_.rc_target_bitrate = 800;
  ResetModel();
  dynamic_drop_layer_ = true;
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  // Don't check rate targeting on two top spatial layer since they will be
  // skipped for part of the sequence.
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_ - 2,
                          number_temporal_layers_, file_datarate_, 0.78, 1.15);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

// Params: speed setting and index for bitrate array.
class DatarateOnePassCbrSvcMultiBR
    : public DatarateOnePassCbrSvc,
      public ::libvpx_test::CodecTestWith2Params<int, int> {
 public:
  DatarateOnePassCbrSvcMultiBR() : DatarateOnePassCbrSvc(GET_PARAM(0)) {
    memset(&svc_params_, 0, sizeof(svc_params_));
  }
  virtual ~DatarateOnePassCbrSvcMultiBR() {}

 protected:
  virtual void SetUp() {
    InitializeConfig();
    SetMode(::libvpx_test::kRealTime);
    speed_setting_ = GET_PARAM(1);
    ResetModel();
  }
};

// Check basic rate targeting for 1 pass CBR SVC: 2 spatial layers and
// 3 temporal layers. Run CIF clip with 1 thread.
TEST_P(DatarateOnePassCbrSvcMultiBR, OnePassCbrSvc2SL3TL) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 2;
  cfg_.ts_number_layers = 3;
  cfg_.ts_rate_decimator[0] = 4;
  cfg_.ts_rate_decimator[1] = 2;
  cfg_.ts_rate_decimator[2] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 1;
  cfg_.temporal_layering_mode = 3;
  svc_params_.scaling_factor_num[0] = 144;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 288;
  svc_params_.scaling_factor_den[1] = 288;
  cfg_.rc_dropframe_thresh = 30;
  cfg_.kf_max_dist = 9999;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ::libvpx_test::I420VideoSource video("niklas_640_480_30.yuv", 640, 480, 30, 1,
                                       0, 400);
  top_sl_width_ = 640;
  top_sl_height_ = 480;
  const int bitrates[3] = { 200, 400, 600 };
  // TODO(marpan): Check that effective_datarate for each layer hits the
  // layer target_bitrate.
  cfg_.rc_target_bitrate = bitrates[GET_PARAM(2)];
  ResetModel();
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.75, 1.2);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

// Params: speed setting, layer framedrop control and index for bitrate array.
class DatarateOnePassCbrSvcFrameDropMultiBR
    : public DatarateOnePassCbrSvc,
      public ::libvpx_test::CodecTestWith3Params<int, int, int> {
 public:
  DatarateOnePassCbrSvcFrameDropMultiBR()
      : DatarateOnePassCbrSvc(GET_PARAM(0)) {
    memset(&svc_params_, 0, sizeof(svc_params_));
  }
  virtual ~DatarateOnePassCbrSvcFrameDropMultiBR() {}

 protected:
  virtual void SetUp() {
    InitializeConfig();
    SetMode(::libvpx_test::kRealTime);
    speed_setting_ = GET_PARAM(1);
    ResetModel();
  }
};

// Check basic rate targeting for 1 pass CBR SVC: 2 spatial layers and
// 3 temporal layers. Run HD clip with 4 threads.
TEST_P(DatarateOnePassCbrSvcFrameDropMultiBR, OnePassCbrSvc2SL3TL4Threads) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 2;
  cfg_.ts_number_layers = 3;
  cfg_.ts_rate_decimator[0] = 4;
  cfg_.ts_rate_decimator[1] = 2;
  cfg_.ts_rate_decimator[2] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 4;
  cfg_.temporal_layering_mode = 3;
  svc_params_.scaling_factor_num[0] = 144;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 288;
  svc_params_.scaling_factor_den[1] = 288;
  cfg_.rc_dropframe_thresh = 30;
  cfg_.kf_max_dist = 9999;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ::libvpx_test::Y4mVideoSource video("niklas_1280_720_30.y4m", 0, 60);
  top_sl_width_ = 1280;
  top_sl_height_ = 720;
  layer_framedrop_ = 0;
  const int bitrates[3] = { 200, 400, 600 };
  cfg_.rc_target_bitrate = bitrates[GET_PARAM(3)];
  ResetModel();
  layer_framedrop_ = GET_PARAM(2);
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.75, 1.45);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

// Check basic rate targeting for 1 pass CBR SVC: 3 spatial layers and
// 3 temporal layers. Run HD clip with 4 threads.
TEST_P(DatarateOnePassCbrSvcFrameDropMultiBR, OnePassCbrSvc3SL3TL4Threads) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 3;
  cfg_.ts_number_layers = 3;
  cfg_.ts_rate_decimator[0] = 4;
  cfg_.ts_rate_decimator[1] = 2;
  cfg_.ts_rate_decimator[2] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 4;
  cfg_.temporal_layering_mode = 3;
  svc_params_.scaling_factor_num[0] = 72;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 144;
  svc_params_.scaling_factor_den[1] = 288;
  svc_params_.scaling_factor_num[2] = 288;
  svc_params_.scaling_factor_den[2] = 288;
  cfg_.rc_dropframe_thresh = 30;
  cfg_.kf_max_dist = 9999;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ::libvpx_test::Y4mVideoSource video("niklas_1280_720_30.y4m", 0, 60);
  top_sl_width_ = 1280;
  top_sl_height_ = 720;
  layer_framedrop_ = 0;
  const int bitrates[3] = { 200, 400, 600 };
  cfg_.rc_target_bitrate = bitrates[GET_PARAM(3)];
  ResetModel();
  layer_framedrop_ = GET_PARAM(2);
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.73, 1.2);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

// Run SVC encoder for 1 temporal layer, 2 spatial layers, with spatial
// downscale 5x5.
TEST_P(DatarateOnePassCbrSvcSingleBR, OnePassCbrSvc2SL1TL5x5MultipleRuns) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 2;
  cfg_.ts_number_layers = 1;
  cfg_.ts_rate_decimator[0] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 3;
  cfg_.temporal_layering_mode = 0;
  svc_params_.scaling_factor_num[0] = 256;
  svc_params_.scaling_factor_den[0] = 1280;
  svc_params_.scaling_factor_num[1] = 1280;
  svc_params_.scaling_factor_den[1] = 1280;
  cfg_.rc_dropframe_thresh = 10;
  cfg_.kf_max_dist = 999999;
  cfg_.kf_min_dist = 0;
  cfg_.ss_target_bitrate[0] = 300;
  cfg_.ss_target_bitrate[1] = 1400;
  cfg_.layer_target_bitrate[0] = 300;
  cfg_.layer_target_bitrate[1] = 1400;
  cfg_.rc_target_bitrate = 1700;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ResetModel();
  layer_target_avg_bandwidth_[0] = cfg_.layer_target_bitrate[0] * 1000 / 30;
  bits_in_buffer_model_[0] =
      cfg_.layer_target_bitrate[0] * cfg_.rc_buf_initial_sz;
  layer_target_avg_bandwidth_[1] = cfg_.layer_target_bitrate[1] * 1000 / 30;
  bits_in_buffer_model_[1] =
      cfg_.layer_target_bitrate[1] * cfg_.rc_buf_initial_sz;
  ::libvpx_test::Y4mVideoSource video("niklas_1280_720_30.y4m", 0, 60);
  top_sl_width_ = 1280;
  top_sl_height_ = 720;
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.78, 1.15);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

#if CONFIG_VP9_TEMPORAL_DENOISING
// Params: speed setting, noise sensitivity and index for bitrate array.
class DatarateOnePassCbrSvcDenoiser
    : public DatarateOnePassCbrSvc,
      public ::libvpx_test::CodecTestWith3Params<int, int, int> {
 public:
  DatarateOnePassCbrSvcDenoiser() : DatarateOnePassCbrSvc(GET_PARAM(0)) {
    memset(&svc_params_, 0, sizeof(svc_params_));
  }
  virtual ~DatarateOnePassCbrSvcDenoiser() {}

 protected:
  virtual void SetUp() {
    InitializeConfig();
    SetMode(::libvpx_test::kRealTime);
    speed_setting_ = GET_PARAM(1);
    ResetModel();
  }
};

// Check basic rate targeting for 1 pass CBR SVC with denoising.
// 2 spatial layers and 3 temporal layer. Run HD clip with 2 threads.
TEST_P(DatarateOnePassCbrSvcDenoiser, OnePassCbrSvc2SL3TLDenoiserOn) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 2;
  cfg_.ts_number_layers = 3;
  cfg_.ts_rate_decimator[0] = 4;
  cfg_.ts_rate_decimator[1] = 2;
  cfg_.ts_rate_decimator[2] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 2;
  cfg_.temporal_layering_mode = 3;
  svc_params_.scaling_factor_num[0] = 144;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 288;
  svc_params_.scaling_factor_den[1] = 288;
  cfg_.rc_dropframe_thresh = 30;
  cfg_.kf_max_dist = 9999;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ::libvpx_test::I420VideoSource video("niklas_640_480_30.yuv", 640, 480, 30, 1,
                                       0, 400);
  top_sl_width_ = 640;
  top_sl_height_ = 480;
  const int bitrates[3] = { 600, 800, 1000 };
  // TODO(marpan): Check that effective_datarate for each layer hits the
  // layer target_bitrate.
  // For SVC, noise_sen = 1 means denoising only the top spatial layer
  // noise_sen = 2 means denoising the two top spatial layers.
  cfg_.rc_target_bitrate = bitrates[GET_PARAM(3)];
  ResetModel();
  denoiser_on_ = GET_PARAM(2);
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.78, 1.15);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}
#endif

// Params: speed setting, key frame dist.
class DatarateOnePassCbrSvcSmallKF
    : public DatarateOnePassCbrSvc,
      public ::libvpx_test::CodecTestWith2Params<int, int> {
 public:
  DatarateOnePassCbrSvcSmallKF() : DatarateOnePassCbrSvc(GET_PARAM(0)) {
    memset(&svc_params_, 0, sizeof(svc_params_));
  }
  virtual ~DatarateOnePassCbrSvcSmallKF() {}

 protected:
  virtual void SetUp() {
    InitializeConfig();
    SetMode(::libvpx_test::kRealTime);
    speed_setting_ = GET_PARAM(1);
    ResetModel();
  }
};

// Check basic rate targeting for 1 pass CBR SVC: 3 spatial layers and 3
// temporal layers. Run CIF clip with 1 thread, and few short key frame periods.
TEST_P(DatarateOnePassCbrSvcSmallKF, OnePassCbrSvc3SL3TLSmallKf) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 3;
  cfg_.ts_number_layers = 3;
  cfg_.ts_rate_decimator[0] = 4;
  cfg_.ts_rate_decimator[1] = 2;
  cfg_.ts_rate_decimator[2] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 1;
  cfg_.temporal_layering_mode = 3;
  svc_params_.scaling_factor_num[0] = 72;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 144;
  svc_params_.scaling_factor_den[1] = 288;
  svc_params_.scaling_factor_num[2] = 288;
  svc_params_.scaling_factor_den[2] = 288;
  cfg_.rc_dropframe_thresh = 10;
  cfg_.rc_target_bitrate = 800;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ::libvpx_test::I420VideoSource video("niklas_640_480_30.yuv", 640, 480, 30, 1,
                                       0, 400);
  top_sl_width_ = 640;
  top_sl_height_ = 480;
  // For this 3 temporal layer case, pattern repeats every 4 frames, so choose
  // 4 key neighboring key frame periods (so key frame will land on 0-2-1-2).
  const int kf_dist = GET_PARAM(2);
  cfg_.kf_max_dist = kf_dist;
  key_frame_spacing_ = kf_dist;
  ResetModel();
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.78, 1.15);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

// Check basic rate targeting for 1 pass CBR SVC: 2 spatial layers and 3
// temporal layers. Run CIF clip with 1 thread, and few short key frame periods.
TEST_P(DatarateOnePassCbrSvcSmallKF, OnePassCbrSvc2SL3TLSmallKf) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 500;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 0;
  cfg_.rc_max_quantizer = 63;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.g_lag_in_frames = 0;
  cfg_.ss_number_layers = 2;
  cfg_.ts_number_layers = 3;
  cfg_.ts_rate_decimator[0] = 4;
  cfg_.ts_rate_decimator[1] = 2;
  cfg_.ts_rate_decimator[2] = 1;
  cfg_.g_error_resilient = 1;
  cfg_.g_threads = 1;
  cfg_.temporal_layering_mode = 3;
  svc_params_.scaling_factor_num[0] = 144;
  svc_params_.scaling_factor_den[0] = 288;
  svc_params_.scaling_factor_num[1] = 288;
  svc_params_.scaling_factor_den[1] = 288;
  cfg_.rc_dropframe_thresh = 10;
  cfg_.rc_target_bitrate = 400;
  number_spatial_layers_ = cfg_.ss_number_layers;
  number_temporal_layers_ = cfg_.ts_number_layers;
  ::libvpx_test::I420VideoSource video("niklas_640_480_30.yuv", 640, 480, 30, 1,
                                       0, 400);
  top_sl_width_ = 640;
  top_sl_height_ = 480;
  // For this 3 temporal layer case, pattern repeats every 4 frames, so choose
  // 4 key neighboring key frame periods (so key frame will land on 0-2-1-2).
  const int kf_dist = GET_PARAM(2) + 32;
  cfg_.kf_max_dist = kf_dist;
  key_frame_spacing_ = kf_dist;
  ResetModel();
  AssignLayerBitrates(&cfg_, &svc_params_, cfg_.ss_number_layers,
                      cfg_.ts_number_layers, cfg_.temporal_layering_mode,
                      layer_target_avg_bandwidth_, bits_in_buffer_model_);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  CheckLayerRateTargeting(&cfg_, number_spatial_layers_,
                          number_temporal_layers_, file_datarate_, 0.78, 1.15);
#if CONFIG_VP9_DECODER
  // The non-reference frames are expected to be mismatched frames as the
  // encoder will avoid loopfilter on these frames.
  EXPECT_EQ(num_nonref_frames_, GetMismatchFrames());
#endif
}

VP9_INSTANTIATE_TEST_CASE(DatarateOnePassCbrSvcSingleBR,
                          ::testing::Range(5, 10));

VP9_INSTANTIATE_TEST_CASE(DatarateOnePassCbrSvcMultiBR, ::testing::Range(5, 10),
                          ::testing::Range(0, 3));

VP9_INSTANTIATE_TEST_CASE(DatarateOnePassCbrSvcFrameDropMultiBR,
                          ::testing::Range(5, 10), ::testing::Range(0, 2),
                          ::testing::Range(0, 3));

#if CONFIG_VP9_TEMPORAL_DENOISING
VP9_INSTANTIATE_TEST_CASE(DatarateOnePassCbrSvcDenoiser,
                          ::testing::Range(5, 10), ::testing::Range(1, 3),
                          ::testing::Range(0, 3));
#endif

VP9_INSTANTIATE_TEST_CASE(DatarateOnePassCbrSvcSmallKF, ::testing::Range(5, 10),
                          ::testing::Range(32, 36));
}  // namespace
