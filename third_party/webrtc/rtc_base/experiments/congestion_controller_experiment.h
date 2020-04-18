/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef RTC_BASE_EXPERIMENTS_CONGESTION_CONTROLLER_EXPERIMENT_H_
#define RTC_BASE_EXPERIMENTS_CONGESTION_CONTROLLER_EXPERIMENT_H_
#include <api/optional.h>
namespace webrtc {
class CongestionControllerExperiment {
 public:
  struct BbrExperimentConfig {
    int exit_startup_on_loss;
    int exit_startup_rtt_threshold_ms;
    int fully_drain_queue;
    int initial_conservation_in_startup;
    int num_startup_rtts;
    int probe_rtt_based_on_bdp;
    int probe_rtt_disabled_if_app_limited;
    int probe_rtt_skipped_if_similar_rtt;
    int rate_based_recovery;
    int rate_based_startup;
    int slower_startup;
    double encoder_rate_gain;
    double encoder_rate_gain_in_probe_rtt;
    double max_ack_height_window_multiplier;
    double max_aggregation_bytes_multiplier;
    double probe_bw_pacing_gain_offset;
    double probe_rtt_congestion_window_gain;
  };
  static bool BbrControllerEnabled();
  static bool InjectedControllerEnabled();
  static rtc::Optional<BbrExperimentConfig> GetBbrExperimentConfig();
};

}  // namespace webrtc

#endif  // RTC_BASE_EXPERIMENTS_CONGESTION_CONTROLLER_EXPERIMENT_H_
