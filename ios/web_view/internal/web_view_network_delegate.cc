// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/web_view_network_delegate.h"

#include "net/base/net_errors.h"

namespace ios_web_view {

WebViewNetworkDelegate::WebViewNetworkDelegate() {}

WebViewNetworkDelegate::~WebViewNetworkDelegate() = default;

int WebViewNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  return net::OK;
}

int WebViewNetworkDelegate::OnBeforeStartTransaction(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  return net::OK;
}

void WebViewNetworkDelegate::OnStartTransaction(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {}

int WebViewNetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return net::OK;
}

void WebViewNetworkDelegate::OnBeforeRedirect(net::URLRequest* request,
                                              const GURL& new_location) {}

void WebViewNetworkDelegate::OnResponseStarted(net::URLRequest* request,
                                               int net_error) {}

void WebViewNetworkDelegate::OnNetworkBytesReceived(net::URLRequest* request,
                                                    int64_t bytes_received) {}

void WebViewNetworkDelegate::OnCompleted(net::URLRequest* request,
                                         bool started) {}

void WebViewNetworkDelegate::OnURLRequestDestroyed(net::URLRequest* request) {}

void WebViewNetworkDelegate::OnPACScriptError(int line_number,
                                              const base::string16& error) {}

WebViewNetworkDelegate::AuthRequiredResponse
WebViewNetworkDelegate::OnAuthRequired(net::URLRequest* request,
                                       const net::AuthChallengeInfo& auth_info,
                                       const AuthCallback& callback,
                                       net::AuthCredentials* credentials) {
  return AUTH_REQUIRED_RESPONSE_NO_ACTION;
}

bool WebViewNetworkDelegate::OnCanGetCookies(
    const net::URLRequest& request,
    const net::CookieList& cookie_list) {
  return true;
}

bool WebViewNetworkDelegate::OnCanSetCookie(const net::URLRequest& request,
                                            const net::CanonicalCookie& cookie,
                                            net::CookieOptions* options) {
  return true;
}

bool WebViewNetworkDelegate::OnCanAccessFile(
    const net::URLRequest& request,
    const base::FilePath& original_path,
    const base::FilePath& absolute_path) const {
  return true;
}

}  // namespace ios_web_view
