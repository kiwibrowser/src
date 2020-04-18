// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_RTC_EVENT_LOG_OUTPUT_SINK_PROXY_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_RTC_EVENT_LOG_OUTPUT_SINK_PROXY_H_

#include "content/renderer/media/webrtc/rtc_event_log_output_sink.h"
#include "third_party/webrtc/api/rtceventlogoutput.h"

namespace content {

class RtcEventLogOutputSinkProxy final : public webrtc::RtcEventLogOutput {
 public:
  RtcEventLogOutputSinkProxy(RtcEventLogOutputSink* sink);
  ~RtcEventLogOutputSinkProxy() override;

  bool IsActive() const override;

  bool Write(const std::string& output) override;

 private:
  RtcEventLogOutputSink* const sink_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_RTC_EVENT_LOG_OUTPUT_SINK_PROXY_H_
