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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_CERTIFICATE_GENERATOR_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_CERTIFICATE_GENERATOR_H_

#include "third_party/blink/public/platform/web_callbacks.h"
#include "third_party/blink/public/platform/web_rtc_certificate.h"
#include "third_party/blink/public/platform/web_rtc_key_params.h"
#include "third_party/blink/public/platform/web_string.h"

#include <memory>

namespace base {
class SingleThreadTaskRunner;
}

namespace blink {

using WebRTCCertificateCallback =
    WebCallbacks<std::unique_ptr<WebRTCCertificate>, void>;

// Interface defining a class that can generate WebRTCCertificates
// asynchronously.
class WebRTCCertificateGenerator {
 public:
  virtual ~WebRTCCertificateGenerator() = default;

  // Start generating a certificate asynchronously. |observer| is invoked on the
  // same thread that called generateCertificate when the operation is
  // completed.
  virtual void GenerateCertificate(
      const WebRTCKeyParams&,
      std::unique_ptr<WebRTCCertificateCallback> observer,
      scoped_refptr<base::SingleThreadTaskRunner>) = 0;
  virtual void GenerateCertificateWithExpiration(
      const WebRTCKeyParams&,
      uint64_t expires_ms,
      std::unique_ptr<WebRTCCertificateCallback> observer,
      scoped_refptr<base::SingleThreadTaskRunner>) = 0;

  // Determines if the parameters are supported by |GenerateCertificate|.
  // For example, if the number of bits of some parameter is too small or too
  // large we may want to reject it for security or performance reasons.
  virtual bool IsSupportedKeyParams(const WebRTCKeyParams&) = 0;

  // Creates a certificate from the PEM strings. See also
  // |WebRTCCertificate::ToPEM|.
  virtual std::unique_ptr<WebRTCCertificate> FromPEM(
      blink::WebString pem_private_key,
      blink::WebString pem_certificate) = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_CERTIFICATE_GENERATOR_H_
