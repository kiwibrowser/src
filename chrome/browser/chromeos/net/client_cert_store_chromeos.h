// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_NET_CLIENT_CERT_STORE_CHROMEOS_H_
#define CHROME_BROWSER_CHROMEOS_NET_CLIENT_CERT_STORE_CHROMEOS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/ssl/client_cert_store_nss.h"

namespace chromeos {

class CertificateProvider;

class ClientCertStoreChromeOS : public net::ClientCertStore {
 public:
  using PasswordDelegateFactory =
      net::ClientCertStoreNSS::PasswordDelegateFactory;

  class CertFilter {
   public:
    virtual ~CertFilter() {}

    // Initializes this filter. Returns true if it finished initialization,
    // otherwise returns false and calls |callback| once the initialization is
    // completed.
    // Must be called at most once.
    virtual bool Init(const base::Closure& callback) = 0;

    // Returns true if |cert| is allowed to be used as a client certificate
    // (e.g. for a certain browser context or user).
    // This is only called once initialization is finished, see Init().
    virtual bool IsCertAllowed(CERTCertificate* cert) const = 0;
  };

  // This ClientCertStore will return client certs from NSS certificate
  // databases that pass the filter |cert_filter| and additionally return
  // certificates provided by |cert_provider|.
  ClientCertStoreChromeOS(
      std::unique_ptr<CertificateProvider> cert_provider,
      std::unique_ptr<CertFilter> cert_filter,
      const PasswordDelegateFactory& password_delegate_factory);
  ~ClientCertStoreChromeOS() override;

  // net::ClientCertStore:
  void GetClientCerts(const net::SSLCertRequestInfo& cert_request_info,
                      const ClientCertListCallback& callback) override;

 private:
  void GotAdditionalCerts(const net::SSLCertRequestInfo* request,
                          const ClientCertListCallback& callback,
                          net::ClientCertIdentityList additional_certs);

  net::ClientCertIdentityList GetAndFilterCertsOnWorkerThread(
      scoped_refptr<crypto::CryptoModuleBlockingPasswordDelegate>
          password_delegate,
      const net::SSLCertRequestInfo* request,
      net::ClientCertIdentityList additional_certs);

  std::unique_ptr<CertificateProvider> cert_provider_;
  std::unique_ptr<CertFilter> cert_filter_;

  // The factory for creating the delegate for requesting a password to a
  // PKCS#11 token. May be null.
  PasswordDelegateFactory password_delegate_factory_;

  DISALLOW_COPY_AND_ASSIGN(ClientCertStoreChromeOS);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_NET_CLIENT_CERT_STORE_CHROMEOS_H_
