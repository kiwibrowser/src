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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_ICE_CANDIDATE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_ICE_CANDIDATE_H_

#include "base/memory/ref_counted.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

class WebRTCICECandidate final : public base::RefCounted<WebRTCICECandidate> {
 public:
  REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE();

  // TODO(guidou): Support setting sdp_m_line_index to -1 to indicate the
  // absence of a value for sdp_m_line_index. crbug.com/614958
  static scoped_refptr<WebRTCICECandidate> Create(
      const WebString& candidate,
      const WebString& sdp_mid,
      unsigned short sdp_m_line_index) {
    return base::AdoptRef(
        new WebRTCICECandidate(candidate, sdp_mid, sdp_m_line_index));
  }

  const WebString& Candidate() const { return candidate_; }
  const WebString& SdpMid() const { return sdp_mid_; }
  unsigned short SdpMLineIndex() const { return sdp_m_line_index_; }
  void SetCandidate(WebString candidate) { candidate_ = std::move(candidate); }
  void SetSdpMid(WebString sdp_mid) { sdp_mid_ = std::move(sdp_mid); }
  void SetSdpMLineIndex(unsigned short sdp_m_line_index) {
    sdp_m_line_index_ = sdp_m_line_index;
  }

 private:
  friend class base::RefCounted<WebRTCICECandidate>;

  WebRTCICECandidate(const WebString& candidate,
                     const WebString& sdp_mid,
                     unsigned short sdp_m_line_index)
      : candidate_(candidate),
        sdp_mid_(sdp_mid),
        sdp_m_line_index_(sdp_m_line_index) {}

  ~WebRTCICECandidate() = default;

  WebString candidate_;
  WebString sdp_mid_;
  unsigned short sdp_m_line_index_;

  DISALLOW_COPY_AND_ASSIGN(WebRTCICECandidate);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_ICE_CANDIDATE_H_
