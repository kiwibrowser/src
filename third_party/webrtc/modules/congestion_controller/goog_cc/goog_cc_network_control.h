/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_CONGESTION_CONTROLLER_GOOG_CC_GOOG_CC_NETWORK_CONTROL_H_
#define MODULES_CONGESTION_CONTROLLER_GOOG_CC_GOOG_CC_NETWORK_CONTROL_H_

#include <stdint.h>
#include <deque>
#include <memory>
#include <vector>

#include "api/optional.h"
#include "api/transport/network_control.h"
#include "logging/rtc_event_log/rtc_event_log.h"
#include "modules/bitrate_controller/send_side_bandwidth_estimation.h"
#include "modules/congestion_controller/goog_cc/acknowledged_bitrate_estimator.h"
#include "modules/congestion_controller/goog_cc/alr_detector.h"
#include "modules/congestion_controller/goog_cc/delay_based_bwe.h"
#include "modules/congestion_controller/goog_cc/probe_controller.h"
#include "rtc_base/constructormagic.h"

namespace webrtc {
namespace webrtc_cc {

class GoogCcNetworkController : public NetworkControllerInterface {
 public:
  GoogCcNetworkController(RtcEventLog* event_log,
                          NetworkControllerConfig config);
  ~GoogCcNetworkController() override;

  // NetworkControllerInterface
  NetworkControlUpdate OnNetworkAvailability(NetworkAvailability msg) override;
  NetworkControlUpdate OnNetworkRouteChange(NetworkRouteChange msg) override;
  NetworkControlUpdate OnProcessInterval(ProcessInterval msg) override;
  NetworkControlUpdate OnRemoteBitrateReport(RemoteBitrateReport msg) override;
  NetworkControlUpdate OnRoundTripTimeUpdate(RoundTripTimeUpdate msg) override;
  NetworkControlUpdate OnSentPacket(SentPacket msg) override;
  NetworkControlUpdate OnStreamsConfig(StreamsConfig msg) override;
  NetworkControlUpdate OnTargetRateConstraints(
      TargetRateConstraints msg) override;
  NetworkControlUpdate OnTransportLossReport(TransportLossReport msg) override;
  NetworkControlUpdate OnTransportPacketsFeedback(
      TransportPacketsFeedback msg) override;

 private:
  void UpdateBitrateConstraints(TargetRateConstraints constraints,
                                rtc::Optional<DataRate> starting_rate);
  rtc::Optional<DataSize> MaybeUpdateCongestionWindow();
  NetworkControlUpdate MaybeTriggerOnNetworkChanged(Timestamp at_time);
  bool GetNetworkParameters(int32_t* estimated_bitrate_bps,
                            uint8_t* fraction_loss,
                            int64_t* rtt_ms,
                            Timestamp at_time);
  NetworkControlUpdate OnNetworkEstimate(NetworkEstimate msg);
  PacerConfig UpdatePacingRates(Timestamp at_time);

  RtcEventLog* const event_log_;

  const std::unique_ptr<ProbeController> probe_controller_;

  std::unique_ptr<SendSideBandwidthEstimation> bandwidth_estimation_;
  std::unique_ptr<AlrDetector> alr_detector_;
  std::unique_ptr<DelayBasedBwe> delay_based_bwe_;
  std::unique_ptr<AcknowledgedBitrateEstimator> acknowledged_bitrate_estimator_;

  std::deque<int64_t> feedback_rtts_;
  rtc::Optional<int64_t> min_feedback_rtt_ms_;

  DataRate last_bandwidth_;
  rtc::Optional<TargetTransferRate> last_target_rate_;

  int32_t last_estimated_bitrate_bps_ = 0;
  uint8_t last_estimated_fraction_loss_ = 0;
  int64_t last_estimated_rtt_ms_ = 0;

  double pacing_factor_;
  DataRate min_pacing_rate_;
  DataRate max_padding_rate_;
  DataRate max_total_allocated_bitrate_;

  bool in_cwnd_experiment_;
  int64_t accepted_queue_ms_;
  bool previously_in_alr = false;

  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(GoogCcNetworkController);
};

}  // namespace webrtc_cc
}  // namespace webrtc

#endif  // MODULES_CONGESTION_CONTROLLER_GOOG_CC_GOOG_CC_NETWORK_CONTROL_H_
