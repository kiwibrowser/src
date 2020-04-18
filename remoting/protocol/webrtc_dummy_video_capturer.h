// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_WEBRTC_DUMMY_VIDEO_CAPTURER_H_
#define REMOTING_PROTOCOL_WEBRTC_DUMMY_VIDEO_CAPTURER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "third_party/webrtc/media/base/videocapturer.h"

namespace remoting {
namespace protocol {

// A dummy video capturer needed to create peer connection. We do not supply
// captured frames through this interface, but instead provide encoded
// frames to Webrtc. We expect this requirement to go away once we have
// proper support for providing encoded frames to Webrtc through
// VideoSourceInterface
class WebrtcDummyVideoCapturer : public cricket::VideoCapturer {
 public:
  explicit WebrtcDummyVideoCapturer();
  ~WebrtcDummyVideoCapturer() override;

  // cricket::VideoCapturer interface.
  cricket::CaptureState Start(
      const cricket::VideoFormat& capture_format) override;
  void Stop() override;
  bool IsRunning() override;
  bool IsScreencast() const override;
  bool GetPreferredFourccs(std::vector<uint32_t>* fourccs) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebrtcDummyVideoCapturer);
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_WEBRTC_DUMMY_VIDEO_CAPTURER_H_
