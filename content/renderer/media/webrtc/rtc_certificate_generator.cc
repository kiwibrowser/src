// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/rtc_certificate_generator.h"

#include <string>
#include <utility>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/renderer/media/webrtc/peer_connection_dependency_factory.h"
#include "content/renderer/media/webrtc/rtc_certificate.h"
#include "content/renderer/render_thread_impl.h"
#include "media/media_buildflags.h"
#include "third_party/webrtc/rtc_base/rtccertificate.h"
#include "third_party/webrtc/rtc_base/rtccertificategenerator.h"
#include "third_party/webrtc/rtc_base/scoped_ref_ptr.h"
#include "url/gurl.h"

namespace content {
namespace {

rtc::KeyParams WebRTCKeyParamsToKeyParams(
    const blink::WebRTCKeyParams& key_params) {
  switch (key_params.KeyType()) {
    case blink::kWebRTCKeyTypeRSA:
      return rtc::KeyParams::RSA(key_params.RsaParams().mod_length,
                                 key_params.RsaParams().pub_exp);
    case blink::kWebRTCKeyTypeECDSA:
      return rtc::KeyParams::ECDSA(
          static_cast<rtc::ECCurve>(key_params.EcCurve()));
    default:
      NOTREACHED();
      return rtc::KeyParams();
  }
}

// A certificate generation request spawned by
// |RTCCertificateGenerator::generateCertificateWithOptionalExpiration|. This
// is handled by a separate class so that reference counting can keep the
// request alive independently of the |RTCCertificateGenerator| that spawned it.
class RTCCertificateGeneratorRequest
    : public base::RefCountedThreadSafe<RTCCertificateGeneratorRequest> {
 private:
  using CertificateCallbackPtr = std::unique_ptr<
     blink::WebRTCCertificateCallback,
     base::OnTaskRunnerDeleter>;
 public:
  RTCCertificateGeneratorRequest(
      const scoped_refptr<base::SingleThreadTaskRunner>& main_thread,
      const scoped_refptr<base::SingleThreadTaskRunner>& worker_thread)
      : main_thread_(main_thread),
        worker_thread_(worker_thread) {
    DCHECK(main_thread_);
    DCHECK(worker_thread_);
  }

  void GenerateCertificateAsync(
      const blink::WebRTCKeyParams& key_params,
      const rtc::Optional<uint64_t>& expires_ms,
      std::unique_ptr<blink::WebRTCCertificateCallback> observer) {
    DCHECK(main_thread_->BelongsToCurrentThread());
    DCHECK(observer);

    CertificateCallbackPtr transition(observer.release(),
                                      base::OnTaskRunnerDeleter(main_thread_));
    worker_thread_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &RTCCertificateGeneratorRequest::GenerateCertificateOnWorkerThread,
            this, key_params, expires_ms, std::move(transition)));
  }

 private:
  friend class base::RefCountedThreadSafe<RTCCertificateGeneratorRequest>;
  ~RTCCertificateGeneratorRequest() {}

  void GenerateCertificateOnWorkerThread(
      const blink::WebRTCKeyParams key_params,
      const rtc::Optional<uint64_t> expires_ms,
      CertificateCallbackPtr observer) {
    DCHECK(worker_thread_->BelongsToCurrentThread());

    rtc::scoped_refptr<rtc::RTCCertificate> certificate =
        rtc::RTCCertificateGenerator::GenerateCertificate(
            WebRTCKeyParamsToKeyParams(key_params), expires_ms);

    main_thread_->PostTask(
        FROM_HERE,
        base::BindOnce(&RTCCertificateGeneratorRequest::DoCallbackOnMainThread,
                       this, std::move(observer),
                       certificate
                           ? std::make_unique<RTCCertificate>(certificate)
                           : nullptr));
  }

  void DoCallbackOnMainThread(
      CertificateCallbackPtr observer,
      std::unique_ptr<blink::WebRTCCertificate> certificate) {
    DCHECK(main_thread_->BelongsToCurrentThread());
    DCHECK(observer);
    if (certificate)
      observer->OnSuccess(std::move(certificate));
    else
      observer->OnError();
  }

  // The main thread is the renderer thread.
  const scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  // The WebRTC worker thread.
  const scoped_refptr<base::SingleThreadTaskRunner> worker_thread_;
};

}  // namespace

void RTCCertificateGenerator::GenerateCertificate(
    const blink::WebRTCKeyParams& key_params,
    std::unique_ptr<blink::WebRTCCertificateCallback> observer,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  generateCertificateWithOptionalExpiration(
      key_params, rtc::Optional<uint64_t>(), std::move(observer), task_runner);
}

void RTCCertificateGenerator::GenerateCertificateWithExpiration(
    const blink::WebRTCKeyParams& key_params,
    uint64_t expires_ms,
    std::unique_ptr<blink::WebRTCCertificateCallback> observer,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  generateCertificateWithOptionalExpiration(key_params,
                                            rtc::Optional<uint64_t>(expires_ms),
                                            std::move(observer), task_runner);
}

void RTCCertificateGenerator::generateCertificateWithOptionalExpiration(
    const blink::WebRTCKeyParams& key_params,
    const rtc::Optional<uint64_t>& expires_ms,
    std::unique_ptr<blink::WebRTCCertificateCallback> observer,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  DCHECK(IsSupportedKeyParams(key_params));
  PeerConnectionDependencyFactory* pc_dependency_factory =
      RenderThreadImpl::current()->GetPeerConnectionDependencyFactory();
  pc_dependency_factory->EnsureInitialized();

  scoped_refptr<RTCCertificateGeneratorRequest> request =
      new RTCCertificateGeneratorRequest(
          task_runner, pc_dependency_factory->GetWebRtcWorkerThread());
  request->GenerateCertificateAsync(
      key_params, expires_ms, std::move(observer));
}

bool RTCCertificateGenerator::IsSupportedKeyParams(
    const blink::WebRTCKeyParams& key_params) {
  return WebRTCKeyParamsToKeyParams(key_params).IsValid();
}

std::unique_ptr<blink::WebRTCCertificate> RTCCertificateGenerator::FromPEM(
    blink::WebString pem_private_key,
    blink::WebString pem_certificate) {
  rtc::scoped_refptr<rtc::RTCCertificate> certificate =
      rtc::RTCCertificate::FromPEM(rtc::RTCCertificatePEM(
          pem_private_key.Utf8(), pem_certificate.Utf8()));
  if (!certificate)
    return nullptr;
  return std::make_unique<RTCCertificate>(certificate);
}

}  // namespace content
