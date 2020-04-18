/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_STATS_REQUEST_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_STATS_REQUEST_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

class RTCStatsRequest;
class WebMediaStreamTrack;
class WebRTCStatsResponse;

// The WebRTCStatsRequest class represents a JavaScript call on
// RTCPeerConnection.getStats(). The user of this API will use
// the calls on this class and WebRTCStatsResponse to fill in the
// data that will be returned via a callback to the user in an
// RTCStatsResponse structure.
//
// The typical usage pattern is:
// WebRTCStatsRequest request = <from somewhere>
// WebRTCStatsResponse response = request.CreateResponse();
//
// For each item on which statistics are going to be reported:
//   WebRTCLegacyStats stats(...);
//   (configuration of stats object depends on item type)
//   response.AddStats(stats);
// When finished adding information:
// request.RequestSucceeded(response);

class WebRTCStatsRequest {
 public:
  WebRTCStatsRequest() = default;
  WebRTCStatsRequest(const WebRTCStatsRequest& other) { Assign(other); }
  ~WebRTCStatsRequest() { Reset(); }

  WebRTCStatsRequest& operator=(const WebRTCStatsRequest& other) {
    Assign(other);
    return *this;
  }

  BLINK_PLATFORM_EXPORT void Assign(const WebRTCStatsRequest&);

  BLINK_PLATFORM_EXPORT void Reset();

  // This function returns true if a selector argument was given to getStats.
  BLINK_PLATFORM_EXPORT bool HasSelector() const;

  // The Component() accessor give the information
  // required to look up a MediaStreamTrack implementation.
  // It is only useful to call it when HasSelector() returns true.
  BLINK_PLATFORM_EXPORT const WebMediaStreamTrack Component() const;

  BLINK_PLATFORM_EXPORT void RequestSucceeded(const WebRTCStatsResponse&) const;

  BLINK_PLATFORM_EXPORT WebRTCStatsResponse CreateResponse() const;

#if INSIDE_BLINK
  BLINK_PLATFORM_EXPORT WebRTCStatsRequest(RTCStatsRequest*);
#endif

 private:
  WebPrivatePtr<RTCStatsRequest, kWebPrivatePtrDestructionCrossThread> private_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_STATS_REQUEST_H_
