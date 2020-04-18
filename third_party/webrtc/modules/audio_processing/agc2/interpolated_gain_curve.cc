/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/audio_processing/agc2/interpolated_gain_curve.h"

#include "modules/audio_processing/agc2/agc2_common.h"
#include "modules/audio_processing/logging/apm_data_dumper.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "system_wrappers/include/metrics.h"

namespace webrtc {
namespace {
void LogRegionStats(const InterpolatedGainCurve::Stats& stats) {
  using Region = InterpolatedGainCurve::GainCurveRegion;
  const int duration_s =
      stats.region_duration_frames / (1000 / kFrameDurationMs);

  switch (stats.region) {
    case Region::kIdentity: {
      RTC_HISTOGRAM_COUNTS_10000(
          "WebRTC.Audio.Agc2.FixedDigitalGainCurveRegion.Identity", duration_s);
      break;
    }
    case Region::kKnee: {
      RTC_HISTOGRAM_COUNTS_10000(
          "WebRTC.Audio.Agc2.FixedDigitalGainCurveRegion.Knee", duration_s);
      break;
    }
    case Region::kLimiter: {
      RTC_HISTOGRAM_COUNTS_10000(
          "WebRTC.Audio.Agc2.FixedDigitalGainCurveRegion.Limiter", duration_s);
      break;
    }
    case Region::kSaturation: {
      RTC_HISTOGRAM_COUNTS_10000(
          "WebRTC.Audio.Agc2.FixedDigitalGainCurveRegion.Saturation",
          duration_s);
      break;
    }
    default: { RTC_NOTREACHED(); }
  }
}
}  // namespace

constexpr std::array<float, kInterpolatedGainCurveTotalPoints>
    InterpolatedGainCurve::approximation_params_x_;

constexpr std::array<float, kInterpolatedGainCurveTotalPoints>
    InterpolatedGainCurve::approximation_params_m_;

constexpr std::array<float, kInterpolatedGainCurveTotalPoints>
    InterpolatedGainCurve::approximation_params_q_;

InterpolatedGainCurve::InterpolatedGainCurve(ApmDataDumper* apm_data_dumper)
    : apm_data_dumper_(apm_data_dumper) {}

InterpolatedGainCurve::~InterpolatedGainCurve() {
  if (stats_.available) {
    RTC_DCHECK(apm_data_dumper_);
    apm_data_dumper_->DumpRaw("agc2_interp_gain_curve_lookups_identity",
                              stats_.look_ups_identity_region);
    apm_data_dumper_->DumpRaw("agc2_interp_gain_curve_lookups_knee",
                              stats_.look_ups_knee_region);
    apm_data_dumper_->DumpRaw("agc2_interp_gain_curve_lookups_limiter",
                              stats_.look_ups_limiter_region);
    apm_data_dumper_->DumpRaw("agc2_interp_gain_curve_lookups_saturation",
                              stats_.look_ups_saturation_region);
    LogRegionStats(stats_);
  }
}

void InterpolatedGainCurve::UpdateStats(float input_level) const {
  stats_.available = true;

  GainCurveRegion region;

  if (input_level < approximation_params_x_[0]) {
    stats_.look_ups_identity_region++;
    region = GainCurveRegion::kIdentity;
  } else if (input_level <
             approximation_params_x_[kInterpolatedGainCurveKneePoints - 1]) {
    stats_.look_ups_knee_region++;
    region = GainCurveRegion::kKnee;
  } else if (input_level < kMaxInputLevelLinear) {
    stats_.look_ups_limiter_region++;
    region = GainCurveRegion::kLimiter;
  } else {
    stats_.look_ups_saturation_region++;
    region = GainCurveRegion::kSaturation;
  }

  if (region == stats_.region) {
    ++stats_.region_duration_frames;
  } else {
    LogRegionStats(stats_);

    stats_.region_duration_frames = 0;
    stats_.region = region;
  }
}

// Looks up a gain to apply given a non-negative input level.
// The cost of this operation depends on the region in which |input_level|
// falls.
// For the identity and the saturation regions the cost is O(1).
// For the other regions, namely knee and limiter, the cost is
// O(2 + log2(|LightkInterpolatedGainCurveTotalPoints|), plus O(1) for the
// linear interpolation (one product and one sum).
float InterpolatedGainCurve::LookUpGainToApply(float input_level) const {
  UpdateStats(input_level);

  if (input_level <= approximation_params_x_[0]) {
    // Identity region.
    return 1.0f;
  }

  if (input_level >= kMaxInputLevelLinear) {
    // Saturating lower bound. The saturing samples exactly hit the clipping
    // level. This method achieves has the lowest harmonic distorsion, but it
    // may reduce the amplitude of the non-saturating samples too much.
    return 32768.f / input_level;
  }

  // Knee and limiter regions; find the linear piece index. Spelling
  // out the complete type was the only way to silence both the clang
  // plugin and the windows compilers.
  std::array<float, kInterpolatedGainCurveTotalPoints>::const_iterator it =
      std::lower_bound(approximation_params_x_.begin(),
                       approximation_params_x_.end(), input_level);
  const size_t index = std::distance(approximation_params_x_.begin(), it) - 1;
  RTC_DCHECK_LE(0, index);
  RTC_DCHECK_LT(index, approximation_params_m_.size());
  RTC_DCHECK_LE(approximation_params_x_[index], input_level);
  if (index < approximation_params_m_.size() - 1) {
    RTC_DCHECK_LE(input_level, approximation_params_x_[index + 1]);
  }

  // Piece-wise linear interploation.
  const float gain = approximation_params_m_[index] * input_level +
                     approximation_params_q_[index];
  RTC_DCHECK_LE(0.f, gain);
  return gain;
}

}  // namespace webrtc
