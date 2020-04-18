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
#include "rtc_base/gunit.h"
#include "test/field_trial.h"

namespace webrtc {
namespace {
void ExpectEquals(CongestionControllerExperiment::BbrExperimentConfig a,
                  CongestionControllerExperiment::BbrExperimentConfig b) {
  EXPECT_EQ(a.exit_startup_on_loss, b.exit_startup_on_loss);
  EXPECT_EQ(a.exit_startup_rtt_threshold_ms, b.exit_startup_rtt_threshold_ms);
  EXPECT_EQ(a.fully_drain_queue, b.fully_drain_queue);
  EXPECT_EQ(a.initial_conservation_in_startup,
            b.initial_conservation_in_startup);
  EXPECT_EQ(a.num_startup_rtts, b.num_startup_rtts);
  EXPECT_EQ(a.probe_rtt_based_on_bdp, b.probe_rtt_based_on_bdp);
  EXPECT_EQ(a.probe_rtt_disabled_if_app_limited,
            b.probe_rtt_disabled_if_app_limited);
  EXPECT_EQ(a.probe_rtt_skipped_if_similar_rtt,
            b.probe_rtt_skipped_if_similar_rtt);
  EXPECT_EQ(a.rate_based_recovery, b.rate_based_recovery);
  EXPECT_EQ(a.rate_based_startup, b.rate_based_startup);
  EXPECT_EQ(a.slower_startup, b.slower_startup);
  EXPECT_EQ(a.encoder_rate_gain, b.encoder_rate_gain);
  EXPECT_EQ(a.encoder_rate_gain_in_probe_rtt, b.encoder_rate_gain_in_probe_rtt);
  EXPECT_EQ(a.max_ack_height_window_multiplier,
            b.max_ack_height_window_multiplier);
  EXPECT_EQ(a.max_aggregation_bytes_multiplier,
            b.max_aggregation_bytes_multiplier);
  EXPECT_EQ(a.probe_bw_pacing_gain_offset, b.probe_bw_pacing_gain_offset);
  EXPECT_EQ(a.probe_rtt_congestion_window_gain,
            b.probe_rtt_congestion_window_gain);
}
}  // namespace

TEST(CongestionControllerExperimentTest, BbrDisabledByDefault) {
  webrtc::test::ScopedFieldTrials field_trials("");
  EXPECT_FALSE(CongestionControllerExperiment::BbrControllerEnabled());
}

TEST(CongestionControllerExperimentTest, BbrEnabledByFieldTrial) {
  webrtc::test::ScopedFieldTrials field_trials(
      "WebRTC-BweCongestionController/Enabled,BBR/");
  EXPECT_TRUE(CongestionControllerExperiment::BbrControllerEnabled());
}

TEST(CongestionControllerExperimentTest, BbrBadParametersFails) {
  webrtc::test::ScopedFieldTrials field_trials(
      "WebRTC-BweCongestionController/Enabled,BBR,"
      "garbage,here/");
  EXPECT_FALSE(CongestionControllerExperiment::GetBbrExperimentConfig());
}

TEST(CongestionControllerExperimentTest, BbrZeroParametersParsed) {
  CongestionControllerExperiment::BbrExperimentConfig truth = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  webrtc::test::ScopedFieldTrials field_trials(
      "WebRTC-BweCongestionController/Enabled,BBR,"
      "0,0,0,0,0,0,0,0,0,0,0,"
      "0,0,0,0,0,0/");
  auto config = CongestionControllerExperiment::GetBbrExperimentConfig();
  EXPECT_TRUE(config);
  if (config)
    ExpectEquals(truth, *config);
}

TEST(CongestionControllerExperimentTest, BbrNonZeroParametersParsed) {
  CongestionControllerExperiment::BbrExperimentConfig truth = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25};

  webrtc::test::ScopedFieldTrials field_trials(
      "WebRTC-BweCongestionController/Enabled,BBR,"
      "1,1,1,1,1,1,1,1,1,1,1,"
      "0.25,0.25,0.25,0.25,0.25,0.25/");
  auto config = CongestionControllerExperiment::GetBbrExperimentConfig();
  EXPECT_TRUE(config);
  if (config)
    ExpectEquals(truth, *config);
}
}  // namespace webrtc
