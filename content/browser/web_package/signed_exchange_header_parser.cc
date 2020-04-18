// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_header_parser.h"

#include <map>
#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/web_package/signed_exchange_consts.h"
#include "content/browser/web_package/signed_exchange_utils.h"
#include "crypto/sha2.h"

namespace content {

namespace {

// This covers the characters allowed in Numbers, Labels, and Binary Content.
constexpr char kTokenChars[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_+-*/";

struct ParameterisedLabel {
  std::string label;
  std::map<std::string, std::string> params;
};

// Parser for (a subset of) Structured Headers defined in [SH].
// [SH] https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-02
class StructuredHeaderParser {
 public:
  explicit StructuredHeaderParser(const base::StringPiece& str)
      : input_(str), failed_(false) {}

  bool ParsedSuccessfully() const { return !failed_ && input_.empty(); }

  // Parses a List ([SH] 4.8) of Strings.
  void ParseStringList(std::vector<std::string>* values) {
    values->push_back(ReadString());
    while (!failed_) {
      SkipWhitespaces();
      if (!ConsumeChar(','))
        break;
      SkipWhitespaces();
      values->push_back(ReadString());
    }
  }

  // Parses a List ([SH] 4.8) of Parameterised Labels.
  void ParseParameterisedLabelList(std::vector<ParameterisedLabel>* values) {
    values->push_back(ParameterisedLabel());
    ParseParameterisedLabel(&values->back());
    while (!failed_) {
      SkipWhitespaces();
      if (!ConsumeChar(','))
        break;
      SkipWhitespaces();
      values->push_back(ParameterisedLabel());
      ParseParameterisedLabel(&values->back());
    }
  }

  // Parses a Parameterised Label ([SH] 4.4).
  void ParseParameterisedLabel(ParameterisedLabel* out) {
    std::string label = ReadToken();
    if (label.empty()) {
      DVLOG(1) << "ParseParameterisedLabel: Label expected, got '"
               << input_.front() << "'";
      failed_ = true;
      return;
    }
    out->label = label;

    while (!failed_) {
      SkipWhitespaces();
      if (!ConsumeChar(';'))
        break;
      SkipWhitespaces();

      std::string name = ReadToken();
      if (name.empty()) {
        DVLOG(1) << "ParseParameterisedLabel: Label expected, got '"
                 << input_.front() << "'";
        failed_ = true;
        return;
      }
      std::string value;
      if (ConsumeChar('='))
        value = ReadItem();
      if (!out->params.insert(std::make_pair(name, value)).second) {
        DVLOG(1) << "ParseParameterisedLabel: duplicated parameter: " << name;
        failed_ = true;
        return;
      }
    }
  }

 private:
  void SkipWhitespaces() {
    input_ = base::TrimWhitespaceASCII(input_, base::TRIM_LEADING);
  }

  std::string ReadToken() {
    size_t len = input_.find_first_not_of(kTokenChars);
    if (len == base::StringPiece::npos)
      len = input_.size();
    std::string token(input_.substr(0, len));
    input_.remove_prefix(len);
    return token;
  }

  bool ConsumeChar(char expected) {
    if (!input_.empty() && input_.front() == expected) {
      input_.remove_prefix(1);
      return true;
    }
    return false;
  }

  // [SH] 4.2. Strings
  std::string ReadString() {
    std::string s;
    if (!ConsumeChar('"')) {
      DVLOG(1) << "ReadString: '\"' expected, got '" << input_.front() << "'";
      failed_ = true;
      return s;
    }
    while (!ConsumeChar('"')) {
      size_t len = input_.find_first_of("\"\\");
      if (len == base::StringPiece::npos) {
        DVLOG(1) << "ReadString: missing closing '\"'";
        failed_ = true;
        return s;
      }
      s.append(std::string(input_.substr(0, len)));
      input_.remove_prefix(len);
      if (ConsumeChar('\\')) {
        if (input_.empty()) {
          DVLOG(1) << "ReadString: backslash at string end";
          failed_ = true;
          return s;
        }
        s.push_back(input_.front());
        input_.remove_prefix(1);
      }
    }
    return s;
  }

  // [SH] 4.5. Binary Content
  std::string ReadBinary() {
    if (!ConsumeChar('*')) {
      DVLOG(1) << "ReadBinary: '*' expected, got '" << input_.front() << "'";
      failed_ = true;
      return std::string();
    }
    std::string base64 = ReadToken();
    // Binary Content does not have padding, so we have to add it.
    base64.resize((base64.size() + 3) / 4 * 4, '=');
    std::string binary;
    if (!base::Base64Decode(base64, &binary)) {
      DVLOG(1) << "ReadBinary: failed to decode base64: " << base64;
      failed_ = true;
    }
    return binary;
  }

  // [SH] 4.6. Items
  std::string ReadItem() {
    if (input_.empty()) {
      DVLOG(1) << "ReadItem: unexpected EOF";
      failed_ = true;
      return std::string();
    }
    switch (input_.front()) {
      case '"':
        return ReadString();
      case '*':
        return ReadBinary();
      default:  // label or number
        return ReadToken();
    }
  }

  base::StringPiece input_;
  bool failed_;
};

}  // namespace

// static
base::Optional<std::vector<SignedExchangeHeaderParser::Signature>>
SignedExchangeHeaderParser::ParseSignature(
    base::StringPiece signature_str,
    SignedExchangeDevToolsProxy* devtools_proxy) {
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "SignedExchangeHeaderParser::ParseSignature");

