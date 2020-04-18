// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/webrtc_dummy_video_capturer.h"

namespace remoting {
namespace protocol {

WebrtcDummyVideoCapturer::WebrtcDummyVideoCapturer() = default;
WebrtcDummyVideoCapturer::~WebrtcDummyVideoCapturer() = default;

cricket::CaptureState WebrtcDummyVideoCapturer::Start(
    const cricket::VideoFormat& capture_format) {
  return cricket::CS_RUNNING;
}

void WebrtcDummyVideoCapturer::Stop() {
  SetCaptureState(cricket::CS_STOPPED);
}

bool WebrtcDummyVideoCapturer::IsRunning() {
  return true;
}

bool WebrtcDummyVideoCapturer::IsScreencast() const {
  return true;
}

bool WebrtcDummyVideoCapturer::GetPreferredFourccs(
    std::vector<uint32_t>* fourccs) {
  return true;
}

}  // namespace protocol
}  // namespace remoting
