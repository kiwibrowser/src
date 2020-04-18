// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_header.h"

#include <utility>

#include "base/callback.h"
#include "base/format_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "components/cbor/cbor_reader.h"
#include "content/browser/web_package/signed_exchange_consts.h"
#include "content/browser/web_package/signed_exchange_utils.h"
#include "net/http/http_util.h"

namespace content {

namespace {

// IsStateful{Request,Response}Header return true if |name| is a stateful
// header field. Stateful header fields will cause validation failure of
// signed exchanges.
// Note that |name| must be lower-cased.
// https://wicg.github.io/webpackage/draft-yasskin-httpbis-origin-signed-exchanges-impl.html#stateful-headers
bool IsStatefulRequestHeader(base::StringPiece name) {
  DCHECK_EQ(name, base::ToLowerASCII(name));

  const char* const kStatefulRequestHeaders[] = {
      "authorization", "cookie", "cookie2", "proxy-authorization",
      "sec-webSocket-key"};

  for (const char* field : kStatefulRequestHeaders) {
    if (name == field)
      return true;
  }
  return false;
}

bool IsStatefulResponseHeader(base::StringPiece name) {
  DCHECK_EQ(name, base::ToLowerASCII(name));

  const char* const kStatefulResponseHeaders[] = {
      "authentication-control",
      "authentication-info",
      "optional-www-authenticate",
      "proxy-authenticate",
      "proxy-authentication-info",
      "sec-websocket-accept",
      "set-cookie",
      "set-cookie2",
      "setprofile",
      "www-authenticate",
  };

  for (const char* field : kStatefulResponseHeaders) {
    if (name == field)
      return true;
  }
  return false;
}

bool IsMethodCacheable(base::StringPiece method) {
  return method == "GET" || method == "HEAD" || method == "POST";
}

bool ParseRequestMap(const cbor::CBORValue& value,
                     SignedExchangeHeader* out,
                     SignedExchangeDevToolsProxy* devtools_proxy) {
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"), "ParseRequestMap");
  if (!value.is_map()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "ParseRequestMap",
        base::StringPrintf(
            "Expected request map, got non-map type. Actual type: %d",
            static_cast<int>(value.type())));
    return false;
  }

  const cbor::CBORValue::MapValue& request_map = value.GetMap();

  auto url_iter = request_map.find(
      cbor::CBORValue(kUrlKey, cbor::CBORValue::Type::BYTE_STRING));
  if (url_iter == request_map.end() || !url_iter->second.is_bytestring()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "ParseRequestMap",
        ":url is not found or not a bytestring.");
    return false;
  }
  out->set_request_url(GURL(url_iter->second.GetBytestringAsString()));
  if (!out->request_url().is_valid()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "ParseRequestMap", ":url is not a valid URL.");
    return false;
  }
  if (out->request_url().has_ref()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "ParseRequestMap", ":url can't have a fragment.");
    return false;
  }

  auto method_iter = request_map.find(
      cbor::CBORValue(kMethodKey, cbor::CBORValue::Type::BYTE_STRING));
  if (method_iter == request_map.end() ||
      !method_iter->second.is_bytestring()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "ParseRequestMap",
        ":method is not found or not a bytestring.");
    return false;
  }
  base::StringPiece method_str = method_iter->second.GetBytestringAsString();
  // 3. If exchange’s request method is not safe (Section 4.2.1 of [RFC7231])
  // or not cacheable (Section 4.2.3 of [RFC7231]), return “invalid”.
  // [spec text]
  if (!net::HttpUtil::IsMethodSafe(method_str.as_string()) ||
      !IsMethodCacheable(method_str)) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "ParseRequestMap",
        base::StringPrintf(
            "Request method is not safe or not cacheable. method: %s",
            method_str.as_string().c_str()));
    return false;
  }
  out->set_request_method(method_str);

  for (const auto& it : request_map) {
    if (!it.first.is_bytestring() || !it.second.is_bytestring()) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "ParseRequestMap",
          "Non-bytestring value in the request map.");
      return false;
    }
    base::StringPiece name_str = it.first.GetBytestringAsString();
    if (name_str == kUrlKey || name_str == kMethodKey)
      continue;

    // TODO(kouhei): Add spec ref here once
    // https://github.com/WICG/webpackage/issues/161 is resolved.
    if (name_str != base::ToLowerASCII(name_str)) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "ParseRequestMap",
          base::StringPrintf(
              "Request header name should be lower-cased. header name: %s",
              name_str.as_string().c_str()));
      return false;
    }

    // 4. If exchange’s headers contain a stateful header field, as defined in
    // Section 4.1, return “invalid”. [spec text]
    if (IsStatefulRequestHeader(name_str)) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "ParseRequestMap",
          base::StringPrintf(
              "Exchange contains stateful request header. header name: %s",
              name_str.as_string().c_str()));
      return false;
    }
  }

  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"), "ParseRequestMap");
  return true;
}

