/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "rtc_base/experiments/congestion_controller_experiment.h"

#include <string>

#include "system_wrappers/include/field_trial.h"

namespace webrtc {
namespace {

const char kControllerExperiment[] = "WebRTC-BweCongestionController";
}  // namespace

bool CongestionControllerExperiment::BbrControllerEnabled() {
  std::string trial_string =
      webrtc::field_trial::FindFullName(kControllerExperiment);
  return trial_string.find("Enabled,BBR") == 0;
}

bool CongestionControllerExperiment::InjectedControllerEnabled() {
  std::string trial_string =
      webrtc::field_trial::FindFullName(kControllerExperiment);
  return trial_string.find("Enabled,Injected") == 0;
}

rtc::Optional<CongestionControllerExperiment::BbrExperimentConfig>
CongestionControllerExperiment::GetBbrExperimentConfig() {
  if (!BbrControllerEnabled())
    return rtc::nullopt;
  std::string trial_string =
      webrtc::field_trial::FindFullName(kControllerExperiment);
  BbrExperimentConfig config;
  if (sscanf(
          trial_string.c_str(),
          "Enabled,BBR,"
          "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
          "%lf,%lf,%lf,%lf,%lf,%lf",
          &config.exit_startup_on_loss, &config.exit_startup_rtt_threshold_ms,
          &config.fully_drain_queue, &config.initial_conservation_in_startup,
          &config.num_startup_rtts, &config.probe_rtt_based_on_bdp,
          &config.probe_rtt_disabled_if_app_limited,
          &config.probe_rtt_skipped_if_similar_rtt, &config.rate_based_recovery,
          &config.rate_based_startup, &config.slower_startup,
          &config.encoder_rate_gain, &config.encoder_rate_gain_in_probe_rtt,
          &config.max_ack_height_window_multiplier,
          &config.max_aggregation_bytes_multiplier,
          &config.probe_bw_pacing_gain_offset,
          &config.probe_rtt_congestion_window_gain) == 17) {
    return config;
  } else {
    return rtc::nullopt;
  }
}

}  // namespace webrtc
