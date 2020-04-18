// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_certificate_chain.h"

#include "base/command_line.h"
#include "base/format_macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "components/cbor/cbor_reader.h"
#include "content/browser/web_package/signed_exchange_consts.h"
#include "content/browser/web_package/signed_exchange_utils.h"
#include "content/public/common/content_switches.h"
#include "net/cert/x509_certificate.h"

namespace content {

namespace {

bool ConsumeByte(base::span<const uint8_t>* data, uint8_t* out) {
  if (data->empty())
    return false;
  *out = (*data)[0];
  *data = data->subspan(1);
  return true;
}

bool Consume2Bytes(base::span<const uint8_t>* data, uint16_t* out) {
  if (data->size() < 2)
    return false;
  *out = ((*data)[0] << 8) | (*data)[1];
  *data = data->subspan(2);
  return true;
}

bool Consume3Bytes(base::span<const uint8_t>* data, uint32_t* out) {
  if (data->size() < 3)
    return false;
  *out = ((*data)[0] << 16) | ((*data)[1] << 8) | (*data)[2];
  *data = data->subspan(3);
  return true;
}

std::unique_ptr<SignedExchangeCertificateChain> ParseB0(
    base::span<const uint8_t> message,
    SignedExchangeDevToolsProxy* devtools_proxy) {
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "SignedExchangeCertificateChain::ParseB0");
  base::Optional<std::vector<base::StringPiece>> der_certs =
      SignedExchangeCertificateChain::GetCertChainFromMessage(message);
  if (!der_certs) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeCertificateChain::ParseB0",
        "Failed to parse the response as a TLS 1.3 Certificate message.");
    return nullptr;
  }

  scoped_refptr<net::X509Certificate> cert =
      net::X509Certificate::CreateFromDERCertChain(*der_certs);
  if (!cert) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeCertificateChain::ParseB0",
        "X509Certificate::CreateFromDERCertChain failed.");
    return nullptr;
  }

  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"),
                   "SignedExchangeCertificateChain::ParseB1");
  // The V0 certificate format doesn't support OCSP nor SCT.
  return base::WrapUnique(
      new SignedExchangeCertificateChain(cert, "" /* ocsp */, "" /* sct */));
}

// https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#cert-chain-format
std::unique_ptr<SignedExchangeCertificateChain> ParseB1(
    base::span<const uint8_t> message,
    SignedExchangeDevToolsProxy* devtools_proxy) {
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "SignedExchangeCertificateChain::ParseB1");

  cbor::CBORReader::DecoderError error;
  base::Optional<cbor::CBORValue> value =
      cbor::CBORReader::Read(message, &error);
  if (!value.has_value()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
        base::StringPrintf("Failed to decode CBORValue. CBOR error: %s",
                           cbor::CBORReader::ErrorCodeToString(error)));
    return nullptr;
  }
  if (!value->is_array()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
        base::StringPrintf(
            "Expected top-level CBORValue to be an array. Actual type: %d",
            static_cast<int>(value->type())));
    return nullptr;
  }

  const cbor::CBORValue::ArrayValue& top_level_array = value->GetArray();
  // Expect at least 2 elements (magic string and main certificate).
  if (top_level_array.size() < 2) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
        base::StringPrintf(
            "Expected top-level array to have at least 2 elements."
            "Actual element count: %" PRIuS,
            top_level_array.size()));
    return nullptr;
  }
  if (!top_level_array[0].is_string() ||
      top_level_array[0].GetString() != kCertChainCborMagic) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
        "First element of cert chain CBOR does not match the magic string.");
    return nullptr;
  }

  std::vector<base::StringPiece> der_certs;
  der_certs.reserve(top_level_array.size() - 1);
  std::string ocsp;
  std::string sct;

  for (size_t i = 1; i < top_level_array.size(); i++) {
    if (!top_level_array[i].is_map()) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
          base::StringPrintf(
              "Expected certificate map, got non-map type at index %zu."
              " Actual type: %d",
              i, static_cast<int>(top_level_array[i].type())));
      return nullptr;
    }
    const cbor::CBORValue::MapValue& cert_map = top_level_array[i].GetMap();

    // Step 1. Each cert value MUST be a DER-encoded X.509v3 certificate
    // ([RFC5280]). Other key/value pairs in the same array item define
    // properties of this certificate. [spec text]
    auto cert_iter = cert_map.find(cbor::CBORValue(kCertKey));
    if (cert_iter == cert_map.end() || !cert_iter->second.is_bytestring()) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
          base::StringPrintf(
              "cert is not found or not a bytestring, at index %zu.", i));
      return nullptr;
    }
    der_certs.push_back(cert_iter->second.GetBytestringAsString());

    auto ocsp_iter = cert_map.find(cbor::CBORValue(kOcspKey));
    if (i == 1) {
      // Step 2. The first certificate’s ocsp value if any MUST be a complete,
      // DER-encoded OCSP response for that certificate (using the ASN.1 type
      // OCSPResponse defined in [RFC2560]). ... [spec text]
      if (ocsp_iter == cert_map.end() || !ocsp_iter->second.is_bytestring()) {
        signed_exchange_utils::ReportErrorAndEndTraceEvent(
            devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
            "ocsp is not a bytestring, or not found in the first cert map.");
        return nullptr;
      }
      ocsp = ocsp_iter->second.GetBytestringAsString().as_string();
      if (ocsp.empty()) {
        signed_exchange_utils::ReportErrorAndEndTraceEvent(
            devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
            "ocsp must not be empty.");
        return nullptr;
      }
    } else if (ocsp_iter != cert_map.end()) {
      // Step 2. ... Subsequent certificates MUST NOT have an ocsp value. [spec
      // text]
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
          base::StringPrintf(
              "ocsp value found in a subsequent cert map, at index %zu.", i));
      return nullptr;
    }

    // Step 3. Each certificate’s sct value MUST be a
    // SignedCertificateTimestampList for that certificate as defined by Section
    // 3.3 of [RFC6962]. [spec text]
    //
    // We use SCTs only of the main certificate.
    // TODO(crbug.com/815025): Update the spec text once
    // https://github.com/WICG/webpackage/issues/175 is resolved.
    if (i == 1) {
      auto sct_iter = cert_map.find(cbor::CBORValue(kSctKey));
      if (sct_iter != cert_map.end()) {
        if (!sct_iter->second.is_bytestring()) {
          signed_exchange_utils::ReportErrorAndEndTraceEvent(
              devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
              "sct is not a bytestring.");
          return nullptr;
        }
        sct = sct_iter->second.GetBytestringAsString().as_string();
        if (sct.empty()) {
          signed_exchange_utils::ReportErrorAndEndTraceEvent(
              devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
              "sct must not be empty.");
          return nullptr;
        }
      }
    }
  }
  scoped_refptr<net::X509Certificate> cert =
      net::X509Certificate::CreateFromDERCertChain(der_certs);
  if (!cert) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeCertificateChain::ParseB1",
        "X509Certificate::CreateFromDERCertChain failed.");
    return nullptr;
  }

  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"),
                   "SignedExchangeCertificateChain::ParseB1");
  return base::WrapUnique(new SignedExchangeCertificateChain(cert, ocsp, sct));
}

}  // namespace