bool ParseResponseMap(const cbor::CBORValue& value,
                      SignedExchangeHeader* out,
                      SignedExchangeDevToolsProxy* devtools_proxy) {
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"), "ParseResponseMap");
  if (!value.is_map()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "ParseResponseMap",
        base::StringPrintf(
            "Expected request map, got non-map type. Actual type: %d",
            static_cast<int>(value.type())));
    return false;
  }

  const cbor::CBORValue::MapValue& response_map = value.GetMap();
  auto status_iter = response_map.find(
      cbor::CBORValue(kStatusKey, cbor::CBORValue::Type::BYTE_STRING));
  if (status_iter == response_map.end() ||
      !status_iter->second.is_bytestring()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "ParseRequestMap",
        ":status is not found or not a bytestring.");
    return false;
  }
  base::StringPiece response_code_str =
      status_iter->second.GetBytestringAsString();
  int response_code;
  if (!base::StringToInt(response_code_str, &response_code)) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "ParseRequestMap",
        "Failed to parse status code to integer.");
    return false;
  }
  out->set_response_code(static_cast<net::HttpStatusCode>(response_code));

  for (const auto& it : response_map) {
    if (!it.first.is_bytestring() || !it.second.is_bytestring()) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "ParseRequestMap",
          "Non-bytestring value in the response map.");
      return false;
    }
    base::StringPiece name_str = it.first.GetBytestringAsString();
    if (name_str == kStatusKey)
      continue;
    if (!net::HttpUtil::IsValidHeaderName(name_str)) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "ParseResponseMap",
          base::StringPrintf("Invalid header name. header_name: %s",
                             name_str.as_string().c_str()));
      return false;
    }

    // TODO(kouhei): Add spec ref here once
    // https://github.com/WICG/webpackage/issues/161 is resolved.
    if (name_str != base::ToLowerASCII(name_str)) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "ParseResponseMap",
          base::StringPrintf(
              "Response header name should be lower-cased. header_name: %s",
              name_str.as_string().c_str()));
      return false;
    }

    // 4. If exchange’s headers contain a stateful header field, as defined in
    // Section 4.1, return “invalid”. [spec text]
    if (IsStatefulResponseHeader(name_str)) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "ParseResponseMap",
          base::StringPrintf(
              "Exchange contains stateful response header. header_name: %s",
              name_str.as_string().c_str()));
      return false;
    }

    base::StringPiece value_str = it.second.GetBytestringAsString();
    if (!net::HttpUtil::IsValidHeaderValue(value_str)) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "ParseRequestMap", "Invalid header value.");
      return false;
    }
    if (!out->AddResponseHeader(name_str, value_str)) {
      signed_exchange_utils::ReportErrorAndEndTraceEvent(
          devtools_proxy, "ParseResponseMap",
          base::StringPrintf("Duplicate header value. header_name: %s",
                             name_str.as_string().c_str()));
      return false;
    }
  }

  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"), "ParseResponseMap");
  return true;
}

}  // namespace

constexpr size_t SignedExchangeHeader::kEncodedLengthInBytes;

