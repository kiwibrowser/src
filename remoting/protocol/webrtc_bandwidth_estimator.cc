// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/webrtc_bandwidth_estimator.h"

namespace remoting {
namespace protocol {

WebrtcBandwidthEstimator::WebrtcBandwidthEstimator() = default;
WebrtcBandwidthEstimator::~WebrtcBandwidthEstimator() = default;

void WebrtcBandwidthEstimator::OnBitrateEstimation(int bitrate_kbps) {
  bitrate_kbps_ = bitrate_kbps;
}

int WebrtcBandwidthEstimator::GetBitrateKbps() {
  return bitrate_kbps_;
}

}  // namespace protocol
}  // namespace remoting
