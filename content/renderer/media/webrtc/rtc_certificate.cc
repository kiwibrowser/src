// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/rtc_certificate.h"

#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "third_party/webrtc/rtc_base/sslidentity.h"
#include "url/gurl.h"

namespace content {

RTCCertificate::RTCCertificate(
    const rtc::scoped_refptr<rtc::RTCCertificate>& certificate)
    : certificate_(certificate) {
  DCHECK(certificate_);
}

RTCCertificate::~RTCCertificate() {
}

std::unique_ptr<blink::WebRTCCertificate> RTCCertificate::ShallowCopy() const {
  return base::WrapUnique(new RTCCertificate(certificate_));
}

uint64_t RTCCertificate::Expires() const {
  return certificate_->Expires();
}

blink::WebVector<blink::WebRTCDtlsFingerprint> RTCCertificate::GetFingerprints()
    const {
  std::vector<blink::WebRTCDtlsFingerprint> fingerprints;
  std::unique_ptr<rtc::SSLCertificateStats> first_certificate_stats =
      certificate_->identity()->certificate().GetStats();
  for (rtc::SSLCertificateStats* certificate_stats =
           first_certificate_stats.get();
       certificate_stats; certificate_stats = certificate_stats->issuer.get()) {
    fingerprints.push_back(blink::WebRTCDtlsFingerprint(
        blink::WebString::FromUTF8(certificate_stats->fingerprint_algorithm),
        blink::WebString::FromUTF8(
            base::ToLowerASCII(certificate_stats->fingerprint))));
  }
  return blink::WebVector<blink::WebRTCDtlsFingerprint>(fingerprints);
}

blink::WebRTCCertificatePEM RTCCertificate::ToPEM() const {
  rtc::RTCCertificatePEM pem = certificate_->ToPEM();
  return blink::WebRTCCertificatePEM(
      blink::WebString::FromUTF8(pem.private_key()),
      blink::WebString::FromUTF8(pem.certificate()));
}

bool RTCCertificate::Equals(const blink::WebRTCCertificate& other) const {
  return *certificate_ ==
         *static_cast<const RTCCertificate&>(other).certificate_;
}

const rtc::scoped_refptr<rtc::RTCCertificate>&
RTCCertificate::rtcCertificate() const {
  return certificate_;
}

}  // namespace content
