// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell_network_delegate.h"

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "content/public/common/content_switches.h"
#include "net/base/net_errors.h"
#include "net/base/static_cookie_policy.h"
#include "net/url_request/url_request.h"

namespace content {

namespace {
bool g_block_third_party_cookies = false;
bool g_cancel_requests_with_referrer_policy_violation = false;
}

ShellNetworkDelegate::ShellNetworkDelegate() {
}

ShellNetworkDelegate::~ShellNetworkDelegate() {
}

void ShellNetworkDelegate::SetBlockThirdPartyCookies(bool block) {
  g_block_third_party_cookies = block;
}

void ShellNetworkDelegate::SetCancelURLRequestWithPolicyViolatingReferrerHeader(
    bool cancel) {
  g_cancel_requests_with_referrer_policy_violation = cancel;
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
  net::StaticCookiePolicy::Type policy_type = g_block_third_party_cookies ?
      net::StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES :
      net::StaticCookiePolicy::ALLOW_ALL_COOKIES;
  net::StaticCookiePolicy policy(policy_type);
  int rv = policy.CanAccessCookies(request.url(), request.site_for_cookies());
  return rv == net::OK;
}

bool ShellNetworkDelegate::OnCanSetCookie(const net::URLRequest& request,
                                          const net::CanonicalCookie& cookie,
                                          net::CookieOptions* options) {
  net::StaticCookiePolicy::Type policy_type = g_block_third_party_cookies ?
      net::StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES :
      net::StaticCookiePolicy::ALLOW_ALL_COOKIES;
  net::StaticCookiePolicy policy(policy_type);
  int rv = policy.CanAccessCookies(request.url(), request.site_for_cookies());
  return rv == net::OK;
}

bool ShellNetworkDelegate::OnCanAccessFile(
    const net::URLRequest& request,
    const base::FilePath& original_path,
    const base::FilePath& absolute_path) const {
  return true;
}

bool ShellNetworkDelegate::OnAreExperimentalCookieFeaturesEnabled() const {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableExperimentalWebPlatformFeatures);
}

bool ShellNetworkDelegate::OnCancelURLRequestWithPolicyViolatingReferrerHeader(
    const net::URLRequest& request,
    const GURL& target_url,
    const GURL& referrer_url) const {
  return g_cancel_requests_with_referrer_policy_violation;
}

}  // namespace content
