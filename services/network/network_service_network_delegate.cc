// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/network_service_network_delegate.h"

#include "net/base/static_cookie_policy.h"
#include "services/network/network_context.h"

namespace network {

NetworkServiceNetworkDelegate::NetworkServiceNetworkDelegate(
    NetworkContext* network_context)
    : network_context_(network_context) {}

bool NetworkServiceNetworkDelegate::OnCanGetCookies(
    const net::URLRequest& request,
    const net::CookieList& cookie_list) {
  net::StaticCookiePolicy::Type policy_type =
      network_context_->block_third_party_cookies()
          ? net::StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES
          : net::StaticCookiePolicy::ALLOW_ALL_COOKIES;
  net::StaticCookiePolicy policy(policy_type);
  int rv = policy.CanAccessCookies(request.url(), request.site_for_cookies());
  return rv == net::OK;
}

bool NetworkServiceNetworkDelegate::OnCanSetCookie(
    const net::URLRequest& request,
    const net::CanonicalCookie& cookie,
    net::CookieOptions* options) {
  net::StaticCookiePolicy::Type policy_type =
      network_context_->block_third_party_cookies()
          ? net::StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES
          : net::StaticCookiePolicy::ALLOW_ALL_COOKIES;
  net::StaticCookiePolicy policy(policy_type);
  int rv = policy.CanAccessCookies(request.url(), request.site_for_cookies());
  return rv == net::OK;
}

bool NetworkServiceNetworkDelegate::OnCanAccessFile(
    const net::URLRequest& request,
    const base::FilePath& original_path,
    const base::FilePath& absolute_path) const {
  // Match the default implementation (BasicNetworkDelegate)'s behavior for
  // now.
  return true;
}

}  // namespace network
