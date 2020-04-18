// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_RTC_CERTIFICATE_GENERATOR_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_RTC_CERTIFICATE_GENERATOR_H_

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/web_rtc_certificate.h"
#include "third_party/blink/public/platform/web_rtc_certificate_generator.h"
#include "third_party/blink/public/platform/web_rtc_key_params.h"
#include "third_party/webrtc/api/optional.h"

namespace content {

// Chromium's WebRTCCertificateGenerator implementation; uses the
// PeerConnectionIdentityStore/SSLIdentity::Generate to generate the identity,
// rtc::RTCCertificate and content::RTCCertificate.
class RTCCertificateGenerator : public blink::WebRTCCertificateGenerator {
 public:
  RTCCertificateGenerator() {}
  ~RTCCertificateGenerator() override {}

  // blink::WebRTCCertificateGenerator implementation.
  void GenerateCertificate(
      const blink::WebRTCKeyParams& key_params,
      std::unique_ptr<blink::WebRTCCertificateCallback> observer,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override;
  void GenerateCertificateWithExpiration(
      const blink::WebRTCKeyParams& key_params,
      uint64_t expires_ms,
      std::unique_ptr<blink::WebRTCCertificateCallback> observer,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override;
  bool IsSupportedKeyParams(const blink::WebRTCKeyParams& key_params) override;
  std::unique_ptr<blink::WebRTCCertificate> FromPEM(
      blink::WebString pem_private_key,
      blink::WebString pem_certificate) override;

 private:
  void generateCertificateWithOptionalExpiration(
      const blink::WebRTCKeyParams& key_params,
      const rtc::Optional<uint64_t>& expires_ms,
      std::unique_ptr<blink::WebRTCCertificateCallback> observer,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  DISALLOW_COPY_AND_ASSIGN(RTCCertificateGenerator);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_RTC_CERTIFICATE_GENERATOR_H_