// static
size_t SignedExchangeHeader::ParseEncodedLength(
    base::span<const uint8_t> input) {
  DCHECK_EQ(input.size(), SignedExchangeHeader::kEncodedLengthInBytes);
  return static_cast<size_t>(input[0]) << 16 |
         static_cast<size_t>(input[1]) << 8 | static_cast<size_t>(input[2]);
}

// static
base::Optional<SignedExchangeHeader> SignedExchangeHeader::Parse(
    base::span<const uint8_t> input,
    SignedExchangeDevToolsProxy* devtools_proxy) {
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "SignedExchangeHeader::Parse");
  cbor::CBORReader::DecoderError error;
  base::Optional<cbor::CBORValue> value = cbor::CBORReader::Read(input, &error);
  if (!value.has_value()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeHeader::Parse",
        base::StringPrintf("Failed to decode CBORValue. CBOR error: %s",
                           cbor::CBORReader::ErrorCodeToString(error)));
    return base::nullopt;
  }
  if (!value->is_array()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeHeader::Parse",
        base::StringPrintf(
            "Expected top-level CBORValue to be an array. Actual type : %d",
            static_cast<int>(value->type())));
    return base::nullopt;
  }

  const cbor::CBORValue::ArrayValue& top_level_array = value->GetArray();
  constexpr size_t kTopLevelArraySize = 2;
  if (top_level_array.size() != kTopLevelArraySize) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeHeader::Parse",
        base::StringPrintf("Expected top-level array to have 2 elements. "
                           "Actual element count: %" PRIuS,
                           top_level_array.size()));
    return base::nullopt;
  }

  SignedExchangeHeader ret;

  if (!ParseRequestMap(top_level_array[0], &ret, devtools_proxy)) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeHeader::Parse",
        "Failed to parse request map.");
    return base::nullopt;
  }
  if (!ParseResponseMap(top_level_array[1], &ret, devtools_proxy)) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeHeader::Parse",
        "Failed to parse response map.");
    return base::nullopt;
  }

  auto signature_iter = ret.response_headers_.find(kSignature);
  if (signature_iter == ret.response_headers_.end()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeHeader::Parse",
        "No signature header found.");
    return base::nullopt;
  }

  base::Optional<std::vector<SignedExchangeHeaderParser::Signature>>
      signatures = SignedExchangeHeaderParser::ParseSignature(
          signature_iter->second, devtools_proxy);
  if (!signatures || signatures->empty()) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy, "SignedExchangeHeader::Parse",
        "Failed to parse signature.");
    return base::nullopt;
  }

  ret.signature_ = (*signatures)[0];

  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"),
                   "SignedExchangeHeader::Parse");
  return std::move(ret);
}

SignedExchangeHeader::SignedExchangeHeader() = default;
SignedExchangeHeader::SignedExchangeHeader(const SignedExchangeHeader&) =
    default;
SignedExchangeHeader::SignedExchangeHeader(SignedExchangeHeader&&) = default;
SignedExchangeHeader::~SignedExchangeHeader() = default;
SignedExchangeHeader& SignedExchangeHeader::operator=(SignedExchangeHeader&&) =
    default;

bool SignedExchangeHeader::AddResponseHeader(base::StringPiece name,
                                             base::StringPiece value) {
  std::string name_str = name.as_string();
  DCHECK_EQ(name_str, base::ToLowerASCII(name))
      << "Response header names should be always lower-cased.";
  if (response_headers_.find(name_str) != response_headers_.end())
    return false;

  response_headers_.emplace(std::move(name_str), value.as_string());
  return true;
}

scoped_refptr<net::HttpResponseHeaders>
SignedExchangeHeader::BuildHttpResponseHeaders() const {
  std::string header_str("HTTP/1.1 ");
  header_str.append(base::NumberToString(response_code()));
  header_str.append(" ");
  header_str.append(net::GetHttpReasonPhrase(response_code()));
  header_str.append(" \r\n");
  for (const auto& it : response_headers()) {
    header_str.append(it.first);
    header_str.append(": ");
    header_str.append(it.second);
    header_str.append("\r\n");
  }
  header_str.append("\r\n");
  return base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(header_str.c_str(), header_str.size()));
}

}  // namespace content
