/*
 * Copyright (C) 2015 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_KEY_PARAMS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_KEY_PARAMS_H_

#include "base/logging.h"
#include "third_party/blink/public/platform/web_common.h"

namespace blink {

// Corresponds to rtc::KeyType in WebRTC.
enum WebRTCKeyType {
  kWebRTCKeyTypeRSA,
  kWebRTCKeyTypeECDSA,
  kWebRTCKeyTypeNull
};

// Corresponds to rtc::RSAParams in WebRTC.
struct WebRTCRSAParams {
  unsigned mod_length;
  unsigned pub_exp;
};

// Corresponds to rtc::ECCurve in WebRTC.
enum WebRTCECCurve { kWebRTCECCurveNistP256 };

// Corresponds to rtc::KeyParams in WebRTC.
class WebRTCKeyParams {
 public:
  static WebRTCKeyParams CreateRSA(unsigned mod_length, unsigned pub_exp) {
    WebRTCKeyParams key_params(kWebRTCKeyTypeRSA);
    key_params.params_.rsa.mod_length = mod_length;
    key_params.params_.rsa.pub_exp = pub_exp;
    return key_params;
  }
  static WebRTCKeyParams CreateECDSA(WebRTCECCurve curve) {
    WebRTCKeyParams key_params(kWebRTCKeyTypeECDSA);
    key_params.params_.ec_curve = curve;
    return key_params;
  }

  WebRTCKeyParams() : WebRTCKeyParams(kWebRTCKeyTypeNull) {}

  WebRTCKeyType KeyType() const { return key_type_; }
  WebRTCRSAParams RsaParams() const {
    DCHECK_EQ(key_type_, kWebRTCKeyTypeRSA);
    return params_.rsa;
  }
  WebRTCECCurve EcCurve() const {
    DCHECK_EQ(key_type_, kWebRTCKeyTypeECDSA);
    return params_.ec_curve;
  }

 private:
  WebRTCKeyParams(WebRTCKeyType key_type) : key_type_(key_type) {}

  WebRTCKeyType key_type_;
  union {
    WebRTCRSAParams rsa;
    WebRTCECCurve ec_curve;
  } params_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_KEY_PARAMS_H_
