// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Helper classes and functions used for the WebRequest API.

#ifndef EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_API_HELPERS_H_
#define EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_API_HELPERS_H_

#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "extensions/browser/warning_set.h"
#include "net/base/auth.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

namespace base {
class ListValue;
class DictionaryValue;
}

namespace extensions {
class Extension;
}

namespace extension_web_request_api_helpers {

typedef std::pair<std::string, std::string> ResponseHeader;
typedef std::vector<ResponseHeader> ResponseHeaders;

// Internal representation of the extraInfoSpec parameter on webRequest
// events, used to specify extra information to be included with network
// events.
struct ExtraInfoSpec {
  enum Flags {
    REQUEST_HEADERS = 1 << 0,
    RESPONSE_HEADERS = 1 << 1,
    BLOCKING = 1 << 2,
    ASYNC_BLOCKING = 1 << 3,
    REQUEST_BODY = 1 << 4,
  };

  static bool InitFromValue(const base::ListValue& value, int* extra_info_spec);
};

// Data container for RequestCookies as defined in the declarative WebRequest
// API definition.
struct RequestCookie {
  RequestCookie();
  ~RequestCookie();
  std::unique_ptr<std::string> name;
  std::unique_ptr<std::string> value;

 private:
  DISALLOW_COPY_AND_ASSIGN(RequestCookie);
};

bool NullableEquals(const RequestCookie* a, const RequestCookie* b);

// Data container for ResponseCookies as defined in the declarative WebRequest
// API definition.
struct ResponseCookie {
  ResponseCookie();
  ~ResponseCookie();
  std::unique_ptr<std::string> name;
  std::unique_ptr<std::string> value;
  std::unique_ptr<std::string> expires;
  std::unique_ptr<int> max_age;
  std::unique_ptr<std::string> domain;
  std::unique_ptr<std::string> path;
  std::unique_ptr<bool> secure;
  std::unique_ptr<bool> http_only;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResponseCookie);
};

bool NullableEquals(const ResponseCookie* a, const ResponseCookie* b);

// Data container for FilterResponseCookies as defined in the declarative
// WebRequest API definition.
struct FilterResponseCookie : ResponseCookie {
  FilterResponseCookie();
  ~FilterResponseCookie();
  std::unique_ptr<int> age_lower_bound;
  std::unique_ptr<int> age_upper_bound;
  std::unique_ptr<bool> session_cookie;

 private:
  DISALLOW_COPY_AND_ASSIGN(FilterResponseCookie);
};

bool NullableEquals(const FilterResponseCookie* a,
                    const FilterResponseCookie* b);

enum CookieModificationType {
  ADD,
  EDIT,
  REMOVE,
};

struct RequestCookieModification {
  RequestCookieModification();
  ~RequestCookieModification();
  CookieModificationType type;
  // Used for EDIT and REMOVE. NULL for ADD.
  std::unique_ptr<RequestCookie> filter;
  // Used for ADD and EDIT. NULL for REMOVE.
  std::unique_ptr<RequestCookie> modification;

 private:
  DISALLOW_COPY_AND_ASSIGN(RequestCookieModification);
};

bool NullableEquals(const RequestCookieModification* a,
                    const RequestCookieModification* b);

struct ResponseCookieModification {
  ResponseCookieModification();
  ~ResponseCookieModification();
  CookieModificationType type;
  // Used for EDIT and REMOVE.
  std::unique_ptr<FilterResponseCookie> filter;
  // Used for ADD and EDIT.
  std::unique_ptr<ResponseCookie> modification;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResponseCookieModification);
};

bool NullableEquals(const ResponseCookieModification* a,
                    const ResponseCookieModification* b);

typedef std::vector<linked_ptr<RequestCookieModification> >
    RequestCookieModifications;
typedef std::vector<linked_ptr<ResponseCookieModification> >
    ResponseCookieModifications;

