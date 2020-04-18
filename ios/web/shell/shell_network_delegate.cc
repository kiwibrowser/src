// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/shell/shell_network_delegate.h"

#include "net/base/net_errors.h"

namespace web {

ShellNetworkDelegate::ShellNetworkDelegate() {
}

ShellNetworkDelegate::~ShellNetworkDelegate() {
}

int ShellNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  return net::OK;
}

int ShellNetworkDelegate::OnBeforeStartTransaction(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  return net::OK;
}

void ShellNetworkDelegate::OnStartTransaction(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {}

int ShellNetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return net::OK;
}

void ShellNetworkDelegate::OnBeforeRedirect(net::URLRequest* request,
                                            const GURL& new_location) {
}

void ShellNetworkDelegate::OnResponseStarted(net::URLRequest* request,
                                             int net_error) {}

void ShellNetworkDelegate::OnCompleted(net::URLRequest* request,
                                       bool started,
                                       int net_error) {}

void ShellNetworkDelegate::OnURLRequestDestroyed(net::URLRequest* request) {
}

void ShellNetworkDelegate::OnPACScriptError(int line_number,
                                            const base::string16& error) {
}

ShellNetworkDelegate::AuthRequiredResponse ShellNetworkDelegate::OnAuthRequired(
    net::URLRequest* request,
    const net::AuthChallengeInfo& auth_info,
    const AuthCallback& callback,
    net::AuthCredentials* credentials) {
  return AUTH_REQUIRED_RESPONSE_NO_ACTION;
}

bool ShellNetworkDelegate::OnCanGetCookies(const net::URLRequest& request,
                                           const net::CookieList& cookie_list) {
  return true;
}

bool ShellNetworkDelegate::OnCanSetCookie(const net::URLRequest& request,
                                          const net::CanonicalCookie& cookie,
                                          net::CookieOptions* options) {
  return true;
}

bool ShellNetworkDelegate::OnCanAccessFile(
    const net::URLRequest& request,
    const base::FilePath& original_path,
    const base::FilePath& absolute_path) const {
  return true;
}

}  // namespace web
