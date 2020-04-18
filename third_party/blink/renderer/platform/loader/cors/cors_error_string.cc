// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/cors/cors_error_string.h"

#include "third_party/blink/renderer/platform/network/http_header_map.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"

namespace blink {

namespace CORS {

namespace {

const KURL& GetInvalidURL() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(KURL, invalid_url, ());
  return invalid_url;
}

bool IsInterestingStatusCode(int status_code) {
  // Predicate that gates what status codes should be included in console error
  // messages for responses containing no access control headers.
  return status_code >= 400;
}

ErrorParameter CreateWrongParameter(network::mojom::CORSError error) {
  return ErrorParameter(
      error, GetInvalidURL(), GetInvalidURL(), 0 /* status_code */,
      HTTPHeaderMap(), *SecurityOrigin::CreateUnique(),
      WebURLRequest::kRequestContextUnspecified, String(), true);
}

}  // namespace

// static
ErrorParameter ErrorParameter::Create(
    const network::CORSErrorStatus& error_status,
    const KURL& first_url,
    const KURL& second_url,
    const int status_code,
    const HTTPHeaderMap& header_map,
    const SecurityOrigin& origin,
    const WebURLRequest::RequestContext context) {
  String hint;
  switch (error_status.cors_error) {
    case network::mojom::CORSError::kMethodDisallowedByPreflightResponse:
    case network::mojom::CORSError::kHeaderDisallowedByPreflightResponse:
      DCHECK(!error_status.failed_parameter.empty());
      hint = String(error_status.failed_parameter.c_str());
      break;
    default:
      break;
  }
  return ErrorParameter(error_status.cors_error, first_url, second_url,
                        status_code, header_map, origin, context, hint, false);
}

// static
ErrorParameter ErrorParameter::CreateForDisallowedByMode(
    const KURL& request_url) {
  return ErrorParameter(network::mojom::CORSError::kDisallowedByMode,
                        request_url, GetInvalidURL(), 0 /* status_code */,
                        HTTPHeaderMap(), *SecurityOrigin::CreateUnique(),
                        WebURLRequest::kRequestContextUnspecified, String(),
                        false);
}

// static
ErrorParameter ErrorParameter::CreateForInvalidResponse(
    const KURL& request_url,
    const SecurityOrigin& origin) {
  return ErrorParameter(
      network::mojom::CORSError::kInvalidResponse, request_url, GetInvalidURL(),
      0 /* status_code */, HTTPHeaderMap(), origin,
      WebURLRequest::kRequestContextUnspecified, String(), false);
}

// static
ErrorParameter ErrorParameter::CreateForAccessCheck(
    const network::mojom::CORSError error,
    const KURL& request_url,
    int response_status_code,
    const HTTPHeaderMap& response_header_map,
    const SecurityOrigin& origin,
    const WebURLRequest::RequestContext context,
    const KURL& redirect_url) {
  switch (error) {
    case network::mojom::CORSError::kInvalidResponse:
    case network::mojom::CORSError::kWildcardOriginNotAllowed:
    case network::mojom::CORSError::kMissingAllowOriginHeader:
    case network::mojom::CORSError::kMultipleAllowOriginValues:
    case network::mojom::CORSError::kInvalidAllowOriginValue:
    case network::mojom::CORSError::kAllowOriginMismatch:
    case network::mojom::CORSError::kInvalidAllowCredentials:
    case network::mojom::CORSError::kPreflightWildcardOriginNotAllowed:
    case network::mojom::CORSError::kPreflightMissingAllowOriginHeader:
    case network::mojom::CORSError::kPreflightMultipleAllowOriginValues:
    case network::mojom::CORSError::kPreflightInvalidAllowOriginValue:
    case network::mojom::CORSError::kPreflightAllowOriginMismatch:
    case network::mojom::CORSError::kPreflightInvalidAllowCredentials:
      return ErrorParameter(error, request_url, redirect_url,
                            response_status_code, response_header_map, origin,
                            context, String(), false);
    default:
      NOTREACHED();
  }
  return CreateWrongParameter(error);
}

// static
ErrorParameter ErrorParameter::CreateForPreflightStatusCheck(
    int response_status_code) {
  return ErrorParameter(network::mojom::CORSError::kPreflightInvalidStatus,
                        GetInvalidURL(), GetInvalidURL(), response_status_code,
                        HTTPHeaderMap(), *SecurityOrigin::CreateUnique(),
                        WebURLRequest::kRequestContextUnspecified, String(),
                        false);
}

// static
ErrorParameter ErrorParameter::CreateForDisallowedRedirect() {
  return ErrorParameter(
      network::mojom::CORSError::kPreflightDisallowedRedirect, GetInvalidURL(),
      GetInvalidURL(), 0, HTTPHeaderMap(), *SecurityOrigin::CreateUnique(),
      WebURLRequest::kRequestContextUnspecified, String(), false);
}

// static
ErrorParameter ErrorParameter::CreateForExternalPreflightCheck(
    const network::mojom::CORSError error,
    const HTTPHeaderMap& response_header_map) {
  switch (error) {
    case network::mojom::CORSError::kPreflightMissingAllowExternal:
    case network::mojom::CORSError::kPreflightInvalidAllowExternal:
      return ErrorParameter(
          error, GetInvalidURL(), GetInvalidURL(), 0 /* status_code */,
          response_header_map, *SecurityOrigin::CreateUnique(),
          WebURLRequest::kRequestContextUnspecified, String(), false);
    default:
      NOTREACHED();
  }
  return CreateWrongParameter(error);
}

// static
ErrorParameter ErrorParameter::CreateForPreflightResponseCheck(
    const network::mojom::CORSError error,
    const String& hint) {
  switch (error) {
    case network::mojom::CORSError::kInvalidAllowMethodsPreflightResponse:
    case network::mojom::CORSError::kInvalidAllowHeadersPreflightResponse:
    case network::mojom::CORSError::kMethodDisallowedByPreflightResponse:
    case network::mojom::CORSError::kHeaderDisallowedByPreflightResponse:
      return ErrorParameter(
          error, GetInvalidURL(), GetInvalidURL(), 0 /* status_code */,
          HTTPHeaderMap(), *SecurityOrigin::CreateUnique(),
          WebURLRequest::kRequestContextUnspecified, hint, false);
    default:
      NOTREACHED();
  }
  return CreateWrongParameter(error);
}

// static
ErrorParameter ErrorParameter::CreateForRedirectCheck(
    network::mojom::CORSError error,
    const KURL& request_url,
    const KURL& redirect_url) {
  switch (error) {
    case network::mojom::CORSError::kRedirectDisallowedScheme:
    case network::mojom::CORSError::kRedirectContainsCredentials:
      return ErrorParameter(
          error, request_url, redirect_url, 0 /* status_code */,
          HTTPHeaderMap(), *SecurityOrigin::CreateUnique(),
          WebURLRequest::kRequestContextUnspecified, String(), false);
    default:
      NOTREACHED();
  }
  return CreateWrongParameter(error);
}

ErrorParameter::ErrorParameter(const network::mojom::CORSError error,
                               const KURL& first_url,
                               const KURL& second_url,
                               const int status_code,
                               const HTTPHeaderMap& header_map,
                               const SecurityOrigin& origin,
                               const WebURLRequest::RequestContext context,
                               const String& hint,
                               bool unknown)
    : error(error),
      first_url(first_url),
      second_url(second_url),
      status_code(status_code),
      header_map(header_map),
      origin(origin),
      context(context),
      hint(hint),
      unknown(unknown) {}

String GetErrorString(const ErrorParameter& param) {
  static const char kNoCorsInformation[] =
      " Have the server send the header with a valid value, or, if an opaque "
      "response serves your needs, set the request's mode to 'no-cors' to "
      "fetch the resource with CORS disabled.";
  static const char kPreflightInformation[] =
      "Response to preflight request doesn't pass access control check: ";

  if (param.unknown)
    return String::Format("CORS error, code %d", static_cast<int>(param.error));

  String redirect_denied =
      param.second_url.IsValid()
          ? String::Format(
                "Redirect from '%s' to '%s' has been blocked by CORS policy: ",
                param.first_url.GetString().Utf8().data(),
                param.second_url.GetString().Utf8().data())
          : String();

  switch (param.error) {
    case network::mojom::CORSError::kDisallowedByMode:
      return String::Format(
          "Failed to load '%s': Cross origin requests are not allowed by "
          "request mode.",
          param.first_url.GetString().Utf8().data());
    case network::mojom::CORSError::kInvalidResponse:
      return String::Format(
          "%sInvalid response. Origin '%s' is therefore not allowed access.",
          redirect_denied.Utf8().data(), param.origin.ToString().Utf8().data());
    case network::mojom::CORSError::kWildcardOriginNotAllowed:
    case network::mojom::CORSError::kPreflightWildcardOriginNotAllowed:
      return String::Format(
          "%s%sThe value of the 'Access-Control-Allow-Origin' header in the "
          "response must not be the wildcard '*' when the request's "
          "credentials mode is 'include'. Origin '%s' is therefore not allowed "
          "access.%s",
          param.error ==
                  network::mojom::CORSError::kPreflightWildcardOriginNotAllowed
              ? kPreflightInformation
              : "",
          redirect_denied.Utf8().data(), param.origin.ToString().Utf8().data(),
          param.context == WebURLRequest::kRequestContextXMLHttpRequest
              ? " The credentials mode of requests initiated by the "
                "XMLHttpRequest is controlled by the withCredentials attribute."
              : "");
    case network::mojom::CORSError::kMissingAllowOriginHeader:
    case network::mojom::CORSError::kPreflightMissingAllowOriginHeader:
      return String::Format(
          "%s%sNo 'Access-Control-Allow-Origin' header is present on the "
          "requested resource. Origin '%s' is therefore not allowed access."
          "%s%s",
          param.error ==
                  network::mojom::CORSError::kPreflightMissingAllowOriginHeader
              ? kPreflightInformation
              : "",
          redirect_denied.Utf8().data(), param.origin.ToString().Utf8().data(),
          IsInterestingStatusCode(param.status_code)
              ? String::Format(" The response had HTTP status code %d.",
                               param.status_code)
                    .Utf8()
                    .data()
              : "",
          param.context == WebURLRequest::kRequestContextFetch
              ? " If an opaque response serves your needs, set the request's "
                "mode to 'no-cors' to fetch the resource with CORS disabled."
              : "");
    case network::mojom::CORSError::kMultipleAllowOriginValues:
    case network::mojom::CORSError::kPreflightMultipleAllowOriginValues:
      return String::Format(
          "%s%sThe 'Access-Control-Allow-Origin' header contains multiple "
          "values '%s', but only one is allowed. Origin '%s' is therefore not "
          "allowed access.%s",
          param.error ==
                  network::mojom::CORSError::kPreflightMultipleAllowOriginValues
              ? kPreflightInformation
              : "",
          redirect_denied.Utf8().data(),
          param.header_map.Get(HTTPNames::Access_Control_Allow_Origin)
              .Utf8()
              .data(),
          param.origin.ToString().Utf8().data(),
          param.context == WebURLRequest::kRequestContextFetch
              ? kNoCorsInformation
              : "");
    case network::mojom::CORSError::kInvalidAllowOriginValue:
    case network::mojom::CORSError::kPreflightInvalidAllowOriginValue:
      return String::Format(
          "%s%sThe 'Access-Control-Allow-Origin' header contains the invalid "
          "value '%s'. Origin '%s' is therefore not allowed access.%s",
          param.error ==
                  network::mojom::CORSError::kPreflightInvalidAllowOriginValue
              ? kPreflightInformation
              : "",
          redirect_denied.Utf8().data(),
          param.header_map.Get(HTTPNames::Access_Control_Allow_Origin)
              .Utf8()
              .data(),
          param.origin.ToString().Utf8().data(),
          param.context == WebURLRequest::kRequestContextFetch
              ? kNoCorsInformation
              : "");
    case network::mojom::CORSError::kAllowOriginMismatch:
    case network::mojom::CORSError::kPreflightAllowOriginMismatch:
      return String::Format(
          "%s%sThe 'Access-Control-Allow-Origin' header has a value '%s' that "
          "is not equal to the supplied origin. Origin '%s' is therefore not "
          "allowed access.%s",
          param.error ==
                  network::mojom::CORSError::kPreflightAllowOriginMismatch
              ? kPreflightInformation
              : "",
          redirect_denied.Utf8().data(),
          param.header_map.Get(HTTPNames::Access_Control_Allow_Origin)
              .Utf8()
              .data(),
          param.origin.ToString().Utf8().data(),
          param.context == WebURLRequest::kRequestContextFetch
              ? kNoCorsInformation
              : "");
    case network::mojom::CORSError::kInvalidAllowCredentials:
    case network::mojom::CORSError::kPreflightInvalidAllowCredentials:
      return String::Format(
          "%s%sThe value of the 'Access-Control-Allow-Credentials' header in "
          "the response is '%s' which must be 'true' when the request's "
          "credentials mode is 'include'. Origin '%s' is therefore not allowed "
          "access.%s",
          param.error ==
                  network::mojom::CORSError::kPreflightInvalidAllowCredentials
              ? kPreflightInformation
              : "",
          redirect_denied.Utf8().data(),
          param.header_map.Get(HTTPNames::Access_Control_Allow_Credentials)
              .Utf8()
              .data(),
          param.origin.ToString().Utf8().data(),
          (param.context == WebURLRequest::kRequestContextXMLHttpRequest
               ? " The credentials mode of requests initiated by the "
                 "XMLHttpRequest is controlled by the withCredentials "
                 "attribute."
               : ""));
    case network::mojom::CORSError::kPreflightInvalidStatus:
      return String("Response for preflight does not have HTTP ok status.");
    case network::mojom::CORSError::kPreflightDisallowedRedirect:
      return String("Response for preflight is invalid (redirect)");
    case network::mojom::CORSError::kPreflightMissingAllowExternal:
      return String(
          "No 'Access-Control-Allow-External' header was present in the "
          "preflight response for this external request (This is an "
          "experimental header which is defined in "
          "'https://wicg.github.io/cors-rfc1918/').");
    case network::mojom::CORSError::kPreflightInvalidAllowExternal:
      return String::Format(
          "The 'Access-Control-Allow-External' header in the preflight "
          "response for this external request had a value of '%s',  not 'true' "
          "(This is an experimental header which is defined in "
          "'https://wicg.github.io/cors-rfc1918/').",
          param.header_map.Get(HTTPNames::Access_Control_Allow_External)
              .Utf8()
              .data());
    case network::mojom::CORSError::kInvalidAllowMethodsPreflightResponse:
      return String(
          "Cannot parse Access-Control-Allow-Methods response header field in "
          "preflight response.");
    case network::mojom::CORSError::kInvalidAllowHeadersPreflightResponse:
      return String(
          "Cannot parse Access-Control-Allow-Headers response header field in "
          "preflight response.");
    case network::mojom::CORSError::kMethodDisallowedByPreflightResponse:
      return String::Format(
          "Method %s is not allowed by Access-Control-Allow-Methods in "
          "preflight response.",
          param.hint.Utf8().data());
    case network::mojom::CORSError::kHeaderDisallowedByPreflightResponse:
      return String::Format(
          "Request header field %s is not allowed by "
          "Access-Control-Allow-Headers in preflight response.",
          param.hint.Utf8().data());
    case network::mojom::CORSError::kRedirectDisallowedScheme:
      return String::Format(
          "%sRedirect location '%s' has a disallowed scheme for cross-origin "
          "requests.",
          redirect_denied.Utf8().data(),
          param.second_url.GetString().Utf8().data());
    case network::mojom::CORSError::kRedirectContainsCredentials:
      return String::Format(
          "%sRedirect location '%s' contains a username and password, which is "
          "disallowed for cross-origin requests.",
          redirect_denied.Utf8().data(),
          param.second_url.GetString().Utf8().data());
  }
  NOTREACHED();
  return String();
}

}  // namespace CORS

}  // namespace blink
