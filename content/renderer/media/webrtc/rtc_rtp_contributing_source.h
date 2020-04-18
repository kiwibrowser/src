// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_RTC_RTP_CONTRIBUTING_SOURCE_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_RTC_RTP_CONTRIBUTING_SOURCE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/web_rtc_rtp_contributing_source.h"
#include "third_party/webrtc/api/rtpreceiverinterface.h"

namespace content {

class CONTENT_EXPORT RTCRtpContributingSource
    : public blink::WebRTCRtpContributingSource {
 public:
  explicit RTCRtpContributingSource(const webrtc::RtpSource& source);
  ~RTCRtpContributingSource() override;

  blink::WebRTCRtpContributingSourceType SourceType() const override;
  double TimestampMs() const override;
  uint32_t Source() const override;

 private:
  const webrtc::RtpSource source_;

  DISALLOW_COPY_AND_ASSIGN(RTCRtpContributingSource);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_RTC_RTP_CONTRIBUTING_SOURCE_H_
