// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/cors/cors.h"

#include <string>

#include "services/network/public/cpp/cors/cors.h"
#include "services/network/public/cpp/cors/preflight_cache.h"
#include "third_party/blink/renderer/platform/loader/cors/cors_error_string.h"
#include "third_party/blink/renderer/platform/network/http_header_map.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/thread_specific.h"
#include "url/gurl.h"

namespace blink {

namespace {

base::Optional<std::string> GetHeaderValue(const HTTPHeaderMap& header_map,
                                           const AtomicString& header_name) {
  if (header_map.Contains(header_name)) {
    const AtomicString& atomic_value = header_map.Get(header_name);
    CString string_value = atomic_value.GetString().Utf8();
    return std::string(string_value.data(), string_value.length());
  }
  return base::nullopt;
}

network::cors::PreflightCache& GetPerThreadPreflightCache() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<network::cors::PreflightCache>,
                                  cache, ());
  return *cache;
}

base::Optional<std::string> GetOptionalHeaderValue(
    const HTTPHeaderMap& header_map,
    const AtomicString& header_name) {
  const AtomicString& result = header_map.Get(header_name);
  if (result.IsNull())
    return base::nullopt;

  return std::string(result.Ascii().data());
}

std::unique_ptr<net::HttpRequestHeaders> CreateNetHttpRequestHeaders(
    const HTTPHeaderMap& header_map) {
  std::unique_ptr<net::HttpRequestHeaders> request_headers =
      std::make_unique<net::HttpRequestHeaders>();
  for (HTTPHeaderMap::const_iterator i = header_map.begin(),
                                     end = header_map.end();
       i != end; ++i) {
    DCHECK(!i->key.IsNull());
    DCHECK(!i->value.IsNull());
    request_headers->SetHeader(std::string(i->key.Ascii().data()),
                               std::string(i->value.Ascii().data()));
  }
  return request_headers;
}

}  // namespace

