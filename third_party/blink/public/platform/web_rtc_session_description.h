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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_SESSION_DESCRIPTION_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_SESSION_DESCRIPTION_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

class WebRTCSessionDescriptionPrivate;

//  In order to establish the media plane, PeerConnection needs specific
//  parameters to indicate what to transmit to the remote side, as well
//  as how to handle the media that is received. These parameters are
//  determined by the exchange of session descriptions in offers and
//  answers, and there are certain details to this process that must be
//  handled in the JSEP APIs.
//
//  Whether a session description was sent or received affects the
//  meaning of that description. For example, the list of codecs sent to
//  a remote party indicates what the local side is willing to decode,
//  and what the remote party should send.

class WebRTCSessionDescription {
 public:
  WebRTCSessionDescription() = default;
  WebRTCSessionDescription(const WebRTCSessionDescription& other) {
    Assign(other);
  }
  ~WebRTCSessionDescription() { Reset(); }

  WebRTCSessionDescription& operator=(const WebRTCSessionDescription& other) {
    Assign(other);
    return *this;
  }

  BLINK_PLATFORM_EXPORT void Assign(const WebRTCSessionDescription&);

  BLINK_PLATFORM_EXPORT void Initialize(const WebString& type,
                                        const WebString& sdp);
  BLINK_PLATFORM_EXPORT void Reset();
  bool IsNull() const { return private_.IsNull(); }

  BLINK_PLATFORM_EXPORT WebString GetType() const;
  BLINK_PLATFORM_EXPORT void SetType(const WebString&);
  BLINK_PLATFORM_EXPORT WebString Sdp() const;
  BLINK_PLATFORM_EXPORT void SetSDP(const WebString&);

#if INSIDE_BLINK
  WebRTCSessionDescription(WebString type, WebString sdp) {
    this->Initialize(type, sdp);
  }
#endif

 private:
  WebPrivatePtr<WebRTCSessionDescriptionPrivate> private_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_SESSION_DESCRIPTION_H_
