// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "content/renderer/media/webrtc/rtc_event_log_output_sink_proxy.h"

namespace content {

RtcEventLogOutputSinkProxy::RtcEventLogOutputSinkProxy(
    RtcEventLogOutputSink* sink)
    : sink_(sink) {
  CHECK(sink_);
}

RtcEventLogOutputSinkProxy::~RtcEventLogOutputSinkProxy() = default;

bool RtcEventLogOutputSinkProxy::IsActive() const {
  return true;  // Active until the proxy is destroyed.
}

bool RtcEventLogOutputSinkProxy::Write(const std::string& output) {
  sink_->OnWebRtcEventLogWrite(output);
  return true;
}

}  // namespace content
