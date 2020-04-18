// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/cors/cors.h"

#include <algorithm>
#include <cctype>
#include <vector>

#include "base/strings/string_util.h"
#include "net/base/mime_util.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"
#include "url/url_util.h"

namespace {

const char kAsterisk[] = "*";
const char kLowerCaseTrue[] = "true";

// TODO(toyoshim): Consider to move following const variables to
// //net/http/http_request_headers.
const char kHeadMethod[] = "HEAD";
const char kPostMethod[] = "POST";

// TODO(toyoshim): Consider to move the following method to
// //net/base/mime_util, and expose to Blink platform/network in order to
// replace the existing equivalent method in HTTPParser.
// We may prefer to implement a strict RFC2616 media-type
// (https://tools.ietf.org/html/rfc2616#section-3.7) parser.
std::string ExtractMIMETypeFromMediaType(const std::string& media_type) {
  std::string::size_type semicolon = media_type.find(';');
  std::string top_level_type;
  std::string subtype;
  if (net::ParseMimeTypeWithoutParameter(media_type.substr(0, semicolon),
                                         &top_level_type, &subtype)) {
    return top_level_type + "/" + subtype;
  }
  return std::string();
}

// url::Origin::Serialize() serializes all Origins with a 'file' scheme to
// 'file://', but it isn't desirable for CORS check. Returns 'null' instead to
// be aligned with HTTP Origin header calculation in Blink SecurityOrigin.
// |allow_file_origin| is used to realize a behavior change that
// the --allow-file-access-from-files command-line flag needs.
// TODO(mkwst): Generalize and move to url/Origin.
std::string Serialize(const url::Origin& origin, bool allow_file_origin) {
  if (!allow_file_origin && origin.scheme() == url::kFileScheme)
    return "null";
  return origin.Serialize();
}

// Returns true only if |header_value| satisfies ABNF: 1*DIGIT [ "." 1*DIGIT ]
bool IsSimilarToDoubleABNF(const std::string& header_value) {
  if (header_value.empty())
    return false;
  char first_char = header_value.at(0);
  if (!isdigit(first_char))
    return false;

  bool period_found = false;
  bool digit_found_after_period = false;
  for (char ch : header_value) {
    if (isdigit(ch)) {
      if (period_found) {
        digit_found_after_period = true;
      }
      continue;
    }
    if (ch == '.') {
      if (period_found)
        return false;
      period_found = true;
      continue;
    }
    return false;
  }
  if (period_found)
    return digit_found_after_period;
  return true;
}

// Returns true only if |header_value| satisfies ABNF: 1*DIGIT
bool IsSimilarToIntABNF(const std::string& header_value) {
  if (header_value.empty())
    return false;

  for (char ch : header_value) {
    if (!isdigit(ch))
      return false;
  }
  return true;
}

// |lower_case_media_type| should be lower case.
bool IsCORSSafelistedLowerCaseContentType(
    const std::string& lower_case_media_type) {
  DCHECK_EQ(lower_case_media_type, base::ToLowerASCII(lower_case_media_type));
  static const std::set<std::string> safe_types = {
      "application/x-www-form-urlencoded", "multipart/form-data", "text/plain"};
  std::string mime_type = ExtractMIMETypeFromMediaType(lower_case_media_type);
  return safe_types.find(mime_type) != safe_types.end();
}

}  // namespace

