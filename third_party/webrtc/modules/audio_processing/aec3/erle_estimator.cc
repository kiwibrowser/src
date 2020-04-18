/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/audio_processing/aec3/erle_estimator.h"

#include <algorithm>
#include <numeric>

#include "rtc_base/numerics/safe_minmax.h"

namespace webrtc {

ErleEstimator::ErleEstimator(float min_erle,
                             float max_erle_lf,
                             float max_erle_hf)
    : min_erle_(min_erle),
      max_erle_lf_(max_erle_lf),
      max_erle_hf_(max_erle_hf) {
  erle_.fill(min_erle_);
  erle_onsets_.fill(min_erle_);
  Y2_acum_.fill(0.f);
  E2_acum_.fill(0.f);
  num_points_.fill(0);
  hold_counters_.fill(0);
  coming_onset_.fill(true);
  erle_time_domain_ = min_erle_;
  hold_counter_time_domain_ = 0;
}

ErleEstimator::~ErleEstimator() = default;

void ErleEstimator::Update(rtc::ArrayView<const float> render_spectrum,
                           rtc::ArrayView<const float> capture_spectrum,
                           rtc::ArrayView<const float> subtractor_spectrum,
                           bool converged_filter) {
  RTC_DCHECK_EQ(kFftLengthBy2Plus1, render_spectrum.size());
  RTC_DCHECK_EQ(kFftLengthBy2Plus1, capture_spectrum.size());
  RTC_DCHECK_EQ(kFftLengthBy2Plus1, subtractor_spectrum.size());
  const auto& X2 = render_spectrum;
  const auto& Y2 = capture_spectrum;
  const auto& E2 = subtractor_spectrum;

  // Corresponds of WGN of power -46 dBFS.
  constexpr float kX2Min = 44015068.0f;
  constexpr int kPointsToAccumulate = 6;
  constexpr int kErleHold = 100;
  constexpr int kBlocksForOnsetDetection = kErleHold + 150;

  auto erle_band_update = [](float erle_band, float new_erle, float alpha_inc,
                             float alpha_dec, float min_erle, float max_erle) {
    float alpha = new_erle > erle_band ? alpha_inc : alpha_dec;
    float erle_band_out = erle_band;
    erle_band_out = erle_band + alpha * (new_erle - erle_band);
    erle_band_out = rtc::SafeClamp(erle_band_out, min_erle, max_erle);
    return erle_band_out;
  };

  // Update the estimates in a clamped minimum statistics manner.
  auto erle_update = [&](size_t start, size_t stop, float max_erle) {
    for (size_t k = start; k < stop; ++k) {
      if (X2[k] > kX2Min) {
        ++num_points_[k];
        Y2_acum_[k] += Y2[k];
        E2_acum_[k] += E2[k];
        if (num_points_[k] == kPointsToAccumulate) {
          if (E2_acum_[k] > 0) {
            const float new_erle = Y2_acum_[k] / E2_acum_[k];
            if (coming_onset_[k]) {
              coming_onset_[k] = false;
              erle_onsets_[k] = erle_band_update(
                  erle_onsets_[k], new_erle, 0.15f, 0.3f, min_erle_, max_erle);
            }
            hold_counters_[k] = kBlocksForOnsetDetection;
            erle_[k] = erle_band_update(erle_[k], new_erle, 0.05f, 0.1f,
                                        min_erle_, max_erle);
          }
          num_points_[k] = 0;
          Y2_acum_[k] = 0.f;
          E2_acum_[k] = 0.f;
        }
      }
    }
  };

  if (converged_filter) {
    // Note that the use of the converged_filter flag already imposed
    // a minimum of the erle that can be estimated as that flag would
    // be false if the filter is performing poorly.
    constexpr size_t kFftLengthBy4 = kFftLengthBy2 / 2;
    erle_update(1, kFftLengthBy4, max_erle_lf_);
    erle_update(kFftLengthBy4, kFftLengthBy2, max_erle_hf_);
  }

  for (size_t k = 1; k < kFftLengthBy2; ++k) {
    hold_counters_[k]--;
    if (hold_counters_[k] <= (kBlocksForOnsetDetection - kErleHold)) {
      if (erle_[k] > erle_onsets_[k]) {
        erle_[k] = std::max(erle_onsets_[k], 0.97f * erle_[k]);
        RTC_DCHECK_LE(min_erle_, erle_[k]);
      }
      if (hold_counters_[k] <= 0) {
        coming_onset_[k] = true;
        hold_counters_[k] = 0;
      }
    }
  }

  erle_[0] = erle_[1];
  erle_[kFftLengthBy2] = erle_[kFftLengthBy2 - 1];

  if (converged_filter) {
    // Compute ERLE over all frequency bins.
    const float X2_sum = std::accumulate(X2.begin(), X2.end(), 0.0f);
    const float E2_sum = std::accumulate(E2.begin(), E2.end(), 0.0f);
    if (X2_sum > kX2Min * X2.size() && E2_sum > 0.f) {
      const float Y2_sum = std::accumulate(Y2.begin(), Y2.end(), 0.0f);
      const float new_erle = Y2_sum / E2_sum;
      if (new_erle > erle_time_domain_) {
        hold_counter_time_domain_ = kErleHold;
        erle_time_domain_ += 0.1f * (new_erle - erle_time_domain_);
        erle_time_domain_ =
            rtc::SafeClamp(erle_time_domain_, min_erle_, max_erle_lf_);
      }
    }
  }
  --hold_counter_time_domain_;
  erle_time_domain_ = (hold_counter_time_domain_ > 0)
                        ? erle_time_domain_
                        : std::max(min_erle_, 0.97f * erle_time_domain_);
}

}  // namespace webrtc
