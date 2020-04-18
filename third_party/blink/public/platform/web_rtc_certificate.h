// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_CERTIFICATE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_CERTIFICATE_H_

#include "third_party/blink/public/platform/web_vector.h"

#include "third_party/blink/public/platform/web_rtc_key_params.h"
#include "third_party/blink/public/platform/web_string.h"

#include <memory>

namespace blink {

// https://w3c.github.io/webrtc-pc/#rtcdtlsfingerprint*
class WebRTCDtlsFingerprint {
 public:
  WebRTCDtlsFingerprint(WebString algorithm, WebString value)
      : algorithm_(algorithm), value_(value) {}

  WebString Algorithm() const { return algorithm_; }
  WebString Value() const { return value_; }

 private:
  WebString algorithm_;
  WebString value_;
};

// Corresponds to |rtc::RTCCertificatePEM| in WebRTC.
// See |WebRTCCertificate::ToPEM| and |WebRTCCertificateGenerator::FromPEM|.
class WebRTCCertificatePEM {
 public:
  WebRTCCertificatePEM(WebString private_key, WebString certificate)
      : private_key_(private_key), certificate_(certificate) {}

  WebString PrivateKey() const { return private_key_; }
  WebString Certificate() const { return certificate_; }

 private:
  WebString private_key_;
  WebString certificate_;
};

// WebRTCCertificate is an interface defining what Blink needs to know about
// certificates, hiding Chromium and WebRTC layer implementation details. It is
// possible to create shallow copies of the WebRTCCertificate. When all copies
// are destroyed, the implementation specific data must be freed.
// WebRTCCertificate objects thus act as references to the reference counted
// internal data.
class WebRTCCertificate {
 public:
  WebRTCCertificate() = default;
  virtual ~WebRTCCertificate() = default;

  // Copies the WebRTCCertificate object without copying the underlying
  // implementation specific (WebRTC layer) certificate. When all copies are
  // destroyed the underlying data is freed.
  virtual std::unique_ptr<WebRTCCertificate> ShallowCopy() const = 0;

  // Returns the expiration time in ms relative to epoch, 1970-01-01T00:00:00Z.
  virtual uint64_t Expires() const = 0;
  virtual WebVector<WebRTCDtlsFingerprint> GetFingerprints() const = 0;
  // Creates a PEM strings representation of the certificate. See also
  // |WebRTCCertificateGenerator::FromPEM|.
  virtual WebRTCCertificatePEM ToPEM() const = 0;
  // Checks if the two certificate objects represent the same certificate value,
  // as should be the case for a clone and the original.
  virtual bool Equals(const WebRTCCertificate& other) const = 0;

 private:
  WebRTCCertificate(const WebRTCCertificate&) = delete;
  WebRTCCertificate& operator=(const WebRTCCertificate&) = delete;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_CERTIFICATE_H_
