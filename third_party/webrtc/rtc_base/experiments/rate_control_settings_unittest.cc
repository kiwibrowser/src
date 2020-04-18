/*
 *  Copyright 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "rtc_base/experiments/rate_control_settings.h"

#include "api/video_codecs/video_codec.h"
#include "api/video_codecs/video_encoder_config.h"
#include "test/field_trial.h"
#include "test/gtest.h"

namespace webrtc {

namespace {

TEST(RateControlSettingsTest, LibvpxTrustedRateController) {
  test::ScopedFieldTrials field_trials(
      "WebRTC-VideoRateControl/trust_vp8:1,trust_vp9:0/");
  const RateControlSettings rate_control_settings =
      RateControlSettings::ParseFromFieldTrials();

  EXPECT_TRUE(rate_control_settings.LibvpxVp8TrustedRateController());
  EXPECT_FALSE(rate_control_settings.LibvpxVp9TrustedRateController());
}

TEST(RateControlSettingsTest, GetSimulcastHysteresisFactor) {
  test::ScopedFieldTrials field_trials(
      "WebRTC-VideoRateControl/"
      "video_hysteresis:1.2,screenshare_hysteresis:1.4/");
  const RateControlSettings rate_control_settings =
      RateControlSettings::ParseFromFieldTrials();

  EXPECT_EQ(rate_control_settings.GetSimulcastHysteresisFactor(
                VideoCodecMode::kRealtimeVideo),
            1.2);
  EXPECT_EQ(rate_control_settings.GetSimulcastHysteresisFactor(
                VideoEncoderConfig::ContentType::kRealtimeVideo),
            1.2);
  EXPECT_EQ(rate_control_settings.GetSimulcastHysteresisFactor(
                VideoCodecMode::kScreensharing),
            1.4);
  EXPECT_EQ(rate_control_settings.GetSimulcastHysteresisFactor(
                VideoEncoderConfig::ContentType::kScreen),
            1.4);
}

}  // namespace

}  // namespace webrtc
