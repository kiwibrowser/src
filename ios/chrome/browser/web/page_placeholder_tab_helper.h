// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_PAGE_PLACEHOLDER_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_WEB_PAGE_PLACEHOLDER_TAB_HELPER_H_

#import <UIKit/UIKit.h>

#include "base/macros.h"
#include "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

// Displays placeholder to cover what WebState is actually displaying. Can be
// used to display the cached image of the web page during the Tab restoration.
class PagePlaceholderTabHelper
    : public web::WebStateUserData<PagePlaceholderTabHelper>,
      public web::WebStateObserver {
 public:
  ~PagePlaceholderTabHelper() override;

  // Displays placeholder between DidStartNavigation and PageLoaded
  // WebStateObserver callbacks. If navigation takes too long, then placeholder
  // will be removed before navigation is finished.
  void AddPlaceholderForNextNavigation();

  // Cancels displaying placeholder during the next navigation. If placeholder
  // is displayed, then it is removed.
  void CancelPlaceholderForNextNavigation();

  // true if placeholder is currently being displayed.
  bool displaying_placeholder() const { return displaying_placeholder_; }

  // true if placeholder will be displayed between DidStartNavigation and
  // PageLoaded WebStateObserver callbacks.
  bool will_add_placeholder_for_next_navigation() const {
    return add_placeholder_for_next_navigation_;
  }

 private:
  friend class web::WebStateUserData<PagePlaceholderTabHelper>;

  explicit PagePlaceholderTabHelper(web::WebState* web_state);

  // web::WebStateObserver overrides:
  void DidStartNavigation(web::WebState* web_state,
                          web::NavigationContext* navigation_context) override;
  void PageLoaded(
      web::WebState* web_state,
      web::PageLoadCompletionStatus load_completion_status) override;
  void WebStateDestroyed(web::WebState* web_state) override;

  void AddPlaceholder();
  void RemovePlaceholder();

  // WebState this tab helper is attached to.
  web::WebState* web_state_ = nullptr;

  // View used to display the placeholder.
  UIImageView* placeholder_view_ = nil;

  // true if placeholder is currently being displayed.
  bool displaying_placeholder_ = false;

  // true if placeholder must be displayed during the next navigation.
  bool add_placeholder_for_next_navigation_ = false;

  base::WeakPtrFactory<PagePlaceholderTabHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PagePlaceholderTabHelper);
};

#endif  // IOS_CHROME_BROWSER_WEB_PAGE_PLACEHOLDER_TAB_HELPER_H_
