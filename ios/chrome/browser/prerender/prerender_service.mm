// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/prerender/prerender_service.h"

#import "ios/chrome/browser/prerender/preload_controller.h"
#import "ios/chrome/browser/tabs/legacy_tab_helper.h"
#import "ios/chrome/browser/tabs/tab.h"
#include "ios/web/public/web_client.h"
#include "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

PrerenderService::PrerenderService(ios::ChromeBrowserState* browser_state)
    : controller_(
          [[PreloadController alloc] initWithBrowserState:browser_state]) {}

PrerenderService::~PrerenderService() {}

void PrerenderService::Shutdown() {
  [controller_ browserStateDestroyed];
  controller_ = nil;
}

void PrerenderService::SetDelegate(id<PreloadControllerDelegate> delegate) {
  controller_.delegate = delegate;
}

void PrerenderService::StartPrerender(const GURL& url,
                                      const web::Referrer& referrer,
                                      ui::PageTransition transition,
                                      bool immediately) {
  // PrerenderService is not compatible with WKBasedNavigationManager because it
  // loads the URL in a new WKWebView, which doesn't have the current session
  // history. TODO(crbug.com/814789): decide whether PrerenderService needs to
  // be supported after evaluating the performance impact in Finch experiment.
  if (web::GetWebClient()->IsSlimNavigationManagerEnabled())
    return;

  [controller_ prerenderURL:url
                   referrer:referrer
                 transition:transition
                immediately:immediately];
}

void PrerenderService::CancelPrerender() {
  [controller_ cancelPrerender];
}

bool PrerenderService::HasPrerenderForUrl(const GURL& url) {
  return url == controller_.prerenderedURL;
}

bool PrerenderService::IsWebStatePrerendered(web::WebState* web_state) {
  return [controller_ isWebStatePrerendered:web_state];
}

std::unique_ptr<web::WebState> PrerenderService::ReleasePrerenderContents() {
  return [controller_ releasePrerenderContents];
}
