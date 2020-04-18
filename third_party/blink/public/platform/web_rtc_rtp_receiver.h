// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_RTP_RECEIVER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_RTP_RECEIVER_H_

#include <memory>
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_rtc_stats.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace blink {

class WebMediaStream;
class WebMediaStreamTrack;
class WebRTCRtpContributingSource;

// Implementations of this interface keep the corresponding WebRTC-layer
// receiver alive through reference counting. Multiple |WebRTCRtpReceiver|s
// could reference the same receiver, see |id|.
// https://w3c.github.io/webrtc-pc/#rtcrtpreceiver-interface
class BLINK_PLATFORM_EXPORT WebRTCRtpReceiver {
 public:
  virtual ~WebRTCRtpReceiver();

  // Two |WebRTCRtpReceiver|s referencing the same WebRTC-layer receiver have
  // the same |id|.
  virtual uintptr_t Id() const = 0;
  virtual const WebMediaStreamTrack& Track() const = 0;
  virtual WebVector<WebMediaStream> Streams() const = 0;
  virtual WebVector<std::unique_ptr<WebRTCRtpContributingSource>>
  GetSources() = 0;
  virtual void GetStats(std::unique_ptr<blink::WebRTCStatsReportCallback>) = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_RTP_RECEIVER_H_
