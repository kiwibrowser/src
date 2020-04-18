// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/tabs/tab_model_notification_observer.h"

#import "ios/chrome/browser/tabs/legacy_tab_helper.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

TabModelNotificationObserver::TabModelNotificationObserver(TabModel* tab_model)
    : tab_model_(tab_model) {}

TabModelNotificationObserver::~TabModelNotificationObserver() = default;

void TabModelNotificationObserver::SetDisabled(bool disabled) {
  disabled_ = disabled;
}

void TabModelNotificationObserver::WebStateInsertedAt(
    WebStateList* web_state_list,
    web::WebState* web_state,
    int index,
    bool activating) {
  if (disabled_)
    return;

  Tab* tab = LegacyTabHelper::GetTabForWebState(web_state);
  [tab_model_ notifyNewTabWillOpen:tab inBackground:!activating];
}

