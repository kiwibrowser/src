/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "rtc_base/experiments/rate_control_settings.h"

#include <inttypes.h>
#include <stdio.h>

#include <string>

#include "api/transport/field_trial_based_config.h"
#include "rtc_base/logging.h"
#include "rtc_base/numerics/safe_conversions.h"

namespace webrtc {

namespace {

const char* kCongestionWindowFieldTrialName = "WebRTC-CwndExperiment";
const int kDefaultAcceptedQueueMs = 250;

const char* kCongestionWindowPushbackFieldTrialName =
    "WebRTC-CongestionWindowPushback";
const int kDefaultMinPushbackTargetBitrateBps = 30000;

const char kVp8TrustedRateControllerFieldTrialName[] =
    "WebRTC-LibvpxVp8TrustedRateController";
const char kVp9TrustedRateControllerFieldTrialName[] =
    "WebRTC-LibvpxVp9TrustedRateController";

const char* kVideoHysteresisFieldTrialname =
    "WebRTC-SimulcastUpswitchHysteresisPercent";
const double kDefaultVideoHysteresisFactor = 1.0;
const char* kScreenshareHysteresisFieldTrialname =
    "WebRTC-SimulcastScreenshareUpswitchHysteresisPercent";
// Default to 35% hysteresis for simulcast screenshare.
const double kDefaultScreenshareHysteresisFactor = 1.35;

absl::optional<int> MaybeReadCwndExperimentParameter(
    const WebRtcKeyValueConfig* const key_value_config) {
  int64_t accepted_queue_ms;
  std::string experiment_string =
      key_value_config->Lookup(kCongestionWindowFieldTrialName);
  int parsed_values =
      sscanf(experiment_string.c_str(), "Enabled-%" PRId64, &accepted_queue_ms);
  if (parsed_values == 1) {
    RTC_CHECK_GE(accepted_queue_ms, 0)
        << "Accepted must be greater than or equal to 0.";
    return rtc::checked_cast<int>(accepted_queue_ms);
  } else if (experiment_string.find("Enabled") == 0) {
    return kDefaultAcceptedQueueMs;
  }
  return absl::nullopt;
}

absl::optional<int> MaybeReadCongestionWindowPushbackExperimentParameter(
    const WebRtcKeyValueConfig* const key_value_config) {
  uint32_t min_pushback_target_bitrate_bps;
  std::string experiment_string =
      key_value_config->Lookup(kCongestionWindowPushbackFieldTrialName);
  int parsed_values = sscanf(experiment_string.c_str(), "Enabled-%" PRIu32,
                             &min_pushback_target_bitrate_bps);
  if (parsed_values == 1) {
    RTC_CHECK_GE(min_pushback_target_bitrate_bps, 0)
        << "Min pushback target bitrate must be greater than or equal to 0.";
    return rtc::checked_cast<int>(min_pushback_target_bitrate_bps);
  } else if (experiment_string.find("Enabled") == 0) {
    return kDefaultMinPushbackTargetBitrateBps;
  }
  return absl::nullopt;
}

bool IsEnabled(const WebRtcKeyValueConfig* const key_value_config,
               absl::string_view key) {
  return key_value_config->Lookup(key).find("Enabled") == 0;
}

double ParseHysteresisFactor(const WebRtcKeyValueConfig* const key_value_config,
                             absl::string_view key,
                             double default_value) {
  std::string group_name = key_value_config->Lookup(key);
  int percent = 0;
  if (!group_name.empty() && sscanf(group_name.c_str(), "%d", &percent) == 1 &&
      percent >= 0) {
    return 1.0 + (percent / 100.0);
  }
  return default_value;
}

}  // namespace

RateControlSettings::RateControlSettings(
    const WebRtcKeyValueConfig* const key_value_config)
    : congestion_window_("cwnd",
                         MaybeReadCwndExperimentParameter(key_value_config)),
      congestion_window_pushback_(
          "cwnd_pushback",
          MaybeReadCongestionWindowPushbackExperimentParameter(
              key_value_config)),
      pacing_factor_("pacing_factor"),
      alr_probing_("alr_probing", false),
      trust_vp8_(
          "trust_vp8",
          IsEnabled(key_value_config, kVp8TrustedRateControllerFieldTrialName)),
      trust_vp9_(
          "trust_vp9",
          IsEnabled(key_value_config, kVp9TrustedRateControllerFieldTrialName)),
      video_hysteresis_("video_hysteresis",
                        ParseHysteresisFactor(key_value_config,
                                              kVideoHysteresisFieldTrialname,
                                              kDefaultVideoHysteresisFactor)),
      screenshare_hysteresis_(
          "screenshare_hysteresis",
          ParseHysteresisFactor(key_value_config,
                                kScreenshareHysteresisFieldTrialname,
                                kDefaultScreenshareHysteresisFactor)),
      probe_max_allocation_("probe_max_allocation", true) {
  ParseFieldTrial(
      {&congestion_window_, &congestion_window_pushback_, &pacing_factor_,
       &alr_probing_, &trust_vp8_, &trust_vp9_, &video_hysteresis_,
       &screenshare_hysteresis_, &probe_max_allocation_},
      key_value_config->Lookup("WebRTC-VideoRateControl"));
}

RateControlSettings::~RateControlSettings() = default;
RateControlSettings::RateControlSettings(RateControlSettings&&) = default;

RateControlSettings RateControlSettings::ParseFromFieldTrials() {
  FieldTrialBasedConfig field_trial_config;
  return RateControlSettings(&field_trial_config);
}

RateControlSettings RateControlSettings::ParseFromKeyValueConfig(
    const WebRtcKeyValueConfig* const key_value_config) {
  FieldTrialBasedConfig field_trial_config;
  return RateControlSettings(key_value_config ? key_value_config
                                              : &field_trial_config);
}

bool RateControlSettings::UseCongestionWindow() const {
  return congestion_window_;
}

int64_t RateControlSettings::GetCongestionWindowAdditionalTimeMs() const {
  return congestion_window_.GetOptional().value_or(kDefaultAcceptedQueueMs);
}

bool RateControlSettings::UseCongestionWindowPushback() const {
  return congestion_window_ && congestion_window_pushback_;
}

uint32_t RateControlSettings::CongestionWindowMinPushbackTargetBitrateBps()
    const {
  return congestion_window_pushback_.GetOptional().value_or(
      kDefaultMinPushbackTargetBitrateBps);
}

absl::optional<double> RateControlSettings::GetPacingFactor() const {
  return pacing_factor_.GetOptional();
}

bool RateControlSettings::UseAlrProbing() const {
  return alr_probing_.Get();
}

bool RateControlSettings::LibvpxVp8TrustedRateController() const {
  return trust_vp8_.Get();
}

bool RateControlSettings::LibvpxVp9TrustedRateController() const {
  return trust_vp9_.Get();
}

double RateControlSettings::GetSimulcastHysteresisFactor(
    VideoCodecMode mode) const {
  if (mode == VideoCodecMode::kScreensharing) {
    return GetSimulcastScreenshareHysteresisFactor();
  }
  return GetSimulcastVideoHysteresisFactor();
}

double RateControlSettings::GetSimulcastHysteresisFactor(
    VideoEncoderConfig::ContentType content_type) const {
  if (content_type == VideoEncoderConfig::ContentType::kScreen) {
    return GetSimulcastScreenshareHysteresisFactor();
  }
  return GetSimulcastVideoHysteresisFactor();
}

double RateControlSettings::GetSimulcastVideoHysteresisFactor() const {
  return video_hysteresis_.Get();
}

double RateControlSettings::GetSimulcastScreenshareHysteresisFactor() const {
  return screenshare_hysteresis_.Get();
}

bool RateControlSettings::TriggerProbeOnMaxAllocatedBitrateChange() const {
  return probe_max_allocation_.Get();
}

}  // namespace webrtc
