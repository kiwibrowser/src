/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/congestion_controller/goog_cc/include/goog_cc_factory.h"

#include "modules/congestion_controller/goog_cc/goog_cc_network_control.h"
#include "rtc_base/ptr_util.h"
namespace webrtc {
GoogCcNetworkControllerFactory::GoogCcNetworkControllerFactory(
    RtcEventLog* event_log)
    : event_log_(event_log) {}

std::unique_ptr<NetworkControllerInterface>
GoogCcNetworkControllerFactory::Create(NetworkControllerConfig config) {
  return rtc::MakeUnique<webrtc_cc::GoogCcNetworkController>(event_log_,
                                                             config);
}

TimeDelta GoogCcNetworkControllerFactory::GetProcessInterval() const {
  const int64_t kUpdateIntervalMs = 25;
  return TimeDelta::ms(kUpdateIntervalMs);
}
}  // namespace webrtc
