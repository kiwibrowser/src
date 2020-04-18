/*
 * Copyright (C) 2015 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#include "third_party/blink/renderer/modules/peerconnection/rtc_certificate.h"

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/platform/bindings/to_v8.h"
#include "third_party/blink/renderer/platform/bindings/v8_binding.h"

namespace blink {

RTCCertificate::RTCCertificate(std::unique_ptr<WebRTCCertificate> certificate)
    : certificate_(base::WrapUnique(certificate.release())) {}

std::unique_ptr<WebRTCCertificate> RTCCertificate::CertificateShallowCopy()
    const {
  return certificate_->ShallowCopy();
}

DOMTimeStamp RTCCertificate::expires() const {
  return static_cast<DOMTimeStamp>(certificate_->Expires());
}

HeapVector<RTCDtlsFingerprint> RTCCertificate::getFingerprints() {
  WebVector<WebRTCDtlsFingerprint> web_fingerprints =
      certificate_->GetFingerprints();
  DCHECK(!web_fingerprints.IsEmpty());
  HeapVector<RTCDtlsFingerprint> fingerprints(web_fingerprints.size());
  for (size_t i = 0; i < fingerprints.size(); ++i) {
    DCHECK(!web_fingerprints[i].Algorithm().IsEmpty());
    DCHECK(!web_fingerprints[i].Value().IsEmpty());
    fingerprints[i].setAlgorithm(web_fingerprints[i].Algorithm());
    fingerprints[i].setValue(web_fingerprints[i].Value());
  }
  return fingerprints;
}

}  // namespace blink
