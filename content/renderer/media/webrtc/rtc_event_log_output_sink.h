// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_RTC_EVENT_LOG_OUTPUT_SINK_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_RTC_EVENT_LOG_OUTPUT_SINK_H_

#include <string>

namespace content {

class RtcEventLogOutputSink {
 public:
  virtual ~RtcEventLogOutputSink() = default;

  virtual void OnWebRtcEventLogWrite(const std::string& output) = 0;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_RTC_EVENT_LOG_OUTPUT_SINK_H_
