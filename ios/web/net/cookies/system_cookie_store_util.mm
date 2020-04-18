// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/system_cookie_store_util.h"

#import <WebKit/WebKit.h>

#import "ios/net/cookies/ns_http_system_cookie_store.h"
#include "ios/net/cookies/system_cookie_store.h"
#import "ios/web/net/cookies/wk_cookie_util.h"
#import "ios/web/net/cookies/wk_http_system_cookie_store.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/features.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

std::unique_ptr<net::SystemCookieStore> CreateSystemCookieStore(
    BrowserState* browser_state) {
  if (@available(iOS 11, *)) {
    if (base::FeatureList::IsEnabled(web::features::kWKHTTPSystemCookieStore)) {
      // Using WKHTTPCookieStore guarantee that cookies are always in sync and
      // allows SystemCookieStore to handle cookies for OffTheRecord browser.
      WKHTTPCookieStore* wk_cookie_store =
          web::WKCookieStoreForBrowserState(browser_state);
      return std::make_unique<web::WKHTTPSystemCookieStore>(wk_cookie_store);
    }
  }
  // TODO(crbug.com/759229): Return a different CookieStore for OffTheRecord
  // browser state.
  return std::make_unique<net::NSHTTPSystemCookieStore>();
}

}  // namespace web