namespace CORS {

base::Optional<network::mojom::CORSError> CheckAccess(
    const KURL& response_url,
    const int response_status_code,
    const HTTPHeaderMap& response_header,
    network::mojom::FetchCredentialsMode credentials_mode,
    const SecurityOrigin& origin) {
  std::unique_ptr<SecurityOrigin::PrivilegeData> privilege =
      origin.CreatePrivilegeData();
  return network::cors::CheckAccess(
      response_url, response_status_code,
      GetHeaderValue(response_header, HTTPNames::Access_Control_Allow_Origin),
      GetHeaderValue(response_header,
                     HTTPNames::Access_Control_Allow_Credentials),
      credentials_mode, origin.ToUrlOrigin(),
      !privilege->block_local_access_from_local_origin_);
}

base::Optional<network::mojom::CORSError> CheckPreflightAccess(
    const KURL& response_url,
    const int response_status_code,
    const HTTPHeaderMap& response_header,
    network::mojom::FetchCredentialsMode actual_credentials_mode,
    const SecurityOrigin& origin) {
  std::unique_ptr<SecurityOrigin::PrivilegeData> privilege =
      origin.CreatePrivilegeData();
  return network::cors::CheckPreflightAccess(
      response_url, response_status_code,
      GetHeaderValue(response_header, HTTPNames::Access_Control_Allow_Origin),
      GetHeaderValue(response_header,
                     HTTPNames::Access_Control_Allow_Credentials),
      actual_credentials_mode, origin.ToUrlOrigin(),
      !privilege->block_local_access_from_local_origin_);
}

base::Optional<network::mojom::CORSError> CheckRedirectLocation(
    const KURL& url) {
  static const bool run_blink_side_scheme_check =
      !RuntimeEnabledFeatures::OutOfBlinkCORSEnabled();
  // TODO(toyoshim): Deprecate Blink side scheme check when we enable
  // out-of-renderer CORS support. This will need to deprecate Blink APIs that
  // are currently used by an embedder. See https://crbug.com/800669.
  if (run_blink_side_scheme_check &&
      !SchemeRegistry::ShouldTreatURLSchemeAsCORSEnabled(url.Protocol())) {
    return network::mojom::CORSError::kRedirectDisallowedScheme;
  }
  return network::cors::CheckRedirectLocation(url, run_blink_side_scheme_check);
}

base::Optional<network::mojom::CORSError> CheckPreflight(
    const int preflight_response_status_code) {
  return network::cors::CheckPreflight(preflight_response_status_code);
}

base::Optional<network::mojom::CORSError> CheckExternalPreflight(
    const HTTPHeaderMap& response_header) {
  return network::cors::CheckExternalPreflight(GetHeaderValue(
      response_header, HTTPNames::Access_Control_Allow_External));
}

bool IsCORSEnabledRequestMode(network::mojom::FetchRequestMode request_mode) {
  return network::cors::IsCORSEnabledRequestMode(request_mode);
}

bool EnsurePreflightResultAndCacheOnSuccess(
    const HTTPHeaderMap& response_header_map,
    const String& origin,
    const KURL& request_url,
    const String& request_method,
    const HTTPHeaderMap& request_header_map,
    network::mojom::FetchCredentialsMode request_credentials_mode,
    String* error_description) {
  DCHECK(!origin.IsNull());
  DCHECK(!request_method.IsNull());
  DCHECK(error_description);

  base::Optional<network::mojom::CORSError> error;

  std::unique_ptr<network::cors::PreflightResult> result =
      network::cors::PreflightResult::Create(
          request_credentials_mode,
          GetOptionalHeaderValue(response_header_map,
                                 HTTPNames::Access_Control_Allow_Methods),
          GetOptionalHeaderValue(response_header_map,
                                 HTTPNames::Access_Control_Allow_Headers),
          GetOptionalHeaderValue(response_header_map,
                                 HTTPNames::Access_Control_Max_Age),
          &error);
  if (error) {
    *error_description = CORS::GetErrorString(
        CORS::ErrorParameter::CreateForPreflightResponseCheck(*error,
                                                              String()));
    return false;
  }

  error = result->EnsureAllowedCrossOriginMethod(
      std::string(request_method.Ascii().data()));
  if (error) {
    *error_description = CORS::GetErrorString(
        CORS::ErrorParameter::CreateForPreflightResponseCheck(*error,
                                                              request_method));
    return false;
  }

  std::string detected_error_header;
  error = result->EnsureAllowedCrossOriginHeaders(
      *CreateNetHttpRequestHeaders(request_header_map), &detected_error_header);
  if (error) {
    *error_description = CORS::GetErrorString(
        CORS::ErrorParameter::CreateForPreflightResponseCheck(
            *error, String(detected_error_header.data(),
                           detected_error_header.length())));
    return false;
  }

  DCHECK(!error);

  GetPerThreadPreflightCache().AppendEntry(std::string(origin.Ascii().data()),
                                           request_url, std::move(result));
  return true;
}

bool CheckIfRequestCanSkipPreflight(
    const String& origin,
    const KURL& url,
    network::mojom::FetchCredentialsMode credentials_mode,
    const String& method,
    const HTTPHeaderMap& request_header_map) {
  DCHECK(!origin.IsNull());
  DCHECK(!method.IsNull());

  return GetPerThreadPreflightCache().CheckIfRequestCanSkipPreflight(
      std::string(origin.Ascii().data()), url, credentials_mode,
      std::string(method.Ascii().data()),
      *CreateNetHttpRequestHeaders(request_header_map));
}

bool IsCORSSafelistedMethod(const String& method) {
  DCHECK(!method.IsNull());
  CString utf8_method = method.Utf8();
  return network::cors::IsCORSSafelistedMethod(
      std::string(utf8_method.data(), utf8_method.length()));
}

bool IsCORSSafelistedContentType(const String& media_type) {
  CString utf8_media_type = media_type.Utf8();
  return network::cors::IsCORSSafelistedContentType(
      std::string(utf8_media_type.data(), utf8_media_type.length()));
}

bool IsCORSSafelistedHeader(const String& name, const String& value) {
  DCHECK(!name.IsNull());
  DCHECK(!value.IsNull());
  CString utf8_name = name.Utf8();
  CString utf8_value = value.Utf8();
  return network::cors::IsCORSSafelistedHeader(
      std::string(utf8_name.data(), utf8_name.length()),
      std::string(utf8_value.data(), utf8_value.length()));
}

bool IsForbiddenHeaderName(const String& name) {
  CString utf8_name = name.Utf8();
  return network::cors::IsForbiddenHeader(
      std::string(utf8_name.data(), utf8_name.length()));
}

bool ContainsOnlyCORSSafelistedHeaders(const HTTPHeaderMap& header_map) {
  for (const auto& header : header_map) {
    if (!IsCORSSafelistedHeader(header.key, header.value))
      return false;
  }
  return true;
}

bool ContainsOnlyCORSSafelistedOrForbiddenHeaders(
    const HTTPHeaderMap& header_map) {
  for (const auto& header : header_map) {
    if (!IsCORSSafelistedHeader(header.key, header.value) &&
        !IsForbiddenHeaderName(header.key))
      return false;
  }
  return true;
}

bool IsOkStatus(int status) {
  return network::cors::IsOkStatus(status);
}

}  // namespace CORS

}  // namespace blink
