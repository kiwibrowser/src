// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/tabs/tab_model_web_usage_enabled_observer.h"

#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Sets |web_state| web usage enabled property and starts loading the content
// if necessary.
void SetWebUsageEnabled(web::WebState* web_state, bool web_usage_enabled) {
  web_state->SetWebUsageEnabled(web_usage_enabled);
  if (web_usage_enabled)
    web_state->GetNavigationManager()->LoadIfNecessary();
}

}  // namespace

TabModelWebUsageEnabledObserver::TabModelWebUsageEnabledObserver(
    TabModel* tab_model)
    : tab_model_(tab_model) {}

TabModelWebUsageEnabledObserver::~TabModelWebUsageEnabledObserver() = default;

void TabModelWebUsageEnabledObserver::WebStateInsertedAt(
    WebStateList* web_state_list,
    web::WebState* web_state,
    int index,
    bool activating) {
  SetWebUsageEnabled(web_state, tab_model_.webUsageEnabled);
}

void TabModelWebUsageEnabledObserver::WebStateReplacedAt(
    WebStateList* web_state_list,
    web::WebState* old_web_state,
    web::WebState* new_web_state,
    int index) {
  SetWebUsageEnabled(new_web_state, tab_model_.webUsageEnabled);
}