// Contains the modification an extension wants to perform on an event.
struct EventResponseDelta {
  EventResponseDelta(const std::string& extension_id,
                     const base::Time& extension_install_time);
  ~EventResponseDelta();

  // ID of the extension that sent this response.
  std::string extension_id;

  // The time that the extension was installed. Used for deciding order of
  // precedence in case multiple extensions respond with conflicting
  // decisions.
  base::Time extension_install_time;

  // Response values. These are mutually exclusive.
  bool cancel;
  GURL new_url;

  // Newly introduced or overridden request headers.
  net::HttpRequestHeaders modified_request_headers;

  // Keys of request headers to be deleted.
  std::vector<std::string> deleted_request_headers;

  // Headers that were added to the response. A modification of a header
  // corresponds to a deletion and subsequent addition of the new header.
  ResponseHeaders added_response_headers;

  // Headers that were deleted from the response.
  ResponseHeaders deleted_response_headers;

  // Authentication Credentials to use.
  std::unique_ptr<net::AuthCredentials> auth_credentials;

  // Modifications to cookies in request headers.
  RequestCookieModifications request_cookie_modifications;

  // Modifications to cookies in response headers.
  ResponseCookieModifications response_cookie_modifications;

  // Messages that shall be sent to the background/event/... pages of the
  // extension.
  std::set<std::string> messages_to_extension;

  DISALLOW_COPY_AND_ASSIGN(EventResponseDelta);
};

typedef std::list<linked_ptr<EventResponseDelta> > EventResponseDeltas;

// Comparison operator that returns true if the extension that caused
// |a| was installed after the extension that caused |b|.
bool InDecreasingExtensionInstallationTimeOrder(
    const linked_ptr<EventResponseDelta>& a,
    const linked_ptr<EventResponseDelta>& b);

// Converts a string to a list of integers, each in 0..255.
std::unique_ptr<base::ListValue> StringToCharList(const std::string& s);

// Converts a list of integer values between 0 and 255 into a string |*out|.
// Returns true if the conversion was successful.
bool CharListToString(const base::ListValue* list, std::string* out);

// The following functions calculate and return the modifications to requests
// commanded by extension handlers. All functions take the id of the extension
// that commanded a modification, the installation time of this extension (used
// for defining a precedence in conflicting modifications) and whether the
// extension requested to |cancel| the request. Other parameters depend on a
// the signal handler. Ownership of the returned object is passed to the caller.

EventResponseDelta* CalculateOnBeforeRequestDelta(
    const std::string& extension_id,
    const base::Time& extension_install_time,
    bool cancel,
    const GURL& new_url);
EventResponseDelta* CalculateOnBeforeSendHeadersDelta(
    const std::string& extension_id,
    const base::Time& extension_install_time,
    bool cancel,
    net::HttpRequestHeaders* old_headers,
    net::HttpRequestHeaders* new_headers);
EventResponseDelta* CalculateOnHeadersReceivedDelta(
    const std::string& extension_id,
    const base::Time& extension_install_time,
    bool cancel,
    const GURL& old_url,
    const GURL& new_url,
    const net::HttpResponseHeaders* old_response_headers,
    ResponseHeaders* new_response_headers);
// Destructively moves the auth credentials from |auth_credentials| to the
// returned EventResponseDelta.
EventResponseDelta* CalculateOnAuthRequiredDelta(
    const std::string& extension_id,
    const base::Time& extension_install_time,
    bool cancel,
    std::unique_ptr<net::AuthCredentials>* auth_credentials);

// These functions merge the responses (the |deltas|) of request handlers.
// The |deltas| need to be sorted in decreasing order of precedence of
// extensions. In case extensions had |deltas| that could not be honored, their
// IDs are reported in |conflicting_extensions|. NetLog events that shall be
// reported will be stored in |event_log_entries|.

// Stores in |canceled| whether any extension wanted to cancel the request.
void MergeCancelOfResponses(const EventResponseDeltas& deltas,
                            bool* canceled,
                            extensions::WebRequestInfo::Logger* logger);
