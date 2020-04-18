// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_HEADER_PARSER_H_
#define CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_HEADER_PARSER_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string_piece.h"
#include "content/browser/web_package/signed_exchange_consts.h"
#include "content/common/content_export.h"
#include "net/base/hash_value.h"
#include "url/gurl.h"

namespace content {

class SignedExchangeDevToolsProxy;

// Provide parsers for signed-exchange headers.
// https://wicg.github.io/webpackage/draft-yasskin-httpbis-origin-signed-exchanges-impl.html
class CONTENT_EXPORT SignedExchangeHeaderParser {
 public:
  struct CONTENT_EXPORT Signature {
    Signature();
    Signature(const Signature&);
    ~Signature();

    std::string label;
    std::string sig;
    std::string integrity;
    GURL cert_url;
    base::Optional<net::SHA256HashValue> cert_sha256;
    // TODO(https://crbug.com/819467): Support ed25519key.
    // std::string ed25519_key;
    GURL validity_url;
    uint64_t date;
    uint64_t expires;
  };

  // Parses a value of the Signature header.
  // https://wicg.github.io/webpackage/draft-yasskin-httpbis-origin-signed-exchanges-impl.html#signature-header
  static base::Optional<std::vector<Signature>> ParseSignature(
      base::StringPiece signature_str,
      SignedExchangeDevToolsProxy* devtools_proxy);

  // Parses |content_type| to get the value of "v=" parameter of the signed
  // exchange, and converts to SignedExchangeVersion. Returns false if failed to
  // parse.
  static bool GetVersionParamFromContentType(
      base::StringPiece content_type,
      base::Optional<SignedExchangeVersion>* version_param);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_HEADER_PARSER_H_
