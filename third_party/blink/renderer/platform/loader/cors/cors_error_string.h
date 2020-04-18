// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_CORS_CORS_ERROR_STRING_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_CORS_CORS_ERROR_STRING_H_

#include "base/macros.h"
#include "services/network/public/cpp/cors/cors_error_status.h"
#include "services/network/public/mojom/cors.mojom-shared.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

class HTTPHeaderMap;
class SecurityOrigin;

// CORS error strings related utility functions.
namespace CORS {

// A struct to pass error dependent arguments for |GetErrorString|.
struct PLATFORM_EXPORT ErrorParameter {
  // Creates an ErrorParameter for generic cases. Use this function if |error|
  // can contain any.
  static ErrorParameter Create(const network::CORSErrorStatus&,
                               const KURL& first_url,
                               const KURL& second_url,
                               const int status_code,
                               const HTTPHeaderMap&,
                               const SecurityOrigin&,
                               const WebURLRequest::RequestContext);

  // Creates an ErrorParameter for kDisallowedByMode.
  static ErrorParameter CreateForDisallowedByMode(const KURL& request_url);

  // Creates an ErrorParameter for kInvalidResponse.
  static ErrorParameter CreateForInvalidResponse(const KURL& request_url,
                                                 const SecurityOrigin&);

  // Creates an ErrorParameter for an error that CORS::CheckAccess() returns.
  // |error| for redirect check needs to specify a valid |redirect_url|. The
  // |redirect_url| can be omitted not to include redirect related information.
  static ErrorParameter CreateForAccessCheck(
      const network::mojom::CORSError,
      const KURL& request_url,
      int response_status_code,
      const HTTPHeaderMap& response_header_map,
      const SecurityOrigin&,
      const WebURLRequest::RequestContext,
      const KURL& redirect_url = KURL());

  // Creates an ErrorParameter for kPreflightInvalidStatus that
  // CORS::CheckPreflight() returns.
  static ErrorParameter CreateForPreflightStatusCheck(int response_status_code);

  // Creates an ErrorParameter for kPreflightDisallowedRedirect.
  static ErrorParameter CreateForDisallowedRedirect();

  // Creates an ErrorParameter for an error that CORS::CheckExternalPreflight()
  // returns.
  static ErrorParameter CreateForExternalPreflightCheck(
      const network::mojom::CORSError,
      const HTTPHeaderMap& response_header_map);

  // Creates an ErrorParameter for an error that is related to CORS-preflight
  // response checks.
  // |hint| should contain a banned request method for
  // kMethodDisallowedByPreflightResponse, a banned request header name for
  // kHeaderDisallowedByPreflightResponse, or can be omitted for others.
  static ErrorParameter CreateForPreflightResponseCheck(
      const network::mojom::CORSError,
      const String& hint);

  // Creates an ErrorParameter for CORS::CheckRedirectLocation() returns.
  static ErrorParameter CreateForRedirectCheck(network::mojom::CORSError,
                                               const KURL& request_url,
                                               const KURL& redirect_url);

  // Should not be used directly by external callers. Use Create functions
  // above.
  ErrorParameter(const network::mojom::CORSError,
                 const KURL& first_url,
                 const KURL& second_url,
                 const int status_code,
                 const HTTPHeaderMap&,
                 const SecurityOrigin&,
                 const WebURLRequest::RequestContext,
                 const String& hint,
                 bool unknown);

  // Members that this struct carries.
  const network::mojom::CORSError error;
  const KURL& first_url;
  const KURL& second_url;
  const int status_code;
  const HTTPHeaderMap& header_map;
  const SecurityOrigin& origin;
  const WebURLRequest::RequestContext context;
  const String& hint;

  // Set to true when an ErrorParameter was created in a wrong way. Used in
  // GetErrorString() to be robust for coding errors.
  const bool unknown;
};

// Stringify CORSError mainly for inspector messages. Generated string should
// not be exposed to JavaScript for security reasons.
PLATFORM_EXPORT String GetErrorString(const ErrorParameter&);

}  // namespace CORS

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_CORS_CORS_ERROR_STRING_H_
