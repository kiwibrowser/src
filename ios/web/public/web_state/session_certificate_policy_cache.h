// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_WEB_STATE_SESSION_CERTIFICATE_POLICY_CACHE_H_
#define IOS_WEB_PUBLIC_WEB_STATE_SESSION_CERTIFICATE_POLICY_CACHE_H_

#include "base/memory/ref_counted.h"
#include "net/cert/cert_status_flags.h"

namespace net {
class X509Certificate;
}

namespace web {

class CertificatePolicyCache;

// Stores certificate policy decisions for a specific session.  The certificate
// policy decisions stored in this object are persisted along with their
// WebStates and are used to populate the CertificatePolicyCache of a restored
// BrowserState.  Must be accessed on the UI thread.
class SessionCertificatePolicyCache {
 public:
  SessionCertificatePolicyCache() {}
  virtual ~SessionCertificatePolicyCache() {}

  // Stores certificate information that a user has indicated should be allowed
  // for this session.
  virtual void RegisterAllowedCertificate(
      const scoped_refptr<net::X509Certificate> certificate,
      const std::string& host,
      net::CertStatus status) = 0;

  // Removes all previously allowed certificates.
  virtual void ClearAllowedCertificates() = 0;

  // Transfers the allowed certificate information from this session to |cache|.
  virtual void UpdateCertificatePolicyCache(
      const scoped_refptr<web::CertificatePolicyCache>& cache) const = 0;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_WEB_STATE_SESSION_CERTIFICATE_POLICY_CACHE_H_
