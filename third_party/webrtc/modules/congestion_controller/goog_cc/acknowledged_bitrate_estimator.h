/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_CONGESTION_CONTROLLER_GOOG_CC_ACKNOWLEDGED_BITRATE_ESTIMATOR_H_
#define MODULES_CONGESTION_CONTROLLER_GOOG_CC_ACKNOWLEDGED_BITRATE_ESTIMATOR_H_

#include <memory>
#include <vector>

#include "api/optional.h"
#include "modules/congestion_controller/goog_cc/bitrate_estimator.h"

namespace webrtc {

struct PacketFeedback;

namespace webrtc_cc {

class AcknowledgedBitrateEstimator {
 public:
  explicit AcknowledgedBitrateEstimator(
      std::unique_ptr<BitrateEstimator> bitrate_estimator);

  AcknowledgedBitrateEstimator();
  ~AcknowledgedBitrateEstimator();

  void IncomingPacketFeedbackVector(
      const std::vector<PacketFeedback>& packet_feedback_vector);
  rtc::Optional<uint32_t> bitrate_bps() const;
  void SetAlrEndedTimeMs(int64_t alr_ended_time_ms);

 private:
  void MaybeExpectFastRateChange(int64_t packet_arrival_time_ms);
  rtc::Optional<int64_t> alr_ended_time_ms_;
  std::unique_ptr<BitrateEstimator> bitrate_estimator_;
};

}  // namespace webrtc_cc
}  // namespace webrtc

#endif  // MODULES_CONGESTION_CONTROLLER_GOOG_CC_ACKNOWLEDGED_BITRATE_ESTIMATOR_H_
