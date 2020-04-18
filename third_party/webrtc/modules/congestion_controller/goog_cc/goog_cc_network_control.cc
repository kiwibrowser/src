/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/congestion_controller/goog_cc/goog_cc_network_control.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "modules/congestion_controller/goog_cc/acknowledged_bitrate_estimator.h"
#include "modules/congestion_controller/goog_cc/alr_detector.h"
#include "modules/congestion_controller/goog_cc/include/goog_cc_factory.h"
#include "modules/congestion_controller/goog_cc/probe_controller.h"
#include "modules/remote_bitrate_estimator/include/bwe_defines.h"
#include "modules/remote_bitrate_estimator/test/bwe_test_logging.h"
#include "rtc_base/checks.h"
#include "rtc_base/format_macros.h"
#include "rtc_base/logging.h"
#include "rtc_base/ptr_util.h"
#include "rtc_base/timeutils.h"
#include "system_wrappers/include/field_trial.h"

namespace webrtc {
namespace webrtc_cc {
namespace {

const char kCwndExperiment[] = "WebRTC-CwndExperiment";
const int64_t kDefaultAcceptedQueueMs = 250;

// Pacing-rate relative to our target send rate.
// Multiplicative factor that is applied to the target bitrate to calculate
// the number of bytes that can be transmitted per interval.
// Increasing this factor will result in lower delays in cases of bitrate
// overshoots from the encoder.
const float kDefaultPaceMultiplier = 2.5f;

bool CwndExperimentEnabled() {
  std::string experiment_string =
      webrtc::field_trial::FindFullName(kCwndExperiment);
  // The experiment is enabled iff the field trial string begins with "Enabled".
  return experiment_string.find("Enabled") == 0;
}

bool ReadCwndExperimentParameter(int64_t* accepted_queue_ms) {
  RTC_DCHECK(accepted_queue_ms);
  std::string experiment_string =
      webrtc::field_trial::FindFullName(kCwndExperiment);
  int parsed_values =
      sscanf(experiment_string.c_str(), "Enabled-%" PRId64, accepted_queue_ms);
  if (parsed_values == 1) {
    RTC_CHECK_GE(*accepted_queue_ms, 0)
        << "Accepted must be greater than or equal to 0.";
    return true;
  }
  return false;
}

// Makes sure that the bitrate and the min, max values are in valid range.
static void ClampBitrates(int64_t* bitrate_bps,
                          int64_t* min_bitrate_bps,
                          int64_t* max_bitrate_bps) {
  // TODO(holmer): We should make sure the default bitrates are set to 10 kbps,
  // and that we don't try to set the min bitrate to 0 from any applications.
  // The congestion controller should allow a min bitrate of 0.
  if (*min_bitrate_bps < congestion_controller::GetMinBitrateBps())
    *min_bitrate_bps = congestion_controller::GetMinBitrateBps();
  if (*max_bitrate_bps > 0)
    *max_bitrate_bps = std::max(*min_bitrate_bps, *max_bitrate_bps);
  if (*bitrate_bps > 0)
    *bitrate_bps = std::max(*min_bitrate_bps, *bitrate_bps);
}

std::vector<PacketFeedback> ReceivedPacketsFeedbackAsRtp(
    const TransportPacketsFeedback report) {
  std::vector<PacketFeedback> packet_feedback_vector;
  for (auto& fb : report.PacketsWithFeedback()) {
    if (fb.receive_time.IsFinite()) {
      PacketFeedback pf(fb.receive_time.ms(), 0);
      pf.creation_time_ms = report.feedback_time.ms();
      if (fb.sent_packet.has_value()) {
        pf.payload_size = fb.sent_packet->size.bytes();
        pf.pacing_info = fb.sent_packet->pacing_info;
        pf.send_time_ms = fb.sent_packet->send_time.ms();
      } else {
        pf.send_time_ms = PacketFeedback::kNoSendTime;
      }
      packet_feedback_vector.push_back(pf);
    }
  }
  return packet_feedback_vector;
}

int64_t GetBpsOrDefault(const rtc::Optional<DataRate>& rate,
                        int64_t fallback_bps) {
  if (rate && rate->IsFinite()) {
    return rate->bps();
  } else {
    return fallback_bps;
  }
}

}  // namespace

GoogCcNetworkController::GoogCcNetworkController(RtcEventLog* event_log,
                                                 NetworkControllerConfig config)
    : event_log_(event_log),
      probe_controller_(new ProbeController()),
      bandwidth_estimation_(
          rtc::MakeUnique<SendSideBandwidthEstimation>(event_log_)),
      alr_detector_(rtc::MakeUnique<AlrDetector>()),
      delay_based_bwe_(new DelayBasedBwe(event_log_)),
      acknowledged_bitrate_estimator_(
          rtc::MakeUnique<AcknowledgedBitrateEstimator>()),
      last_bandwidth_(config.starting_bandwidth),
      pacing_factor_(kDefaultPaceMultiplier),
      min_pacing_rate_(DataRate::Zero()),
      max_padding_rate_(DataRate::Zero()),
      max_total_allocated_bitrate_(DataRate::Zero()),
      in_cwnd_experiment_(CwndExperimentEnabled()),
      accepted_queue_ms_(kDefaultAcceptedQueueMs) {
  delay_based_bwe_->SetMinBitrate(congestion_controller::GetMinBitrateBps());
  UpdateBitrateConstraints(config.constraints, config.starting_bandwidth);
  OnStreamsConfig(config.stream_based_config);
  if (in_cwnd_experiment_ &&
      !ReadCwndExperimentParameter(&accepted_queue_ms_)) {
    RTC_LOG(LS_WARNING) << "Failed to parse parameters for CwndExperiment "
                           "from field trial string. Experiment disabled.";
    in_cwnd_experiment_ = false;
  }
}

GoogCcNetworkController::~GoogCcNetworkController() {}

NetworkControlUpdate GoogCcNetworkController::OnNetworkAvailability(
    NetworkAvailability msg) {
  probe_controller_->OnNetworkAvailability(msg);
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnNetworkRouteChange(
    NetworkRouteChange msg) {
  int64_t min_bitrate_bps = GetBpsOrDefault(msg.constraints.min_data_rate, -1);
  int64_t max_bitrate_bps = GetBpsOrDefault(msg.constraints.max_data_rate, -1);
  int64_t start_bitrate_bps = GetBpsOrDefault(msg.starting_rate, -1);

  ClampBitrates(&start_bitrate_bps, &min_bitrate_bps, &max_bitrate_bps);

  bandwidth_estimation_ =
      rtc::MakeUnique<SendSideBandwidthEstimation>(event_log_);
  bandwidth_estimation_->SetBitrates(start_bitrate_bps, min_bitrate_bps,
                                     max_bitrate_bps);
  delay_based_bwe_.reset(new DelayBasedBwe(event_log_));
  acknowledged_bitrate_estimator_.reset(new AcknowledgedBitrateEstimator());
  delay_based_bwe_->SetStartBitrate(start_bitrate_bps);
  delay_based_bwe_->SetMinBitrate(min_bitrate_bps);

  probe_controller_->Reset(msg.at_time.ms());
  probe_controller_->SetBitrates(min_bitrate_bps, start_bitrate_bps,
                                 max_bitrate_bps, msg.at_time.ms());

  return MaybeTriggerOnNetworkChanged(msg.at_time);
}

NetworkControlUpdate GoogCcNetworkController::OnProcessInterval(
    ProcessInterval msg) {
  bandwidth_estimation_->UpdateEstimate(msg.at_time.ms());
  rtc::Optional<int64_t> start_time_ms =
      alr_detector_->GetApplicationLimitedRegionStartTime();
  probe_controller_->SetAlrStartTimeMs(start_time_ms);
  probe_controller_->Process(msg.at_time.ms());
  NetworkControlUpdate update = MaybeTriggerOnNetworkChanged(msg.at_time);
  for (const ProbeClusterConfig& config :
       probe_controller_->GetAndResetPendingProbes()) {
    update.probe_cluster_configs.push_back(config);
  }
  return update;
}

NetworkControlUpdate GoogCcNetworkController::OnRemoteBitrateReport(
    RemoteBitrateReport msg) {
  bandwidth_estimation_->UpdateReceiverEstimate(msg.receive_time.ms(),
                                                msg.bandwidth.bps());
  BWE_TEST_LOGGING_PLOT(1, "REMB_kbps", msg.receive_time.ms(),
                        msg.bandwidth.bps() / 1000);
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnRoundTripTimeUpdate(
    RoundTripTimeUpdate msg) {
  if (msg.smoothed) {
    delay_based_bwe_->OnRttUpdate(msg.round_trip_time.ms());
  } else {
    bandwidth_estimation_->UpdateRtt(msg.round_trip_time.ms(),
                                     msg.receive_time.ms());
  }
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnSentPacket(
    SentPacket sent_packet) {
  alr_detector_->OnBytesSent(sent_packet.size.bytes(),
                             sent_packet.send_time.ms());
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnStreamsConfig(
    StreamsConfig msg) {
  probe_controller_->EnablePeriodicAlrProbing(msg.requests_alr_probing);
  if (msg.max_total_allocated_bitrate &&
      *msg.max_total_allocated_bitrate != max_total_allocated_bitrate_) {
    probe_controller_->OnMaxTotalAllocatedBitrate(
        msg.max_total_allocated_bitrate->bps(), msg.at_time.ms());
    max_total_allocated_bitrate_ = *msg.max_total_allocated_bitrate;
  }
  bool pacing_changed = false;
  if (msg.pacing_factor && *msg.pacing_factor != pacing_factor_) {
    pacing_factor_ = *msg.pacing_factor;
    pacing_changed = true;
  }
  if (msg.min_pacing_rate && *msg.min_pacing_rate != min_pacing_rate_) {
    min_pacing_rate_ = *msg.min_pacing_rate;
    pacing_changed = true;
  }
  if (msg.max_padding_rate && *msg.max_padding_rate != max_padding_rate_) {
    max_padding_rate_ = *msg.max_padding_rate;
    pacing_changed = true;
  }
  NetworkControlUpdate update;
  if (pacing_changed)
    update.pacer_config = UpdatePacingRates(msg.at_time);
  return update;
}

NetworkControlUpdate GoogCcNetworkController::OnTargetRateConstraints(
    TargetRateConstraints constraints) {
  UpdateBitrateConstraints(constraints, rtc::nullopt);
  return MaybeTriggerOnNetworkChanged(constraints.at_time);
}

void GoogCcNetworkController::UpdateBitrateConstraints(
    TargetRateConstraints constraints,
    rtc::Optional<DataRate> starting_rate) {
  int64_t min_bitrate_bps = GetBpsOrDefault(constraints.min_data_rate, 0);
  int64_t max_bitrate_bps = GetBpsOrDefault(constraints.max_data_rate, -1);
  int64_t start_bitrate_bps = GetBpsOrDefault(starting_rate, -1);

  ClampBitrates(&start_bitrate_bps, &min_bitrate_bps, &max_bitrate_bps);

  probe_controller_->SetBitrates(min_bitrate_bps, start_bitrate_bps,
                                 max_bitrate_bps, constraints.at_time.ms());

  bandwidth_estimation_->SetBitrates(start_bitrate_bps, min_bitrate_bps,
                                     max_bitrate_bps);
  if (start_bitrate_bps > 0)
    delay_based_bwe_->SetStartBitrate(start_bitrate_bps);
  delay_based_bwe_->SetMinBitrate(min_bitrate_bps);
}

NetworkControlUpdate GoogCcNetworkController::OnTransportLossReport(
    TransportLossReport msg) {
  int64_t total_packets_delta =
      msg.packets_received_delta + msg.packets_lost_delta;
  bandwidth_estimation_->UpdatePacketsLost(
      msg.packets_lost_delta, total_packets_delta, msg.receive_time.ms());
  return NetworkControlUpdate();
}

NetworkControlUpdate GoogCcNetworkController::OnTransportPacketsFeedback(
    TransportPacketsFeedback report) {
  int64_t feedback_rtt = -1;
  for (const auto& packet_feedback : report.PacketsWithFeedback()) {
    if (packet_feedback.sent_packet.has_value() &&
        packet_feedback.receive_time.IsFinite()) {
      int64_t rtt = report.feedback_time.ms() -
                    packet_feedback.sent_packet->send_time.ms();
      // max() is used to account for feedback being delayed by the
      // receiver.
      feedback_rtt = std::max(rtt, feedback_rtt);
    }
  }
  if (feedback_rtt > -1) {
    feedback_rtts_.push_back(feedback_rtt);
    const size_t kFeedbackRttWindow = 32;
    if (feedback_rtts_.size() > kFeedbackRttWindow)
      feedback_rtts_.pop_front();
    min_feedback_rtt_ms_.emplace(
        *std::min_element(feedback_rtts_.begin(), feedback_rtts_.end()));
  }

  std::vector<PacketFeedback> received_feedback_vector =
      ReceivedPacketsFeedbackAsRtp(report);

  rtc::Optional<int64_t> alr_start_time =
      alr_detector_->GetApplicationLimitedRegionStartTime();

  if (previously_in_alr && !alr_start_time.has_value()) {
    int64_t now_ms = report.feedback_time.ms();
    acknowledged_bitrate_estimator_->SetAlrEndedTimeMs(now_ms);
    probe_controller_->SetAlrEndedTimeMs(now_ms);
  }
  previously_in_alr = alr_start_time.has_value();
  acknowledged_bitrate_estimator_->IncomingPacketFeedbackVector(
      received_feedback_vector);
  DelayBasedBwe::Result result;
  result = delay_based_bwe_->IncomingPacketFeedbackVector(
      received_feedback_vector, acknowledged_bitrate_estimator_->bitrate_bps(),
      report.feedback_time.ms());
  NetworkControlUpdate update;
  if (result.updated) {
    if (result.probe) {
      bandwidth_estimation_->SetSendBitrate(result.target_bitrate_bps);
    }
    // Since SetSendBitrate now resets the delay-based estimate, we have to call
    // UpdateDelayBasedEstimate after SetSendBitrate.
    bandwidth_estimation_->UpdateDelayBasedEstimate(report.feedback_time.ms(),
                                                    result.target_bitrate_bps);
    // Update the estimate in the ProbeController, in case we want to probe.
    update = MaybeTriggerOnNetworkChanged(report.feedback_time);
  }
  if (result.recovered_from_overuse) {
    probe_controller_->SetAlrStartTimeMs(alr_start_time);
    probe_controller_->RequestProbe(report.feedback_time.ms());
  }
  update.congestion_window = MaybeUpdateCongestionWindow();
  return update;
}

rtc::Optional<DataSize> GoogCcNetworkController::MaybeUpdateCongestionWindow() {
  if (!in_cwnd_experiment_)
    return rtc::nullopt;
  // No valid RTT. Could be because send-side BWE isn't used, in which case
  // we don't try to limit the outstanding packets.
  if (!min_feedback_rtt_ms_)
    return rtc::nullopt;

  const DataSize kMinCwnd = DataSize::bytes(2 * 1500);
  TimeDelta time_window =
      TimeDelta::ms(*min_feedback_rtt_ms_ + accepted_queue_ms_);
  DataSize data_window = last_bandwidth_ * time_window;
  data_window = std::max(kMinCwnd, data_window);
  RTC_LOG(LS_INFO) << "Feedback rtt: " << *min_feedback_rtt_ms_
                   << " Bitrate: " << last_bandwidth_.bps();
  return data_window;
}

NetworkControlUpdate GoogCcNetworkController::MaybeTriggerOnNetworkChanged(
    Timestamp at_time) {
  int32_t estimated_bitrate_bps;
  uint8_t fraction_loss;
  int64_t rtt_ms;

  bool estimate_changed = GetNetworkParameters(
      &estimated_bitrate_bps, &fraction_loss, &rtt_ms, at_time);
  if (estimate_changed) {
    TimeDelta bwe_period =
        TimeDelta::ms(delay_based_bwe_->GetExpectedBwePeriodMs());

    NetworkEstimate new_estimate;
    new_estimate.at_time = at_time;
    new_estimate.round_trip_time = TimeDelta::ms(rtt_ms);
    new_estimate.bandwidth = DataRate::bps(estimated_bitrate_bps);
    new_estimate.loss_rate_ratio = fraction_loss / 255.0f;
    new_estimate.bwe_period = bwe_period;
    last_bandwidth_ = new_estimate.bandwidth;
    return OnNetworkEstimate(new_estimate);
  }
  return NetworkControlUpdate();
}

bool GoogCcNetworkController::GetNetworkParameters(
    int32_t* estimated_bitrate_bps,
    uint8_t* fraction_loss,
    int64_t* rtt_ms,
    Timestamp at_time) {
  bandwidth_estimation_->CurrentEstimate(estimated_bitrate_bps, fraction_loss,
                                         rtt_ms);
  *estimated_bitrate_bps = std::max<int32_t>(
      *estimated_bitrate_bps, bandwidth_estimation_->GetMinBitrate());

  bool estimate_changed = false;
  if ((*estimated_bitrate_bps != last_estimated_bitrate_bps_) ||
      (*fraction_loss != last_estimated_fraction_loss_) ||
      (*rtt_ms != last_estimated_rtt_ms_)) {
    last_estimated_bitrate_bps_ = *estimated_bitrate_bps;
    last_estimated_fraction_loss_ = *fraction_loss;
    last_estimated_rtt_ms_ = *rtt_ms;
    estimate_changed = true;
  }

  BWE_TEST_LOGGING_PLOT(1, "fraction_loss_%", at_time.ms(),
                        (*fraction_loss * 100) / 256);
  BWE_TEST_LOGGING_PLOT(1, "rtt_ms", at_time.ms(), *rtt_ms);
  BWE_TEST_LOGGING_PLOT(1, "Target_bitrate_kbps", at_time.ms(),
                        *estimated_bitrate_bps / 1000);

  return estimate_changed;
}

NetworkControlUpdate GoogCcNetworkController::OnNetworkEstimate(
    NetworkEstimate estimate) {
  NetworkControlUpdate update;
  update.pacer_config = UpdatePacingRates(estimate.at_time);
  alr_detector_->SetEstimatedBitrate(estimate.bandwidth.bps());
  probe_controller_->SetEstimatedBitrate(estimate.bandwidth.bps(),
                                         estimate.at_time.ms());

  TargetTransferRate target_rate;
  target_rate.at_time = estimate.at_time;
  // Set the target rate to the full estimated bandwidth since the estimation
  // for legacy reasons includes target rate constraints.
  target_rate.target_rate = estimate.bandwidth;
  target_rate.network_estimate = estimate;
  update.target_rate = target_rate;
  return update;
}

PacerConfig GoogCcNetworkController::UpdatePacingRates(Timestamp at_time) {
  DataRate pacing_rate =
      std::max(min_pacing_rate_, last_bandwidth_) * pacing_factor_;
  DataRate padding_rate = std::min(max_padding_rate_, last_bandwidth_);
  PacerConfig msg;
  msg.at_time = at_time;
  msg.time_window = TimeDelta::seconds(1);
  msg.data_window = pacing_rate * msg.time_window;
  msg.pad_window = padding_rate * msg.time_window;
  return msg;
}
}  // namespace webrtc_cc
}  // namespace webrtc