// Stores in |*new_url| the redirect request of the extension with highest
// precedence. Extensions that did not command to redirect the request are
// ignored in this logic.
void MergeRedirectUrlOfResponses(const GURL& url,
                                 const EventResponseDeltas& deltas,
                                 GURL* new_url,
                                 extensions::WarningSet* conflicting_extensions,
                                 extensions::WebRequestInfo::Logger* logger);
// Stores in |*new_url| the redirect request of the extension with highest
// precedence. Extensions that did not command to redirect the request are
// ignored in this logic.
void MergeOnBeforeRequestResponses(
    const GURL& url,
    const EventResponseDeltas& deltas,
    GURL* new_url,
    extensions::WarningSet* conflicting_extensions,
    extensions::WebRequestInfo::Logger* logger);
// Modifies the "Cookie" header in |request_headers| according to
// |deltas.request_cookie_modifications|. Conflicts are currently ignored
// silently.
void MergeCookiesInOnBeforeSendHeadersResponses(
    const GURL& gurl,
    const EventResponseDeltas& deltas,
    net::HttpRequestHeaders* request_headers,
    extensions::WarningSet* conflicting_extensions,
    extensions::WebRequestInfo::Logger* logger);
// Modifies the headers in |request_headers| according to |deltas|. Conflicts
// are tried to be resolved.
// Stores in |request_headers_modified| whether the request headers were
// modified.
void MergeOnBeforeSendHeadersResponses(
    const GURL& url,
    const EventResponseDeltas& deltas,
    net::HttpRequestHeaders* request_headers,
    extensions::WarningSet* conflicting_extensions,
    extensions::WebRequestInfo::Logger* logger,
    bool* request_headers_modified);
// Modifies the "Set-Cookie" headers in |override_response_headers| according to
// |deltas.response_cookie_modifications|. If |override_response_headers| is
// NULL, a copy of |original_response_headers| is created. Conflicts are
// currently ignored silently.
void MergeCookiesInOnHeadersReceivedResponses(
    const GURL& url,
    const EventResponseDeltas& deltas,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    extensions::WarningSet* conflicting_extensions,
    extensions::WebRequestInfo::Logger* logger);
// Stores a copy of |original_response_header| into |override_response_headers|
// that is modified according to |deltas|. If |deltas| does not instruct to
// modify the response headers, |override_response_headers| remains empty.
// Extension-initiated redirects are written to |override_response_headers|
// (to request redirection) and |*allowed_unsafe_redirect_url| (to make sure
// that the request is not cancelled with net::ERR_UNSAFE_REDIRECT).
// Stores in |response_headers_modified| whether the response headers were
// modified.
void MergeOnHeadersReceivedResponses(
    const GURL& url,
    const EventResponseDeltas& deltas,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url,
    extensions::WarningSet* conflicting_extensions,
    extensions::WebRequestInfo::Logger* logger,
    bool* response_headers_modified);
// Merge the responses of blocked onAuthRequired handlers. The first
// registered listener that supplies authentication credentials in a response,
// if any, will have its authentication credentials used. |request| must be
// non-NULL, and contain |deltas| that are sorted in decreasing order of
// precedence.
// Returns whether authentication credentials are set.
bool MergeOnAuthRequiredResponses(
    const EventResponseDeltas& deltas,
    net::AuthCredentials* auth_credentials,
    extensions::WarningSet* conflicting_extensions,
    extensions::WebRequestInfo::Logger* logger);

// Triggers clearing each renderer's in-memory cache the next time it navigates.
void ClearCacheOnNavigation();

// Converts the |name|, |value| pair of a http header to a HttpHeaders
// dictionary.
std::unique_ptr<base::DictionaryValue> CreateHeaderDictionary(
    const std::string& name,
    const std::string& value);

}  // namespace extension_web_request_api_helpers

#endif  // EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_API_HELPERS_H_
