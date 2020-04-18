/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/congestion_controller/rtp/control_handler.h"

#include <algorithm>
#include <vector>

#include "api/units/data_rate.h"
#include "rtc_base/logging.h"
#include "rtc_base/numerics/safe_conversions.h"
#include "rtc_base/numerics/safe_minmax.h"
#include "system_wrappers/include/field_trial.h"

namespace webrtc {
namespace {

// When PacerPushbackExperiment is enabled, build-up in the pacer due to
// the congestion window and/or data spikes reduces encoder allocations.
bool IsPacerPushbackExperimentEnabled() {
  return field_trial::IsEnabled("WebRTC-PacerPushbackExperiment");
}

// By default, pacer emergency stops encoder when buffer reaches a high level.
bool IsPacerEmergencyStopDisabled() {
  return field_trial::IsEnabled("WebRTC-DisablePacerEmergencyStop");
}

}  // namespace
CongestionControlHandler::CongestionControlHandler()
    : pacer_pushback_experiment_(IsPacerPushbackExperimentEnabled()),
      disable_pacer_emergency_stop_(IsPacerEmergencyStopDisabled()) {
  sequenced_checker_.Detach();
}

CongestionControlHandler::~CongestionControlHandler() {}

void CongestionControlHandler::SetTargetRate(
    TargetTransferRate new_target_rate) {
  RTC_DCHECK_CALLED_SEQUENTIALLY(&sequenced_checker_);
  last_incoming_ = new_target_rate;
}

void CongestionControlHandler::SetNetworkAvailability(bool network_available) {
  RTC_DCHECK_CALLED_SEQUENTIALLY(&sequenced_checker_);
  network_available_ = network_available;
}

void CongestionControlHandler::SetPacerQueue(TimeDelta expected_queue_time) {
  RTC_DCHECK_CALLED_SEQUENTIALLY(&sequenced_checker_);
  pacer_expected_queue_ms_ = expected_queue_time.ms();
}

absl::optional<TargetTransferRate> CongestionControlHandler::GetUpdate() {
  RTC_DCHECK_CALLED_SEQUENTIALLY(&sequenced_checker_);
  if (!last_incoming_.has_value())
    return absl::nullopt;
  TargetTransferRate new_outgoing = *last_incoming_;
  DataRate log_target_rate = new_outgoing.target_rate;
  bool pause_encoding = false;
  if (!network_available_) {
    pause_encoding = true;
  } else if (pacer_pushback_experiment_) {
    const int64_t queue_length_ms = pacer_expected_queue_ms_;
    if (queue_length_ms == 0) {
      encoding_rate_ratio_ = 1.0;
    } else if (queue_length_ms > 50) {
      double encoding_ratio = 1.0 - queue_length_ms / 1000.0;
      encoding_rate_ratio_ = std::min(encoding_rate_ratio_, encoding_ratio);
      encoding_rate_ratio_ = std::max(encoding_rate_ratio_, 0.0);
    }
    new_outgoing.target_rate = new_outgoing.target_rate * encoding_rate_ratio_;
    log_target_rate = new_outgoing.target_rate;
    if (new_outgoing.target_rate < DataRate::kbps(50))
      pause_encoding = true;
  } else if (!disable_pacer_emergency_stop_ &&
             pacer_expected_queue_ms_ > PacedSender::kMaxQueueLengthMs) {
    pause_encoding = true;
  }
  if (pause_encoding)
    new_outgoing.target_rate = DataRate::Zero();
  if (!last_reported_ ||
      last_reported_->target_rate != new_outgoing.target_rate ||
      (!new_outgoing.target_rate.IsZero() &&
       (last_reported_->network_estimate.loss_rate_ratio !=
            new_outgoing.network_estimate.loss_rate_ratio ||
        last_reported_->network_estimate.round_trip_time !=
            new_outgoing.network_estimate.round_trip_time))) {
    if (encoder_paused_in_last_report_ != pause_encoding)
      RTC_LOG(LS_INFO) << "Bitrate estimate state changed, BWE: "
                       << ToString(log_target_rate) << ".";
    encoder_paused_in_last_report_ = pause_encoding;
    last_reported_ = new_outgoing;
    return new_outgoing;
  }
  return absl::nullopt;
}

}  // namespace webrtc
