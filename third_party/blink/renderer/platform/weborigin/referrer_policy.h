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
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_REFERRER_POLICY_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_REFERRER_POLICY_H_

namespace blink {

enum ReferrerPolicy : uint8_t {
  // https://w3c.github.io/webappsec/specs/referrer-policy/#referrer-policy-state-unsafe-url
  kReferrerPolicyAlways,
  // The default policy, if no policy is explicitly set by the page.
  kReferrerPolicyDefault,
  // https://w3c.github.io/webappsec/specs/referrer-policy/#referrer-policy-state-no-referrer-when-downgrade
  kReferrerPolicyNoReferrerWhenDowngrade,
  // https://w3c.github.io/webappsec/specs/referrer-policy/#referrer-policy-state-no-referrer
  kReferrerPolicyNever,
  // https://w3c.github.io/webappsec/specs/referrer-policy/#referrer-policy-state-origin
  kReferrerPolicyOrigin,
  // https://w3c.github.io/webappsec/specs/referrer-policy/#referrer-policy-state-origin-when-cross-origin
  kReferrerPolicyOriginWhenCrossOrigin,
  // https://w3c.github.io/webappsec-referrer-policy/#referrer-policy-strict-origin-when-cross-origin
  // Also used as the default policy when reduced-referrer-granularity is
  // enabled (not spec conformant).
  kReferrerPolicyStrictOriginWhenCrossOrigin,
  // https://w3c.github.io/webappsec-referrer-policy/#referrer-policy-same-origin
  kReferrerPolicySameOrigin,
  // https://w3c.github.io/webappsec-referrer-policy/#referrer-policy-strict-origin
  kReferrerPolicyStrictOrigin,
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_REFERRER_POLICY_H_