namespace network {

namespace cors {

namespace header_names {

const char kAccessControlAllowCredentials[] =
    "Access-Control-Allow-Credentials";
const char kAccessControlAllowExternal[] = "Access-Control-Allow-External";
const char kAccessControlAllowHeaders[] = "Access-Control-Allow-Headers";
const char kAccessControlAllowMethods[] = "Access-Control-Allow-Methods";
const char kAccessControlAllowOrigin[] = "Access-Control-Allow-Origin";
const char kAccessControlMaxAge[] = "Access-Control-Max-Age";
const char kAccessControlRequestExternal[] = "Access-Control-Request-External";
const char kAccessControlRequestHeaders[] = "Access-Control-Request-Headers";
const char kAccessControlRequestMethod[] = "Access-Control-Request-Method";

}  // namespace header_names

// See https://fetch.spec.whatwg.org/#cors-check.
base::Optional<mojom::CORSError> CheckAccess(
    const GURL& response_url,
    const int response_status_code,
    const base::Optional<std::string>& allow_origin_header,
    const base::Optional<std::string>& allow_credentials_header,
    mojom::FetchCredentialsMode credentials_mode,
    const url::Origin& origin,
    bool allow_file_origin) {
  // TODO(toyoshim): This response status code check should not be needed. We
  // have another status code check after a CheckAccess() call if it is needed.
  if (!response_status_code)
    return mojom::CORSError::kInvalidResponse;

  if (allow_origin_header == kAsterisk) {
    // A wildcard Access-Control-Allow-Origin can not be used if credentials are
    // to be sent, even with Access-Control-Allow-Credentials set to true.
    // See https://fetch.spec.whatwg.org/#cors-protocol-and-credentials.
    if (credentials_mode != mojom::FetchCredentialsMode::kInclude)
      return base::nullopt;

    // Since the credential is a concept for network schemes, we perform the
    // wildcard check only for HTTP and HTTPS. This is a quick hack to allow
    // data URL (see https://crbug.com/315152).
    // TODO(https://crbug.com/736308): Once the callers exist only in the
    // browser process or network service, this check won't be needed any more
    // because it is always for network requests there.
    if (response_url.SchemeIsHTTPOrHTTPS())
      return mojom::CORSError::kWildcardOriginNotAllowed;
  } else if (!allow_origin_header) {
    return mojom::CORSError::kMissingAllowOriginHeader;
  } else if (*allow_origin_header != Serialize(origin, allow_file_origin)) {
    // We do not use url::Origin::IsSameOriginWith() here for two reasons below.
    //  1. Allow "null" to match here. The latest spec does not have a clear
    //     information about this (https://fetch.spec.whatwg.org/#cors-check),
    //     but the old spec explicitly says that "null" works here
    //     (https://www.w3.org/TR/cors/#resource-sharing-check-0).
    //  2. We do not have a good way to construct url::Origin from the string,
    //     *allow_origin_header, that may be broken. Unfortunately
    //     url::Origin::Create(GURL(*allow_origin_header)) accepts malformed
    //     URL and constructs a valid origin with unexpected fixes, which
    //     results in unexpected behavior.

    // We run some more value checks below to provide better information to
    // developers.

    // Does not allow to have multiple origins in the allow origin header.
    // See https://fetch.spec.whatwg.org/#http-access-control-allow-origin.
    if (allow_origin_header->find_first_of(" ,") != std::string::npos)
      return mojom::CORSError::kMultipleAllowOriginValues;

    // Check valid "null" first since GURL assumes it as invalid.
    if (*allow_origin_header == "null")
      return mojom::CORSError::kAllowOriginMismatch;

    // As commented above, this validation is not strict as an origin
    // validation, but should be ok for providing error details to developers.
    GURL header_origin(*allow_origin_header);
    if (!header_origin.is_valid())
      return mojom::CORSError::kInvalidAllowOriginValue;

    return mojom::CORSError::kAllowOriginMismatch;
  }

  if (credentials_mode == mojom::FetchCredentialsMode::kInclude) {
    // https://fetch.spec.whatwg.org/#http-access-control-allow-credentials.
    // This check should be case sensitive.
    // See also https://fetch.spec.whatwg.org/#http-new-header-syntax.
    if (allow_credentials_header != kLowerCaseTrue)
      return mojom::CORSError::kInvalidAllowCredentials;
  }
  return base::nullopt;
}

base::Optional<mojom::CORSError> CheckPreflightAccess(
    const GURL& response_url,
    const int response_status_code,
    const base::Optional<std::string>& allow_origin_header,
    const base::Optional<std::string>& allow_credentials_header,
    mojom::FetchCredentialsMode actual_credentials_mode,
    const url::Origin& origin,
    bool allow_file_origin) {
  base::Optional<mojom::CORSError> error =
      CheckAccess(response_url, response_status_code, allow_origin_header,
                  allow_credentials_header, actual_credentials_mode, origin,
                  allow_file_origin);
  if (!error)
    return base::nullopt;

  // TODO(toyoshim): Remove following two lines when the status code check is
  // removed from CheckAccess().
  if (*error == mojom::CORSError::kInvalidResponse)
    return error;

  switch (*error) {
    case mojom::CORSError::kWildcardOriginNotAllowed:
      return mojom::CORSError::kPreflightWildcardOriginNotAllowed;
    case mojom::CORSError::kMissingAllowOriginHeader:
      return mojom::CORSError::kPreflightMissingAllowOriginHeader;
    case mojom::CORSError::kMultipleAllowOriginValues:
      return mojom::CORSError::kPreflightMultipleAllowOriginValues;
    case mojom::CORSError::kInvalidAllowOriginValue:
      return mojom::CORSError::kPreflightInvalidAllowOriginValue;
    case mojom::CORSError::kAllowOriginMismatch:
      return mojom::CORSError::kPreflightAllowOriginMismatch;
    case mojom::CORSError::kInvalidAllowCredentials:
      return mojom::CORSError::kPreflightInvalidAllowCredentials;
    default:
      NOTREACHED();
  }
  return error;
}

base::Optional<mojom::CORSError> CheckRedirectLocation(const GURL& redirect_url,
                                                       bool skip_scheme_check) {
  if (!skip_scheme_check) {
    // Block non HTTP(S) schemes as specified in the step 4 in
    // https://fetch.spec.whatwg.org/#http-redirect-fetch. Chromium also allows
    // the data scheme.
    auto& schemes = url::GetCORSEnabledSchemes();
    if (std::find(std::begin(schemes), std::end(schemes),
                  redirect_url.scheme()) == std::end(schemes)) {
      return mojom::CORSError::kRedirectDisallowedScheme;
    }
  }

  // Block URLs including credentials as specified in the step 9 in
  // https://fetch.spec.whatwg.org/#http-redirect-fetch.
  //
  // TODO(tyoshino): This check should be performed also when request's
  // origin is not same origin with the redirect destination's origin.
  if (redirect_url.has_username() || redirect_url.has_password())
    return mojom::CORSError::kRedirectContainsCredentials;

  return base::nullopt;
}

base::Optional<mojom::CORSError> CheckPreflight(const int status_code) {
  // CORS preflight with 3XX is considered network error in
  // Fetch API Spec: https://fetch.spec.whatwg.org/#cors-preflight-fetch
  // CORS Spec: http://www.w3.org/TR/cors/#cross-origin-request-with-preflight-0
  // https://crbug.com/452394
  if (IsOkStatus(status_code))
    return base::nullopt;
  return mojom::CORSError::kPreflightInvalidStatus;
}

// https://wicg.github.io/cors-rfc1918/#http-headerdef-access-control-allow-external
base::Optional<mojom::CORSError> CheckExternalPreflight(
    const base::Optional<std::string>& allow_external) {
  if (!allow_external)
    return mojom::CORSError::kPreflightMissingAllowExternal;
  if (*allow_external == kLowerCaseTrue)
    return base::nullopt;
  return mojom::CORSError::kPreflightInvalidAllowExternal;
}

bool IsCORSEnabledRequestMode(mojom::FetchRequestMode mode) {
  return mode == mojom::FetchRequestMode::kCORS ||
         mode == mojom::FetchRequestMode::kCORSWithForcedPreflight;
}

bool IsCORSSafelistedMethod(const std::string& method) {
  // https://fetch.spec.whatwg.org/#cors-safelisted-method
  // "A CORS-safelisted method is a method that is `GET`, `HEAD`, or `POST`."
  static const std::set<std::string> safe_methods = {
      net::HttpRequestHeaders::kGetMethod, kHeadMethod, kPostMethod};
  return safe_methods.find(base::ToUpperASCII(method)) != safe_methods.end();
}

bool IsCORSSafelistedContentType(const std::string& media_type) {
  return IsCORSSafelistedLowerCaseContentType(base::ToLowerASCII(media_type));
}

bool IsCORSSafelistedHeader(const std::string& name, const std::string& value) {
  // https://fetch.spec.whatwg.org/#cors-safelisted-request-header
  // "A CORS-safelisted header is a header whose name is either one of `Accept`,
  // `Accept-Language`, and `Content-Language`, or whose name is
  // `Content-Type` and value, once parsed, is one of
  //     `application/x-www-form-urlencoded`, `multipart/form-data`, and
  //     `text/plain`
  // or whose name is a byte-case-insensitive match for one of
  //      `DPR`, `Save-Data`, `device-memory`, `Viewport-Width`, and `Width`,
  // and whose value, once extracted, is not failure."
  //
  // Treat inspector headers as a CORS-safelisted headers, since they are added
  // by blink when the inspector is open.
  //
  // Treat 'Intervention' as a CORS-safelisted header, since it is added by
  // Chrome when an intervention is (or may be) applied.
  static const std::set<std::string> safe_names = {
      "accept", "accept-language", "content-language",
      "x-devtools-emulate-network-conditions-client-id", "intervention",
      "content-type", "save-data",
      // The Device Memory header field is a number that indicates the client’s
      // device memory i.e. approximate amount of ram in GiB. The header value
      // must satisfy ABNF  1*DIGIT [ "." 1*DIGIT ]
      // See
      // https://w3c.github.io/device-memory/#sec-device-memory-client-hint-header
      // for more details.
      "device-memory", "dpr", "width", "viewport-width"};
  const std::string lower_name = base::ToLowerASCII(name);
  if (safe_names.find(lower_name) == safe_names.end())
    return false;

  // Client hints are device specific, and not origin specific. As such all
  // client hint headers are considered as safe.
  // See third_party/WebKit/public/platform/web_client_hints_types.mojom.
  // Client hint headers can be added by Chrome automatically or via JavaScript.
  if (lower_name == "device-memory" || lower_name == "dpr")
    return IsSimilarToDoubleABNF(value);
  if (lower_name == "width" || lower_name == "viewport-width")
    return IsSimilarToIntABNF(value);
  const std::string lower_value = base::ToLowerASCII(value);
  if (lower_name == "save-data")
    return lower_value == "on";

  if (lower_name == "content-type")
    return IsCORSSafelistedLowerCaseContentType(lower_value);

  return true;
}

bool IsForbiddenMethod(const std::string& method) {
  static const std::vector<std::string> forbidden_methods = {"trace", "track",
                                                             "connect"};
  const std::string lower_method = base::ToLowerASCII(method);
  return std::find(forbidden_methods.begin(), forbidden_methods.end(),
                   lower_method) != forbidden_methods.end();
}

bool IsForbiddenHeader(const std::string& name) {
  // http://fetch.spec.whatwg.org/#forbidden-header-name
  // "A forbidden header name is a header name that is one of:
  //   `Accept-Charset`, `Accept-Encoding`, `Access-Control-Request-Headers`,
  //   `Access-Control-Request-Method`, `Connection`, `Content-Length`,
  //   `Cookie`, `Cookie2`, `Date`, `DNT`, `Expect`, `Host`, `Keep-Alive`,
  //   `Origin`, `Referer`, `TE`, `Trailer`, `Transfer-Encoding`, `Upgrade`,
  //   `User-Agent`, `Via`
  // or starts with `Proxy-` or `Sec-` (including when it is just `Proxy-` or
  // `Sec-`)."
  static const std::set<std::string> forbidden_names = {
      "accept-charset",
      "accept-encoding",
      "access-control-request-headers",
      "access-control-request-method",
      "connection",
      "content-length",
      "cookie",
      "cookie2",
      "date",
      "dnt",
      "expect",
      "host",
      "keep-alive",
      "origin",
      "referer",
      "te",
      "trailer",
      "transfer-encoding",
      "upgrade",
      "user-agent",
      "via"};
  const std::string lower_name = base::ToLowerASCII(name);
  if (StartsWith(lower_name, "proxy-", base::CompareCase::SENSITIVE) ||
      StartsWith(lower_name, "sec-", base::CompareCase::SENSITIVE)) {
    return true;
  }
  return forbidden_names.find(lower_name) != forbidden_names.end();
}

bool IsOkStatus(int status) {
  return status >= 200 && status < 300;
}

}  // namespace cors

}  // namespace network
