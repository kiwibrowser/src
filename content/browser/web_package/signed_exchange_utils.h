// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_UTILS_H_
#define CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_UTILS_H_

#include <string>

class GURL;

namespace network {
struct ResourceResponseHead;
}  // namespace network

namespace content {

class SignedExchangeDevToolsProxy;

namespace signed_exchange_utils {

// Utility method to call SignedExchangeDevToolsProxy::ReportErrorMessage() and
// TRACE_EVENT_END() to report the error to both DevTools and about:tracing. If
// |devtools_proxy| is nullptr, it just calls TRACE_EVENT_END().
void ReportErrorAndEndTraceEvent(SignedExchangeDevToolsProxy* devtools_proxy,
                                 const char* trace_event_name,
                                 const std::string& error_message);

// Returns true when SignedHTTPExchange feature or SignedHTTPExchangeOriginTrial
// feature is enabled.
bool IsSignedExchangeHandlingEnabled();

// Returns true when the response should be handled as a signed exchange by
// checking the mime type and the feature flags. When SignedHTTPExchange feature
// is not enabled and SignedHTTPExchangeOriginTrial feature is enabled, this
// method also checks the Origin Trial header.
bool ShouldHandleAsSignedHTTPExchange(
    const GURL& request_url,
    const network::ResourceResponseHead& head);

}  // namespace  signed_exchange_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_UTILS_H_