// static
std::unique_ptr<SignedExchangeCertificateChain>
SignedExchangeCertificateChain::Parse(
    SignedExchangeVersion version,
    base::span<const uint8_t> cert_response_body,
    SignedExchangeDevToolsProxy* devtools_proxy) {
  switch (version) {
    case SignedExchangeVersion::kB0:
      return ParseB0(cert_response_body, devtools_proxy);
    case SignedExchangeVersion::kB1:
      return ParseB1(cert_response_body, devtools_proxy);
  }
  NOTREACHED();
  return nullptr;
}

// static
base::Optional<std::vector<base::StringPiece>>
SignedExchangeCertificateChain::GetCertChainFromMessage(
    base::span<const uint8_t> message) {
  uint8_t cert_request_context_size = 0;
  if (!ConsumeByte(&message, &cert_request_context_size)) {
    DVLOG(1) << "Can't read certificate request request context size.";
    return base::nullopt;
  }
  if (cert_request_context_size != 0) {
    DVLOG(1) << "Invalid certificate request context size: "
             << static_cast<int>(cert_request_context_size);
    return base::nullopt;
  }
  uint32_t cert_list_size = 0;
  if (!Consume3Bytes(&message, &cert_list_size)) {
    DVLOG(1) << "Can't read certificate list size.";
    return base::nullopt;
  }

  if (cert_list_size != message.size()) {
    DVLOG(1) << "Certificate list size error: cert_list_size=" << cert_list_size
             << " remaining=" << message.size();
    return base::nullopt;
  }

  std::vector<base::StringPiece> certs;
  while (!message.empty()) {
    uint32_t cert_data_size = 0;
    if (!Consume3Bytes(&message, &cert_data_size)) {
      DVLOG(1) << "Can't read certificate data size.";
      return base::nullopt;
    }
    if (message.size() < cert_data_size) {
      DVLOG(1) << "Certificate data size error: cert_data_size="
               << cert_data_size << " remaining=" << message.size();
      return base::nullopt;
    }
    certs.emplace_back(base::StringPiece(
        reinterpret_cast<const char*>(message.data()), cert_data_size));
    message = message.subspan(cert_data_size);

    uint16_t extensions_size = 0;
    if (!Consume2Bytes(&message, &extensions_size)) {
      DVLOG(1) << "Can't read extensions size.";
      return base::nullopt;
    }
    if (message.size() < extensions_size) {
      DVLOG(1) << "Extensions size error: extensions_size=" << extensions_size
               << " remaining=" << message.size();
      return base::nullopt;
    }
    message = message.subspan(extensions_size);
  }
  return certs;
}

SignedExchangeCertificateChain::SignedExchangeCertificateChain(
    scoped_refptr<net::X509Certificate> cert,
    const std::string& ocsp,
    const std::string& sct)
    : cert_(cert), ocsp_(ocsp), sct_(sct) {
  DCHECK(cert);
}

SignedExchangeCertificateChain::~SignedExchangeCertificateChain() = default;

}  // namespace content
