// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_TEMP_CERTS_CACHE_NSS_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_TEMP_CERTS_CACHE_NSS_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "net/cert/scoped_nss_types.h"

namespace policy {

// Holds NSS temporary certificates in memory as ScopedCERTCertificates, making
// them available e.g. for client certificate discovery.
class TempCertsCacheNSS {
 public:
  explicit TempCertsCacheNSS(const std::vector<std::string>& x509_certs);
  ~TempCertsCacheNSS();

  static std::vector<std::string> GetUntrustedAuthoritiesFromDeviceOncPolicy();

 private:
  // The actual cache of NSS temporary certificates.
  // Don't delete this field, even if it looks unused!
  // This is a list which owns ScopedCERTCertificate objects. This is sufficient
  // for NSS to be able to find them using CERT_FindCertByName, which is enough
  // for them to be used as intermediate certificates during client certificate
  // matching. Note that when the ScopedCERTCertificate objects go out of scope,
  // they don't necessarily become unavailable in NSS due to caching behavior.
  // However, this is not an issue, as these certificates are not imported into
  // permanent databases, nor are the trust settings mutated to trust them.
  net::ScopedCERTCertificateList temp_certs_;

  DISALLOW_COPY_AND_ASSIGN(TempCertsCacheNSS);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_TEMP_CERTS_CACHE_NSS_H_
