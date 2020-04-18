// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_COOKIE_ACCESS_POLICY_H_
#define ANDROID_WEBVIEW_BROWSER_AW_COOKIE_ACCESS_POLICY_H_

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "net/base/static_cookie_policy.h"
#include "net/cookies/canonical_cookie.h"
#include "net/url_request/url_request.h"

namespace content {
class ResourceContext;
}

namespace net {
class CookieOptions;
}

class GURL;

namespace android_webview {

// Manages the cookie access (both setting and getting) policy for WebView.
// Currently we don't distinguish between sources (i.e. network vs. JavaScript)
// or between reading vs. writing cookies.
class AwCookieAccessPolicy {
 public:
  static AwCookieAccessPolicy* GetInstance();

  // Can we read/write any cookies?
  bool GetShouldAcceptCookies();
  void SetShouldAcceptCookies(bool allow);

  // Can we read/write third party cookies?
  // |render_process_id| and |render_frame_id| must be valid.
  // Navigation requests are not associated with a renderer process. In this
  // case, |frame_tree_node_id| must be valid instead.
  bool GetShouldAcceptThirdPartyCookies(int render_process_id,
                                        int render_frame_id,
                                        int frame_tree_node_id);
  bool GetShouldAcceptThirdPartyCookies(const net::URLRequest& request);

  // These are the functions called when operating over cookies from the
  // network. See NetworkDelegate for further descriptions.
  bool OnCanGetCookies(const net::URLRequest& request,
                       const net::CookieList& cookie_list);
  bool OnCanSetCookie(const net::URLRequest& request,
                      const net::CanonicalCookie& cookie,
                      net::CookieOptions* options);

  // These are the functions called when operating over cookies from the
  // renderer. See ContentBrowserClient for further descriptions.
  bool AllowGetCookie(const GURL& url,
                      const GURL& first_party,
                      const net::CookieList& cookie_list,
                      content::ResourceContext* context,
                      int render_process_id,
                      int render_frame_id);
  bool AllowSetCookie(const GURL& url,
                      const GURL& first_party,
                      const net::CanonicalCookie& cookie,
                      content::ResourceContext* context,
                      int render_process_id,
                      int render_frame_id,
                      const net::CookieOptions& options);

 private:
  friend struct base::LazyInstanceTraitsBase<AwCookieAccessPolicy>;

  AwCookieAccessPolicy();
  ~AwCookieAccessPolicy();
  bool accept_cookies_;
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(AwCookieAccessPolicy);
};

class AwStaticCookiePolicy {
 public:
  AwStaticCookiePolicy(bool allow_global_access,
                       bool allow_third_party_access);

  bool accept_cookies() const {
    return accept_cookies_;
  }

  bool accept_third_party_cookies() const {
    return accept_third_party_cookies_;
  }

  bool AllowGet(const GURL& url, const GURL& first_party) const;
  bool AllowSet(const GURL& url, const GURL& first_party) const;

 private:
  const bool accept_cookies_;
  const bool accept_third_party_cookies_;

  // We have two bits of state but only three different cases:
  // If !ShouldAcceptCookies
  //    then reject all cookies.
  // If ShouldAcceptCookies and !ShouldAcceptThirdPartyCookies
  //    then reject third party.
  // If ShouldAcceptCookies and ShouldAcceptThirdPartyCookies
  //    then allow all cookies.
  net::StaticCookiePolicy::Type GetPolicy(const GURL& url) const;

  DISALLOW_COPY_AND_ASSIGN(AwStaticCookiePolicy);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_COOKIE_ACCESS_POLICY_H_
