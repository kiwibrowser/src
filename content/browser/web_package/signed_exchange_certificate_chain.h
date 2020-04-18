// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_CERTIFICATE_CHAIN_H_
#define CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_CERTIFICATE_CHAIN_H_

#include <memory>

#include "base/containers/span.h"
#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "base/strings/string_piece_forward.h"
#include "content/browser/web_package/signed_exchange_consts.h"
#include "content/common/content_export.h"

namespace net {
class X509Certificate;
}  // namespace net

namespace content {

class SignedExchangeDevToolsProxy;

// SignedExchangeCertificateChain contains all information in signed exchange
// certificate chain.
// https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#cert-chain-format
class CONTENT_EXPORT SignedExchangeCertificateChain {
 public:
  static std::unique_ptr<SignedExchangeCertificateChain> Parse(
      SignedExchangeVersion version,
      base::span<const uint8_t> cert_response_body,
      SignedExchangeDevToolsProxy* devtools_proxy);

  // Regular consumers should use the static Parse() rather than directly
  // calling this.
  SignedExchangeCertificateChain(scoped_refptr<net::X509Certificate> cert,
                                 const std::string& ocsp,
                                 const std::string& sct);

  // Parses a TLS 1.3 Certificate message containing X.509v3 certificates and
  // returns a vector of cert_data. Returns nullopt when failed to parse.
  static base::Optional<std::vector<base::StringPiece>> GetCertChainFromMessage(
      base::span<const uint8_t> message);

  ~SignedExchangeCertificateChain();

  const scoped_refptr<net::X509Certificate>& cert() const { return cert_; }
  const std::string& ocsp() const { return ocsp_; }
  const std::string& sct() const { return sct_; }

 private:
  scoped_refptr<net::X509Certificate> cert_;

  // Version b1 specific fields:
  std::string ocsp_;
  std::string sct_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_CERTIFICATE_CHAIN_H_
