/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video/encoder_key_frame_callback.h"

#include "rtc_base/checks.h"

static const int kMinKeyFrameRequestIntervalMs = 300;

namespace webrtc {

EncoderKeyFrameCallback::EncoderKeyFrameCallback(
    Clock* clock,
    const std::vector<uint32_t>& ssrcs,
    VideoStreamEncoderInterface* encoder)
    : clock_(clock),
      ssrcs_(ssrcs),
      video_stream_encoder_(encoder),
      time_last_intra_request_ms_(-1) {
  RTC_DCHECK(!ssrcs.empty());
}

bool EncoderKeyFrameCallback::HasSsrc(uint32_t ssrc) {
  for (uint32_t registered_ssrc : ssrcs_) {
    if (registered_ssrc == ssrc) {
      return true;
    }
  }
  return false;
}

void EncoderKeyFrameCallback::OnReceivedIntraFrameRequest(uint32_t ssrc) {
  RTC_DCHECK(HasSsrc(ssrc));
  {
    int64_t now_ms = clock_->TimeInMilliseconds();
    rtc::CritScope lock(&crit_);
    if (time_last_intra_request_ms_ + kMinKeyFrameRequestIntervalMs > now_ms) {
      return;
    }
    time_last_intra_request_ms_ = now_ms;
  }

  // Always produce key frame for all streams.
  video_stream_encoder_->SendKeyFrame();
}

void EncoderKeyFrameCallback::OnKeyFrameRequested(uint64_t channel_id) {
  if (channel_id != ssrcs_[0]) {
    RTC_LOG(LS_INFO) << "Key frame request on unknown channel id " << channel_id
                     << " expected " << ssrcs_[0];
    return;
  }

  video_stream_encoder_->SendKeyFrame();
}

}  // namespace webrtc
