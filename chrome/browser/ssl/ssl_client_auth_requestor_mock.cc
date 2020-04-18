// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_client_auth_requestor_mock.h"

#include "base/macros.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/url_request/url_request.h"

namespace {

class FakeClientCertificateDelegate
    : public content::ClientCertificateDelegate {
 public:
  explicit FakeClientCertificateDelegate(SSLClientAuthRequestorMock* requestor)
      : requestor_(requestor) {}

  ~FakeClientCertificateDelegate() override {
    if (requestor_)
      requestor_->CancelCertificateSelection();
  }

  // content::ClientCertificateDelegate implementation:
  void ContinueWithCertificate(scoped_refptr<net::X509Certificate> cert,
                               scoped_refptr<net::SSLPrivateKey> key) override {
    requestor_->CertificateSelected(cert.get(), key.get());
    requestor_ = nullptr;
  }

 private:
  scoped_refptr<SSLClientAuthRequestorMock> requestor_;

  DISALLOW_COPY_AND_ASSIGN(FakeClientCertificateDelegate);
};

}  // namespace

SSLClientAuthRequestorMock::SSLClientAuthRequestorMock(
    net::URLRequest* request,
    const scoped_refptr<net::SSLCertRequestInfo>& cert_request_info)
    : cert_request_info_(cert_request_info) {
}

SSLClientAuthRequestorMock::~SSLClientAuthRequestorMock() {}

std::unique_ptr<content::ClientCertificateDelegate>
SSLClientAuthRequestorMock::CreateDelegate() {
  return std::make_unique<FakeClientCertificateDelegate>(this);
}