  StructuredHeaderParser parser(signature_str);
  std::vector<ParameterisedLabel> values;
  parser.ParseParameterisedLabelList(&values);
  if (!parser.ParsedSuccessfully()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeHeaderParser::ParseSignature",
        "Failed to parse signature header.");
    return base::nullopt;
  }

  std::vector<Signature> signatures;
  signatures.reserve(values.size());
  for (auto& value : values) {
    signatures.push_back(Signature());
    Signature& sig = signatures.back();
    sig.label = value.label;
    sig.sig = value.params[kSig];
    if (sig.sig.empty()) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "SignedExchangeHeaderParser::ParseSignature",
          "'sig' parameter is not set,");
      return base::nullopt;
    }
    sig.integrity = value.params[kIntegrity];
    if (sig.integrity.empty()) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "SignedExchangeHeaderParser::ParseSignature",
          "'integrity' parameter is not set.");
      return base::nullopt;
    }
    sig.cert_url = GURL(value.params[kCertUrl]);
    if (!sig.cert_url.is_valid() || sig.cert_url.has_ref()) {
      // TODO(https://crbug.com/819467) : When we will support "ed25519Key", the
      // params may not have "certUrl".
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "SignedExchangeHeaderParser::ParseSignature",
          "'certUrl' parameter is not a valid URL.");
      return base::nullopt;
    }
    const std::string cert_sha256_string = value.params[kCertSha256Key];
    if (cert_sha256_string.size() != crypto::kSHA256Length) {
      // TODO(https://crbug.com/819467) : When we will support "ed25519Key", the
      // params may not have "certSha256".
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "SignedExchangeHeaderParser::ParseSignature",
          "'certSha256' parameter is not a SHA-256 digest.");
      return base::nullopt;
    }
    net::SHA256HashValue cert_sha256;
    memcpy(&cert_sha256.data, cert_sha256_string.data(), crypto::kSHA256Length);
    sig.cert_sha256 = std::move(cert_sha256);
    // TODO(https://crbug.com/819467): Support ed25519key.
    // sig.ed25519_key = value.params["ed25519Key"];
    sig.validity_url = GURL(value.params[kValidityUrlKey]);
    if (!sig.validity_url.is_valid()) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "SignedExchangeHeaderParser::ParseSignature",
          "'validityUrl' parameter is not a valid URL.");
      return base::nullopt;
    }
    if (sig.validity_url.has_ref()) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "SignedExchangeHeaderParser::ParseSignature",
          "'validityUrl' parameter can't have a fragment.");
      return base::nullopt;
    }
    if (!base::StringToUint64(value.params[kDateKey], &sig.date)) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "SignedExchangeHeaderParser::ParseSignature",
          "'date' parameter is not a number.");
      return base::nullopt;
    }
    if (!base::StringToUint64(value.params[kExpiresKey], &sig.expires)) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "SignedExchangeHeaderParser::ParseSignature",
          "'expires' parameter is not a number.");
      return base::nullopt;
    }
  }
  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"),
                   "SignedExchangeHeaderParser::ParseSignature");
  return signatures;
}

// static
bool SignedExchangeHeaderParser::GetVersionParamFromContentType(
    base::StringPiece content_type,
    base::Optional<SignedExchangeVersion>* version_param) {
  DCHECK(version_param);
  StructuredHeaderParser parser(content_type);
  ParameterisedLabel parameterised_label;
  parser.ParseParameterisedLabel(&parameterised_label);
  if (!parser.ParsedSuccessfully())
    return false;
  const auto it = parameterised_label.params.find("v");
  if (it == parameterised_label.params.end()) {
    *version_param = base::nullopt;
  } else {
    if (it->second == "b0")
      *version_param = SignedExchangeVersion::kB0;
    else if (it->second == "b1")
      *version_param = SignedExchangeVersion::kB1;
    else
      return false;
  }
  return true;
}

SignedExchangeHeaderParser::Signature::Signature() = default;
SignedExchangeHeaderParser::Signature::Signature(const Signature& other) =
    default;
SignedExchangeHeaderParser::Signature::~Signature() = default;

}  // namespace content
