// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_WEB_STATE_OBSERVER_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_WEB_STATE_OBSERVER_H_

#include <memory>

#include "ios/web/public/web_state/web_state_observer.h"

class FullscreenController;
class FullscreenMediator;
class FullscreenModel;
@class FullscreenWebViewProxyObserver;
class ScopedFullscreenDisabler;

// A WebStateObserver that updates a FullscreenModel for navigation events.
class FullscreenWebStateObserver : public web::WebStateObserver {
 public:
  // Constructor for an observer that updates |controller| and |model|.
  FullscreenWebStateObserver(FullscreenController* controller,
                             FullscreenModel* model,
                             FullscreenMediator* mediator);
  ~FullscreenWebStateObserver() override;

  // Tells the observer to start observing |web_state|.
  void SetWebState(web::WebState* web_state);

 private:
  // WebStateObserver:
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void DidStartLoading(web::WebState* web_state) override;
  void DidStopLoading(web::WebState* web_state) override;
  void DidChangeVisibleSecurityState(web::WebState* web_state) override;
  void WebStateDestroyed(web::WebState* web_state) override;
  // Setter for whether the current page's SSL should disable fullscreen.
  void SetDisableFullscreenForSSL(bool disable);
  // Setter for whether the WebState is currently loading.
  void SetIsLoading(bool loading);

  // The WebState being observed.
  web::WebState* web_state_ = nullptr;
  // The FullscreenController passed on construction.
  FullscreenController* controller_;
  // The model passed on construction.
  FullscreenModel* model_;
  // Observer for |web_state_|'s scroll view proxy.
  __strong FullscreenWebViewProxyObserver* web_view_proxy_observer_;
  // The disabler for invalid SSL states.
  std::unique_ptr<ScopedFullscreenDisabler> ssl_disabler_;
  // The disabler for loading.
  std::unique_ptr<ScopedFullscreenDisabler> loading_disabler_;
};

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_WEB_STATE_OBSERVER_H_
