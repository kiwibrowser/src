// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_MOCK_SSL_HOST_STATE_DELEGATE_H_
#define CONTENT_PUBLIC_TEST_MOCK_SSL_HOST_STATE_DELEGATE_H_

#include "content/public/browser/ssl_host_state_delegate.h"

namespace content {

class MockSSLHostStateDelegate : public SSLHostStateDelegate {
 public:
  MockSSLHostStateDelegate();
  ~MockSSLHostStateDelegate() override;

  void AllowCert(const std::string& host,
                 const net::X509Certificate& cert,
                 net::CertStatus error) override;

  void Clear(
      const base::Callback<bool(const std::string&)>& host_filter) override;

  CertJudgment QueryPolicy(const std::string& host,
                           const net::X509Certificate& cert,
                           net::CertStatus error,
                           bool* expired_previous_decision) override;

  void HostRanInsecureContent(const std::string& host,
                              int child_id,
                              InsecureContentType content_type) override;

  bool DidHostRunInsecureContent(
      const std::string& host,
      int child_id,
      InsecureContentType content_type) const override;

  void RevokeUserAllowExceptions(const std::string& host) override;

  bool HasAllowException(const std::string& host) const override;

 private:
  std::set<std::string> exceptions_;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_MOCK_SSL_HOST_STATE_DELEGATE_H_
