/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef VIDEO_ENCODER_KEY_FRAME_CALLBACK_H_
#define VIDEO_ENCODER_KEY_FRAME_CALLBACK_H_

#include <vector>

#include "api/media_transport_interface.h"
#include "api/video/video_stream_encoder_interface.h"
#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "rtc_base/critical_section.h"
#include "system_wrappers/include/clock.h"

namespace webrtc {

class VideoStreamEncoderInterface;

// This class receives keyframe requests from either Mediatransport or the
// RtpRtcp module.
// TODO(bugs.webrtc.org/9719): Should be eliminated when RtpMediaTransport is
// implemented.
class EncoderKeyFrameCallback : public RtcpIntraFrameObserver,
                                public MediaTransportKeyFrameRequestCallback {
 public:
  EncoderKeyFrameCallback(Clock* clock,
                          const std::vector<uint32_t>& ssrcs,
                          VideoStreamEncoderInterface* encoder);
  void OnReceivedIntraFrameRequest(uint32_t ssrc) override;

  // Implements MediaTransportKeyFrameRequestCallback
  void OnKeyFrameRequested(uint64_t channel_id) override;

 private:
  bool HasSsrc(uint32_t ssrc);

  Clock* const clock_;
  const std::vector<uint32_t> ssrcs_;
  VideoStreamEncoderInterface* const video_stream_encoder_;

  rtc::CriticalSection crit_;
  int64_t time_last_intra_request_ms_ RTC_GUARDED_BY(crit_);
};

}  // namespace webrtc

#endif  // VIDEO_ENCODER_KEY_FRAME_CALLBACK_H_
