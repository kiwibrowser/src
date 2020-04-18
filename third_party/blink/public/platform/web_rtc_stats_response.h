/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_STATS_RESPONSE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_STATS_RESPONSE_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"
#include "third_party/blink/public/platform/web_rtc_legacy_stats.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

class RTCStatsResponseBase;

class WebRTCStatsResponse {
 public:
  WebRTCStatsResponse(const WebRTCStatsResponse& other) { Assign(other); }
  WebRTCStatsResponse() = default;
  ~WebRTCStatsResponse() { Reset(); }

  WebRTCStatsResponse& operator=(const WebRTCStatsResponse& other) {
    Assign(other);
    return *this;
  }
  BLINK_PLATFORM_EXPORT void Assign(const WebRTCStatsResponse&);
  BLINK_PLATFORM_EXPORT void Reset();

  BLINK_PLATFORM_EXPORT void AddStats(const WebRTCLegacyStats&);

#if INSIDE_BLINK
  BLINK_PLATFORM_EXPORT WebRTCStatsResponse(RTCStatsResponseBase*);
  BLINK_PLATFORM_EXPORT operator RTCStatsResponseBase*() const;
#endif

 private:
  WebPrivatePtr<RTCStatsResponseBase> private_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_STATS_RESPONSE_H_
