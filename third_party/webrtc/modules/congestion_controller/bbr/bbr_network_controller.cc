/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/congestion_controller/bbr/bbr_network_controller.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "rtc_base/checks.h"
#include "rtc_base/experiments/congestion_controller_experiment.h"
#include "rtc_base/logging.h"
#include "rtc_base/system/fallthrough.h"

namespace webrtc {
namespace bbr {
namespace {

// If greater than zero, mean RTT variation is multiplied by the specified
// factor and added to the congestion window limit.
const double kBbrRttVariationWeight = 0.0f;

// Congestion window gain for QUIC BBR during PROBE_BW phase.
const double kProbeBWCongestionWindowGain = 2.0f;

// The maximum packet size of any QUIC packet, based on ethernet's max size,
// minus the IP and UDP headers. IPv6 has a 40 byte header, UDP adds an
// additional 8 bytes.  This is a total overhead of 48 bytes.  Ethernet's
// max packet size is 1500 bytes,  1500 - 48 = 1452.
const DataSize kMaxPacketSize = DataSize::bytes(1452);

// Default maximum packet size used in the Linux TCP implementation.
// Used in QUIC for congestion window computations in bytes.
const DataSize kDefaultTCPMSS = DataSize::bytes(1460);
// Constants based on TCP defaults.
const DataSize kMaxSegmentSize = kDefaultTCPMSS;
// The minimum CWND to ensure delayed acks don't reduce bandwidth measurements.
// Does not inflate the pacing rate.
const DataSize kMinimumCongestionWindow = DataSize::bytes(1000);

// The gain used for the slow start, equal to 2/ln(2).
const double kHighGain = 2.885f;
// The gain used in STARTUP after loss has been detected.
// 1.5 is enough to allow for 25% exogenous loss and still observe a 25% growth
// in measured bandwidth.
const double kStartupAfterLossGain = 1.5;
// The gain used to drain the queue after the slow start.
const double kDrainGain = 1.f / kHighGain;

// The length of the gain cycle.
const size_t kGainCycleLength = 8;
// The size of the bandwidth filter window, in round-trips.
const BbrRoundTripCount kBandwidthWindowSize = kGainCycleLength + 2;

// The time after which the current min_rtt value expires.
constexpr int64_t kMinRttExpirySeconds = 10;
// The minimum time the connection can spend in PROBE_RTT mode.
constexpr int64_t kProbeRttTimeMs = 200;
// If the bandwidth does not increase by the factor of |kStartupGrowthTarget|
// within |kRoundTripsWithoutGrowthBeforeExitingStartup| rounds, the connection
// will exit the STARTUP mode.
const double kStartupGrowthTarget = 1.25;
// Coefficient to determine if a new RTT is sufficiently similar to min_rtt that
// we don't need to enter PROBE_RTT.
const double kSimilarMinRttThreshold = 1.125;

constexpr int64_t kInitialRttMs = 200;
constexpr int64_t kInitialBandwidthKbps = 300;

constexpr int64_t kMaxRttMs = 1000;
constexpr int64_t kMaxBandwidthKbps = 5000;

constexpr int64_t kInitialCongestionWindowBytes =
    (kInitialRttMs * kInitialBandwidthKbps) / 8;
constexpr int64_t kDefaultMaxCongestionWindowBytes =
    (kMaxRttMs * kMaxBandwidthKbps) / 8;
}  // namespace

BbrNetworkController::UpdateState::UpdateState() = default;
BbrNetworkController::UpdateState::UpdateState(
    const BbrNetworkController::UpdateState&) = default;
BbrNetworkController::UpdateState::~UpdateState() = default;

BbrNetworkController::BbrControllerConfig
BbrNetworkController::BbrControllerConfig::DefaultConfig() {
  BbrControllerConfig config;
  config.probe_bw_pacing_gain_offset = 0.25;
  config.encoder_rate_gain = 1;
  config.encoder_rate_gain_in_probe_rtt = 1;
  config.exit_startup_rtt_threshold_ms = 0;
  config.probe_rtt_congestion_window_gain = 0.75;
  config.exit_startup_on_loss = true;
  config.num_startup_rtts = 3;
  config.rate_based_recovery = false;
  config.max_aggregation_bytes_multiplier = 0;
  config.slower_startup = false;
  config.rate_based_startup = false;
  config.fully_drain_queue = false;
  config.initial_conservation_in_startup = CONSERVATION;
  config.max_ack_height_window_multiplier = 1;
  config.probe_rtt_based_on_bdp = false;
  config.probe_rtt_skipped_if_similar_rtt = false;
  config.probe_rtt_disabled_if_app_limited = false;

  return config;
}

BbrNetworkController::BbrControllerConfig
BbrNetworkController::BbrControllerConfig::ExperimentConfig() {
  auto exp = CongestionControllerExperiment::GetBbrExperimentConfig();
  if (exp) {
    BbrControllerConfig config;
    config.exit_startup_on_loss = exp->exit_startup_on_loss;
    config.exit_startup_rtt_threshold_ms = exp->exit_startup_rtt_threshold_ms;
    config.fully_drain_queue = exp->fully_drain_queue;
    config.initial_conservation_in_startup =
        static_cast<RecoveryState>(exp->initial_conservation_in_startup);
    config.num_startup_rtts = exp->num_startup_rtts;
    config.probe_rtt_based_on_bdp = exp->probe_rtt_based_on_bdp;
    config.probe_rtt_disabled_if_app_limited =
        exp->probe_rtt_disabled_if_app_limited;
    config.probe_rtt_skipped_if_similar_rtt =
        exp->probe_rtt_skipped_if_similar_rtt;
    config.rate_based_recovery = exp->rate_based_recovery;
    config.rate_based_startup = exp->rate_based_startup;
    config.slower_startup = exp->slower_startup;
    config.encoder_rate_gain = exp->encoder_rate_gain;
    config.encoder_rate_gain_in_probe_rtt = exp->encoder_rate_gain_in_probe_rtt;
    config.max_ack_height_window_multiplier =
        exp->max_ack_height_window_multiplier;
    config.max_aggregation_bytes_multiplier =
        exp->max_aggregation_bytes_multiplier;
    config.probe_bw_pacing_gain_offset = exp->probe_bw_pacing_gain_offset;
    config.probe_rtt_congestion_window_gain =
        exp->probe_rtt_congestion_window_gain;
    return config;
  } else {
    return DefaultConfig();
  }
}

BbrNetworkController::DebugState::DebugState(const BbrNetworkController& sender)
    : mode(sender.mode_),
      max_bandwidth(sender.max_bandwidth_.GetBest()),
      round_trip_count(sender.round_trip_count_),
      gain_cycle_index(sender.cycle_current_offset_),
      congestion_window(sender.congestion_window_),
      is_at_full_bandwidth(sender.is_at_full_bandwidth_),
      bandwidth_at_last_round(sender.bandwidth_at_last_round_),
      rounds_without_bandwidth_gain(sender.rounds_without_bandwidth_gain_),
      min_rtt(sender.min_rtt_),
      min_rtt_timestamp(sender.min_rtt_timestamp_),
      recovery_state(sender.recovery_state_),
      recovery_window(sender.recovery_window_),
      last_sample_is_app_limited(sender.last_sample_is_app_limited_),
      end_of_app_limited_phase(sender.end_of_app_limited_phase_) {}

BbrNetworkController::DebugState::DebugState(const DebugState& state) = default;

BbrNetworkController::BbrNetworkController(NetworkControllerConfig config)
    : random_(10),
      max_bandwidth_(kBandwidthWindowSize, DataRate::Zero(), 0),
      default_bandwidth_(DataRate::kbps(kInitialBandwidthKbps)),
      max_ack_height_(kBandwidthWindowSize, DataSize::Zero(), 0),
      congestion_window_(DataSize::bytes(kInitialCongestionWindowBytes)),
      initial_congestion_window_(
          DataSize::bytes(kInitialCongestionWindowBytes)),
      max_congestion_window_(DataSize::bytes(kDefaultMaxCongestionWindowBytes)),
      congestion_window_gain_constant_(kProbeBWCongestionWindowGain),
      rtt_variance_weight_(kBbrRttVariationWeight),
      recovery_window_(max_congestion_window_) {
  RTC_LOG(LS_INFO) << "Creating BBR controller";
  config_ = BbrControllerConfig::ExperimentConfig();
  if (config.starting_bandwidth.IsFinite())
    default_bandwidth_ = config.starting_bandwidth;
  constraints_ = config.constraints;
  Reset();
  if (config_.num_startup_rtts > 0) {
    EnterStartupMode();
  } else {
    EnterProbeBandwidthMode(constraints_->at_time);
  }
}

BbrNetworkController::~BbrNetworkController() {}

void BbrNetworkController::Reset() {
  round_trip_count_ = 0;
  rounds_without_bandwidth_gain_ = 0;
  is_at_full_bandwidth_ = false;
  last_update_state_.mode = Mode::STARTUP;
  last_update_state_.bandwidth.reset();
  last_update_state_.rtt.reset();
  last_update_state_.pacing_rate.reset();
  last_update_state_.target_rate.reset();
  last_update_state_.probing_for_bandwidth = false;
  EnterStartupMode();
}

NetworkControlUpdate BbrNetworkController::CreateRateUpdate(Timestamp at_time) {
  DataRate bandwidth = BandwidthEstimate();
  if (bandwidth.IsZero())
    bandwidth = default_bandwidth_;
  TimeDelta rtt = GetMinRtt();
  DataRate pacing_rate = PacingRate();
  DataRate target_rate = bandwidth;
  if (mode_ == PROBE_RTT)
    target_rate = bandwidth * config_.encoder_rate_gain_in_probe_rtt;
  else
    target_rate = bandwidth * config_.encoder_rate_gain;
  target_rate = std::min(target_rate, pacing_rate);

  if (constraints_) {
    if (constraints_->max_data_rate)
      target_rate = std::min(target_rate, *constraints_->max_data_rate);
    if (constraints_->min_data_rate)
      target_rate = std::max(target_rate, *constraints_->min_data_rate);
  }
  bool probing_for_bandwidth = IsProbingForMoreBandwidth();
  if (last_update_state_.mode == mode_ &&
      last_update_state_.bandwidth == bandwidth &&
      last_update_state_.rtt == rtt &&
      last_update_state_.pacing_rate == pacing_rate &&
      last_update_state_.target_rate == target_rate &&
      last_update_state_.probing_for_bandwidth == probing_for_bandwidth)
    return NetworkControlUpdate();
  last_update_state_.mode = mode_;
  last_update_state_.bandwidth = bandwidth;
  last_update_state_.rtt = rtt;
  last_update_state_.pacing_rate = pacing_rate;
  last_update_state_.target_rate = target_rate;
  last_update_state_.probing_for_bandwidth = probing_for_bandwidth;

  NetworkControlUpdate update;

  TargetTransferRate target_rate_msg;
  target_rate_msg.network_estimate.at_time = at_time;
  target_rate_msg.network_estimate.bandwidth = bandwidth;
  target_rate_msg.network_estimate.round_trip_time = rtt;

  // TODO(srte): Fill in fields below with proper values.
  target_rate_msg.network_estimate.loss_rate_ratio = 0;
  target_rate_msg.network_estimate.bwe_period = TimeDelta::Zero();

  target_rate_msg.target_rate = target_rate;
  target_rate_msg.at_time = at_time;
  update.target_rate = target_rate_msg;

  PacerConfig pacer_config;
  // A small time window ensures an even pacing rate.
  pacer_config.time_window = rtt * 0.25;
  pacer_config.data_window = pacer_config.time_window * pacing_rate;

  if (IsProbingForMoreBandwidth())
    pacer_config.pad_window = pacer_config.data_window;
  else
    pacer_config.pad_window = DataSize::Zero();

  pacer_config.at_time = at_time;
  update.pacer_config = pacer_config;

  update.congestion_window = GetCongestionWindow();
  return update;
}

NetworkControlUpdate BbrNetworkController::OnNetworkAvailability(
    NetworkAvailability msg) {
  Reset();
  rtt_stats_.OnConnectionMigration();
  return CreateRateUpdate(msg.at_time);
}

NetworkControlUpdate BbrNetworkController::OnNetworkRouteChange(
    NetworkRouteChange msg) {
  constraints_ = msg.constraints;
  Reset();
  if (msg.starting_rate)
    default_bandwidth_ = *msg.starting_rate;

  rtt_stats_.OnConnectionMigration();
  return CreateRateUpdate(msg.at_time);
}

NetworkControlUpdate BbrNetworkController::OnProcessInterval(
    ProcessInterval msg) {
  return CreateRateUpdate(msg.at_time);
}

NetworkControlUpdate BbrNetworkController::OnStreamsConfig(StreamsConfig msg) {
  return NetworkControlUpdate();
}

NetworkControlUpdate BbrNetworkController::OnTargetRateConstraints(
    TargetRateConstraints msg) {
  constraints_ = msg;
  return CreateRateUpdate(msg.at_time);
}

bool BbrNetworkController::InSlowStart() const {
  return mode_ == STARTUP;
}

NetworkControlUpdate BbrNetworkController::OnSentPacket(SentPacket msg) {
  last_send_time_ = msg.send_time;
  if (!aggregation_epoch_start_time_) {
    aggregation_epoch_start_time_ = msg.send_time;
    aggregation_epoch_bytes_ = DataSize::Zero();
  }
  return NetworkControlUpdate();
}

bool BbrNetworkController::CanSend(DataSize bytes_in_flight) {
  return bytes_in_flight < GetCongestionWindow();
}

DataRate BbrNetworkController::PacingRate() const {
  if (pacing_rate_.IsZero()) {
    return kHighGain * initial_congestion_window_ / GetMinRtt();
  }
  return pacing_rate_;
}

DataRate BbrNetworkController::BandwidthEstimate() const {
  return max_bandwidth_.GetBest();
}

DataSize BbrNetworkController::GetCongestionWindow() const {
  if (mode_ == PROBE_RTT) {
    return ProbeRttCongestionWindow();
  }

  if (InRecovery() && !config_.rate_based_recovery &&
      !(config_.rate_based_startup && mode_ == STARTUP)) {
    return std::min(congestion_window_, recovery_window_);
  }

  return congestion_window_;
}

double BbrNetworkController::GetPacingGain(int round_offset) const {
  if (round_offset == 0)
    return 1 + config_.probe_bw_pacing_gain_offset;
  else if (round_offset == 1)
    return 1 - config_.probe_bw_pacing_gain_offset;
  else
    return 1;
}

bool BbrNetworkController::InRecovery() const {
  return recovery_state_ != NOT_IN_RECOVERY;
}

bool BbrNetworkController::IsProbingForMoreBandwidth() const {
  return (mode_ == PROBE_BW && pacing_gain_ > 1) || mode_ == STARTUP;
}

NetworkControlUpdate BbrNetworkController::OnTransportPacketsFeedback(
    TransportPacketsFeedback msg) {
  Timestamp feedback_recv_time = msg.feedback_time;
  rtc::Optional<SentPacket> last_sent_packet =
      msg.PacketsWithFeedback().back().sent_packet;
  if (!last_sent_packet.has_value()) {
    RTC_LOG(LS_WARNING) << "Last ack packet not in history, no RTT update";
  } else {
    Timestamp send_time = last_sent_packet->send_time;
    TimeDelta send_delta = feedback_recv_time - send_time;
    rtt_stats_.UpdateRtt(send_delta, TimeDelta::Zero(), feedback_recv_time);
  }

  DataSize bytes_in_flight = msg.data_in_flight;
  DataSize total_acked_size = DataSize::Zero();

  bool is_round_start = false;
  bool min_rtt_expired = false;

  std::vector<PacketResult> acked_packets = msg.ReceivedWithSendInfo();
  std::vector<PacketResult> lost_packets = msg.LostWithSendInfo();
  // Input the new data into the BBR model of the connection.
  if (!acked_packets.empty()) {
    for (const PacketResult& packet : acked_packets) {
      const SentPacket& sent_packet = *packet.sent_packet;
      send_ack_tracker_.AddSample(sent_packet.size, sent_packet.send_time,
                                  msg.feedback_time);
      total_acked_size += sent_packet.size;
    }
    Timestamp last_acked_send_time =
        acked_packets.rbegin()->sent_packet->send_time;
    is_round_start = UpdateRoundTripCounter(last_acked_send_time);
    UpdateBandwidth(msg.feedback_time, acked_packets);
    // Min rtt will be the rtt for the last packet, since all packets are acked
    // at the same time.
    Timestamp last_send_time = acked_packets.back().sent_packet->send_time;
    min_rtt_expired = UpdateMinRtt(msg.feedback_time, last_send_time);
    UpdateRecoveryState(last_acked_send_time, !lost_packets.empty(),
                        is_round_start);

    UpdateAckAggregationBytes(msg.feedback_time, total_acked_size);
    if (max_aggregation_bytes_multiplier_ > 0) {
      if (msg.data_in_flight <=
          1.25 * GetTargetCongestionWindow(pacing_gain_)) {
        bytes_acked_since_queue_drained_ = DataSize::Zero();
      } else {
        bytes_acked_since_queue_drained_ += total_acked_size;
      }
    }
  }
  total_bytes_acked_ += total_acked_size;

  // Handle logic specific to PROBE_BW mode.
  if (mode_ == PROBE_BW) {
    UpdateGainCyclePhase(msg.feedback_time, msg.prior_in_flight,
                         !lost_packets.empty());
  }

  // Handle logic specific to STARTUP and DRAIN modes.
  if (is_round_start && !is_at_full_bandwidth_) {
    CheckIfFullBandwidthReached();
  }
  MaybeExitStartupOrDrain(msg);

  // Handle logic specific to PROBE_RTT.
  MaybeEnterOrExitProbeRtt(msg, is_round_start, min_rtt_expired);

  // Calculate number of packets acked and lost.
  DataSize bytes_lost = DataSize::Zero();
  for (const PacketResult& packet : lost_packets) {
    bytes_lost += packet.sent_packet->size;
  }

  // After the model is updated, recalculate the pacing rate and congestion
  // window.
  CalculatePacingRate();
  CalculateCongestionWindow(total_acked_size);
  CalculateRecoveryWindow(total_acked_size, bytes_lost, bytes_in_flight);
  return CreateRateUpdate(msg.feedback_time);
}

NetworkControlUpdate BbrNetworkController::OnRemoteBitrateReport(
    RemoteBitrateReport msg) {
  return NetworkControlUpdate();
}
NetworkControlUpdate BbrNetworkController::OnRoundTripTimeUpdate(
    RoundTripTimeUpdate msg) {
  return NetworkControlUpdate();
}
NetworkControlUpdate BbrNetworkController::OnTransportLossReport(
    TransportLossReport msg) {
  return NetworkControlUpdate();
}

TimeDelta BbrNetworkController::GetMinRtt() const {
  return !min_rtt_.IsZero() ? min_rtt_
                            : TimeDelta::us(rtt_stats_.initial_rtt_us());
}

DataSize BbrNetworkController::GetTargetCongestionWindow(double gain) const {
  DataSize bdp = GetMinRtt() * BandwidthEstimate();
  DataSize congestion_window = gain * bdp;

  // BDP estimate will be zero if no bandwidth samples are available yet.
  if (congestion_window.IsZero()) {
    congestion_window = gain * initial_congestion_window_;
  }

  return std::max(congestion_window, kMinimumCongestionWindow);
}

DataSize BbrNetworkController::ProbeRttCongestionWindow() const {
  if (config_.probe_rtt_based_on_bdp) {
    return GetTargetCongestionWindow(config_.probe_rtt_congestion_window_gain);
  }
  return kMinimumCongestionWindow;
}

void BbrNetworkController::EnterStartupMode() {
  mode_ = STARTUP;
  pacing_gain_ = kHighGain;
  congestion_window_gain_ = kHighGain;
}

void BbrNetworkController::EnterProbeBandwidthMode(Timestamp now) {
  mode_ = PROBE_BW;
  congestion_window_gain_ = congestion_window_gain_constant_;

  // Pick a random offset for the gain cycle out of {0, 2..7} range. 1 is
  // excluded because in that case increased gain and decreased gain would not
  // follow each other.
  cycle_current_offset_ = random_.Rand(kGainCycleLength - 1);
  if (cycle_current_offset_ >= 1) {
    cycle_current_offset_ += 1;
  }

  last_cycle_start_ = now;
  pacing_gain_ = GetPacingGain(cycle_current_offset_);
}

bool BbrNetworkController::UpdateRoundTripCounter(
    Timestamp last_acked_send_time) {
  if (last_acked_send_time > current_round_trip_end_) {
    round_trip_count_++;
    current_round_trip_end_ = last_send_time_;
    return true;
  }

  return false;
}

bool BbrNetworkController::UpdateMinRtt(Timestamp ack_time,
                                        Timestamp last_packet_send_time) {
  // Note: This sample does not account for delayed acknowledgement time.  This
  // means that the RTT measurements here can be artificially high, especially
  // on low bandwidth connections.
  TimeDelta sample_rtt = ack_time - last_packet_send_time;
  last_rtt_ = sample_rtt;
  min_rtt_since_last_probe_rtt_ =
      std::min(min_rtt_since_last_probe_rtt_, sample_rtt);

  // Do not expire min_rtt if none was ever available.
  bool min_rtt_expired =
      !min_rtt_.IsZero() &&
      (ack_time >
       (min_rtt_timestamp_ + TimeDelta::seconds(kMinRttExpirySeconds)));

  if (min_rtt_expired || sample_rtt < min_rtt_ || min_rtt_.IsZero()) {
    if (ShouldExtendMinRttExpiry()) {
      min_rtt_expired = false;
    } else {
      min_rtt_ = sample_rtt;
    }
    min_rtt_timestamp_ = ack_time;
    // Reset since_last_probe_rtt fields.
    min_rtt_since_last_probe_rtt_ = TimeDelta::PlusInfinity();
    app_limited_since_last_probe_rtt_ = false;
  }

  return min_rtt_expired;
}

void BbrNetworkController::UpdateBandwidth(
    Timestamp ack_time,
    const std::vector<PacketResult>& acked_packets) {
  // There are two possible maximum receive bandwidths based on the duration
  // from send to ack of a packet, either including or excluding the time until
  // the current ack was received. Therefore looking at the last and the first
  // packet is enough. This holds if at most one feedback was received during
  // the sending of the acked packets.
  std::array<const PacketResult, 2> packets = {
      {acked_packets.front(), acked_packets.back()}};
  for (const PacketResult& packet : packets) {
    const Timestamp& send_time = packet.sent_packet->send_time;
    is_app_limited_ = send_time > end_of_app_limited_phase_;
    auto result = send_ack_tracker_.GetRatesByAckTime(send_time, ack_time);
    if (result.acked_data == DataSize::Zero())
      continue;
    send_ack_tracker_.ClearOldSamples(send_time);

    DataRate ack_rate = result.acked_data / result.ack_timespan;
    DataRate send_rate = result.send_timespan.IsZero()
                             ? DataRate::Infinity()
                             : result.acked_data / result.send_timespan;
    DataRate bandwidth = std::min(send_rate, ack_rate);
    if (!bandwidth.IsFinite())
      continue;
    if (!is_app_limited_ || bandwidth > BandwidthEstimate()) {
      max_bandwidth_.Update(bandwidth, round_trip_count_);
    }
  }
}

bool BbrNetworkController::ShouldExtendMinRttExpiry() const {
  if (config_.probe_rtt_disabled_if_app_limited &&
      app_limited_since_last_probe_rtt_) {
    // Extend the current min_rtt if we've been app limited recently.
    return true;
  }
  const bool min_rtt_increased_since_last_probe =
      min_rtt_since_last_probe_rtt_ > min_rtt_ * kSimilarMinRttThreshold;
  if (config_.probe_rtt_skipped_if_similar_rtt &&
      app_limited_since_last_probe_rtt_ &&
      !min_rtt_increased_since_last_probe) {
    // Extend the current min_rtt if we've been app limited recently and an rtt
    // has been measured in that time that's less than 12.5% more than the
    // current min_rtt.
    return true;
  }
  return false;
}

void BbrNetworkController::UpdateGainCyclePhase(Timestamp now,
                                                DataSize prior_in_flight,
                                                bool has_losses) {
  // In most cases, the cycle is advanced after an RTT passes.
  bool should_advance_gain_cycling = now - last_cycle_start_ > GetMinRtt();

  // If the pacing gain is above 1.0, the connection is trying to probe the
  // bandwidth by increasing the number of bytes in flight to at least
  // pacing_gain * BDP.  Make sure that it actually reaches the target, as long
  // as there are no losses suggesting that the buffers are not able to hold
  // that much.
  if (pacing_gain_ > 1.0 && !has_losses &&
      prior_in_flight < GetTargetCongestionWindow(pacing_gain_)) {
    should_advance_gain_cycling = false;
  }

  // If pacing gain is below 1.0, the connection is trying to drain the extra
  // queue which could have been incurred by probing prior to it.  If the number
  // of bytes in flight falls down to the estimated BDP value earlier, conclude
  // that the queue has been successfully drained and exit this cycle early.
  if (pacing_gain_ < 1.0 && prior_in_flight <= GetTargetCongestionWindow(1)) {
    should_advance_gain_cycling = true;
  }

  if (should_advance_gain_cycling) {
    cycle_current_offset_ = (cycle_current_offset_ + 1) % kGainCycleLength;
    last_cycle_start_ = now;
    // Stay in low gain mode until the target BDP is hit.
    // Low gain mode will be exited immediately when the target BDP is achieved.
    if (config_.fully_drain_queue && pacing_gain_ < 1 &&
        GetPacingGain(cycle_current_offset_) == 1 &&
        prior_in_flight > GetTargetCongestionWindow(1)) {
      return;
    }
    pacing_gain_ = GetPacingGain(cycle_current_offset_);
  }
}

void BbrNetworkController::CheckIfFullBandwidthReached() {
  if (last_sample_is_app_limited_) {
    return;
  }

  DataRate target = bandwidth_at_last_round_ * kStartupGrowthTarget;
  if (BandwidthEstimate() >= target) {
    bandwidth_at_last_round_ = BandwidthEstimate();
    rounds_without_bandwidth_gain_ = 0;
    return;
  }

  rounds_without_bandwidth_gain_++;
  if ((rounds_without_bandwidth_gain_ >= config_.num_startup_rtts) ||
      (exit_startup_on_loss_ && InRecovery())) {
    is_at_full_bandwidth_ = true;
  }
}

void BbrNetworkController::MaybeExitStartupOrDrain(
    const TransportPacketsFeedback& msg) {
  int64_t exit_threshold_ms = config_.exit_startup_rtt_threshold_ms;
  bool rtt_over_threshold =
      exit_threshold_ms > 0 && (last_rtt_ - min_rtt_).ms() > exit_threshold_ms;
  if (mode_ == STARTUP && (is_at_full_bandwidth_ || rtt_over_threshold)) {
    if (rtt_over_threshold)
      RTC_LOG(LS_INFO) << "Exiting startup due to rtt increase from: "
                       << ToString(min_rtt_) << " to:" << ToString(last_rtt_)
                       << " > "
                       << ToString(min_rtt_ + TimeDelta::ms(exit_threshold_ms));
    mode_ = DRAIN;
    pacing_gain_ = kDrainGain;
    congestion_window_gain_ = kHighGain;
  }
  if (mode_ == DRAIN && msg.data_in_flight <= GetTargetCongestionWindow(1)) {
    EnterProbeBandwidthMode(msg.feedback_time);
  }
}

void BbrNetworkController::MaybeEnterOrExitProbeRtt(
    const TransportPacketsFeedback& msg,
    bool is_round_start,
    bool min_rtt_expired) {
  if (min_rtt_expired && mode_ != PROBE_RTT) {
    mode_ = PROBE_RTT;
    pacing_gain_ = 1;
    // Do not decide on the time to exit PROBE_RTT until the |bytes_in_flight|
    // is at the target small value.
    exit_probe_rtt_at_.reset();
  }

  if (mode_ == PROBE_RTT) {
    is_app_limited_ = true;
    end_of_app_limited_phase_ = last_send_time_;

    if (!exit_probe_rtt_at_) {
      // If the window has reached the appropriate size, schedule exiting
      // PROBE_RTT.  The CWND during PROBE_RTT is kMinimumCongestionWindow, but
      // we allow an extra packet since QUIC checks CWND before sending a
      // packet.
      if (msg.data_in_flight < ProbeRttCongestionWindow() + kMaxPacketSize) {
        exit_probe_rtt_at_ = msg.feedback_time + TimeDelta::ms(kProbeRttTimeMs);
        probe_rtt_round_passed_ = false;
      }
    } else {
      if (is_round_start) {
        probe_rtt_round_passed_ = true;
      }
      if (msg.feedback_time >= *exit_probe_rtt_at_ && probe_rtt_round_passed_) {
        min_rtt_timestamp_ = msg.feedback_time;
        if (!is_at_full_bandwidth_) {
          EnterStartupMode();
        } else {
          EnterProbeBandwidthMode(msg.feedback_time);
        }
      }
    }
  }
}

void BbrNetworkController::UpdateRecoveryState(Timestamp last_acked_send_time,
                                               bool has_losses,
                                               bool is_round_start) {
  // Exit recovery when there are no losses for a round.
  if (has_losses) {
    end_recovery_at_ = last_acked_send_time;
  }

  switch (recovery_state_) {
    case NOT_IN_RECOVERY:
      // Enter conservation on the first loss.
      if (has_losses) {
        recovery_state_ = CONSERVATION;
        if (mode_ == STARTUP) {
          recovery_state_ = config_.initial_conservation_in_startup;
        }
        // This will cause the |recovery_window_| to be set to the correct
        // value in CalculateRecoveryWindow().
        recovery_window_ = DataSize::Zero();
        // Since the conservation phase is meant to be lasting for a whole
        // round, extend the current round as if it were started right now.
        current_round_trip_end_ = last_send_time_;
      }
      break;

    case CONSERVATION:
    case MEDIUM_GROWTH:
      if (is_round_start) {
        recovery_state_ = GROWTH;
      }
      RTC_FALLTHROUGH();
    case GROWTH:
      // Exit recovery if appropriate.
      if (!has_losses && end_recovery_at_ &&
          last_acked_send_time > *end_recovery_at_) {
        recovery_state_ = NOT_IN_RECOVERY;
      }

      break;
  }
}

void BbrNetworkController::UpdateAckAggregationBytes(
    Timestamp ack_time,
    DataSize newly_acked_bytes) {
  if (!aggregation_epoch_start_time_) {
    RTC_LOG(LS_ERROR)
        << "Received feedback before information about sent packets.";
    RTC_DCHECK(aggregation_epoch_start_time_.has_value());
    return;
  }
  // Compute how many bytes are expected to be delivered, assuming max bandwidth
  // is correct.
  DataSize expected_bytes_acked =
      max_bandwidth_.GetBest() * (ack_time - *aggregation_epoch_start_time_);
  // Reset the current aggregation epoch as soon as the ack arrival rate is less
  // than or equal to the max bandwidth.
  if (aggregation_epoch_bytes_ <= expected_bytes_acked) {
    // Reset to start measuring a new aggregation epoch.
    aggregation_epoch_bytes_ = newly_acked_bytes;
    aggregation_epoch_start_time_ = ack_time;
    return;
  }

  // Compute how many extra bytes were delivered vs max bandwidth.
  // Include the bytes most recently acknowledged to account for stretch acks.
  aggregation_epoch_bytes_ += newly_acked_bytes;
  max_ack_height_.Update(aggregation_epoch_bytes_ - expected_bytes_acked,
                         round_trip_count_);
}

void BbrNetworkController::CalculatePacingRate() {
  if (BandwidthEstimate().IsZero()) {
    return;
  }

  DataRate target_rate = pacing_gain_ * BandwidthEstimate();
  if (config_.rate_based_recovery && InRecovery()) {
    pacing_rate_ = pacing_gain_ * max_bandwidth_.GetThirdBest();
  }
  if (is_at_full_bandwidth_) {
    pacing_rate_ = target_rate;
    return;
  }

  // Pace at the rate of initial_window / RTT as soon as RTT measurements are
  // available.
  if (pacing_rate_.IsZero() && !rtt_stats_.min_rtt().IsZero()) {
    pacing_rate_ = initial_congestion_window_ / rtt_stats_.min_rtt();
    return;
  }
  // Slow the pacing rate in STARTUP once loss has ever been detected.
  const bool has_ever_detected_loss = end_recovery_at_.has_value();
  if (config_.slower_startup && has_ever_detected_loss) {
    pacing_rate_ = kStartupAfterLossGain * BandwidthEstimate();
    return;
  }

  // Do not decrease the pacing rate during the startup.
  pacing_rate_ = std::max(pacing_rate_, target_rate);
}

void BbrNetworkController::CalculateCongestionWindow(DataSize bytes_acked) {
  if (mode_ == PROBE_RTT) {
    return;
  }

  DataSize target_window = GetTargetCongestionWindow(congestion_window_gain_);

  if (rtt_variance_weight_ > 0.f && !BandwidthEstimate().IsZero()) {
    target_window += rtt_variance_weight_ * rtt_stats_.mean_deviation() *
                     BandwidthEstimate();
  } else if (max_aggregation_bytes_multiplier_ > 0 && is_at_full_bandwidth_) {
    // Subtracting only half the bytes_acked_since_queue_drained ensures sending
    // doesn't completely stop for a long period of time if the queue hasn't
    // been drained recently.
    if (max_aggregation_bytes_multiplier_ * max_ack_height_.GetBest() >
        bytes_acked_since_queue_drained_ / 2) {
      target_window +=
          max_aggregation_bytes_multiplier_ * max_ack_height_.GetBest() -
          bytes_acked_since_queue_drained_ / 2;
    }
  } else if (is_at_full_bandwidth_) {
    target_window += max_ack_height_.GetBest();
  }

  // Instead of immediately setting the target CWND as the new one, BBR grows
  // the CWND towards |target_window| by only increasing it |bytes_acked| at a
  // time.
  if (is_at_full_bandwidth_) {
    congestion_window_ =
        std::min(target_window, congestion_window_ + bytes_acked);
  } else if (congestion_window_ < target_window ||
             total_bytes_acked_ < initial_congestion_window_) {
    // If the connection is not yet out of startup phase, do not decrease the
    // window.
    congestion_window_ = congestion_window_ + bytes_acked;
  }

  // Enforce the limits on the congestion window.
  congestion_window_ = std::max(congestion_window_, kMinimumCongestionWindow);
  congestion_window_ = std::min(congestion_window_, max_congestion_window_);
}

void BbrNetworkController::CalculateRecoveryWindow(DataSize bytes_acked,
                                                   DataSize bytes_lost,
                                                   DataSize bytes_in_flight) {
  if (config_.rate_based_recovery ||
      (config_.rate_based_startup && mode_ == STARTUP)) {
    return;
  }

  if (recovery_state_ == NOT_IN_RECOVERY) {
    return;
  }

  // Set up the initial recovery window.
  if (recovery_window_.IsZero()) {
    recovery_window_ = bytes_in_flight + bytes_acked;
    recovery_window_ = std::max(kMinimumCongestionWindow, recovery_window_);
    return;
  }

  // Remove losses from the recovery window, while accounting for a potential
  // integer underflow.
  recovery_window_ = recovery_window_ >= bytes_lost
                         ? recovery_window_ - bytes_lost
                         : kMaxSegmentSize;

  // In CONSERVATION mode, just subtracting losses is sufficient.  In GROWTH,
  // release additional |bytes_acked| to achieve a slow-start-like behavior.
  // In MEDIUM_GROWTH, release |bytes_acked| / 2 to split the difference.
  if (recovery_state_ == GROWTH) {
    recovery_window_ += bytes_acked;
  } else if (recovery_state_ == MEDIUM_GROWTH) {
    recovery_window_ += bytes_acked / 2;
  }

  // Sanity checks.  Ensure that we always allow to send at leastś
  // |bytes_acked| in response.
  recovery_window_ = std::max(recovery_window_, bytes_in_flight + bytes_acked);
  recovery_window_ = std::max(kMinimumCongestionWindow, recovery_window_);
}

void BbrNetworkController::OnApplicationLimited(DataSize bytes_in_flight) {
  if (bytes_in_flight >= GetCongestionWindow()) {
    return;
  }

  app_limited_since_last_probe_rtt_ = true;

  is_app_limited_ = true;
  end_of_app_limited_phase_ = last_send_time_;

  RTC_LOG(LS_INFO) << "Becoming application limited. Last sent time: "
                   << ToString(last_send_time_)
                   << ", CWND: " << ToString(GetCongestionWindow());
}
}  // namespace bbr
}  // namespace webrtc
