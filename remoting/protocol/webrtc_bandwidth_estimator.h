// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_WEBRTC_BANDWIDTH_ESTIMATOR_H_
#define REMOTING_PROTOCOL_WEBRTC_BANDWIDTH_ESTIMATOR_H_

#include "remoting/protocol/bandwidth_estimator.h"

namespace remoting {
namespace protocol {

// An implementation of BandwidthEstimator relying on the estimate from WebRTC.
class WebrtcBandwidthEstimator final : public BandwidthEstimator {
 public:
  WebrtcBandwidthEstimator();
  ~WebrtcBandwidthEstimator() override;

  void OnBitrateEstimation(int bitrate_kbps) override;
  int GetBitrateKbps() override;

 private:
  int bitrate_kbps_ = 0;
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_WEBRTC_BANDWIDTH_ESTIMATOR_H_
