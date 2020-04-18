// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/rtc_rtp_contributing_source.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "third_party/webrtc/rtc_base/scoped_ref_ptr.h"

namespace content {

RTCRtpContributingSource::RTCRtpContributingSource(
    const webrtc::RtpSource& source)
    : source_(source) {}

RTCRtpContributingSource::~RTCRtpContributingSource() {}

blink::WebRTCRtpContributingSourceType RTCRtpContributingSource::SourceType()
    const {
  switch (source_.source_type()) {
    case webrtc::RtpSourceType::SSRC:
      return blink::WebRTCRtpContributingSourceType::SSRC;
    case webrtc::RtpSourceType::CSRC:
      return blink::WebRTCRtpContributingSourceType::CSRC;
    default:
      NOTREACHED();
      return blink::WebRTCRtpContributingSourceType::SSRC;
  }
}

double RTCRtpContributingSource::TimestampMs() const {
  return source_.timestamp_ms();
}

uint32_t RTCRtpContributingSource::Source() const {
  return source_.source_id();
}

}  // namespace content
