// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/tabs/tab_model_favicon_driver_observer.h"

#include "base/logging.h"
#include "components/favicon/ios/web_favicon_driver.h"
#import "ios/chrome/browser/tabs/legacy_tab_helper.h"
#import "ios/chrome/browser/tabs/tab_model_observers.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

TabModelFaviconDriverObserver::TabModelFaviconDriverObserver(
    TabModel* tab_model,
    TabModelObservers* observers)
    : tab_model_(tab_model), observers_(observers) {}

TabModelFaviconDriverObserver::~TabModelFaviconDriverObserver() {
  if (!driver_to_web_state_map_.empty()) {
    for (const auto& pair : driver_to_web_state_map_) {
      favicon::FaviconDriver* driver = pair.first;
      driver->RemoveObserver(this);
    }
    driver_to_web_state_map_.clear();
  }
}

void TabModelFaviconDriverObserver::WebStateInsertedAt(
    WebStateList* web_state_list,
    web::WebState* web_state,
    int index,
    bool activating) {
  // Forward to WebStateReplacedAt as an insertion can be simulated as
  // replacing a null WebState by a real one.
  WebStateReplacedAt(web_state_list, nullptr, web_state, index);
}

void TabModelFaviconDriverObserver::WebStateReplacedAt(
    WebStateList* web_state_list,
    web::WebState* old_web_state,
    web::WebState* new_web_state,
    int index) {
  if (old_web_state) {
    favicon::WebFaviconDriver* driver =
        favicon::WebFaviconDriver::FromWebState(old_web_state);
    if (driver) {
      auto iterator = driver_to_web_state_map_.find(driver);
      DCHECK(iterator != driver_to_web_state_map_.end());
      DCHECK(iterator->second == old_web_state);
      driver_to_web_state_map_.erase(iterator);
      driver->RemoveObserver(this);
    }
  }

  if (new_web_state) {
    favicon::WebFaviconDriver* driver =
        favicon::WebFaviconDriver::FromWebState(new_web_state);
    if (driver) {
      auto iterator = driver_to_web_state_map_.find(driver);
      DCHECK(iterator == driver_to_web_state_map_.end());
      driver_to_web_state_map_[driver] = new_web_state;
      driver->AddObserver(this);
    }
  }
}

void TabModelFaviconDriverObserver::WebStateDetachedAt(
    WebStateList* web_state_list,
    web::WebState* web_state,
    int index) {
  // Forward to WebStateReplacedAt as a removal can be simulated as
  // replacing a real WebState by a null one.
  WebStateReplacedAt(web_state_list, web_state, nullptr, index);
}

void TabModelFaviconDriverObserver::OnFaviconUpdated(
    favicon::FaviconDriver* driver,
    favicon::FaviconDriverObserver::NotificationIconType icon_type,
    const GURL& icon_url,
    bool icon_url_changed,
    const gfx::Image& image) {
  auto iterator = driver_to_web_state_map_.find(driver);
  DCHECK(iterator != driver_to_web_state_map_.end());
  DCHECK(iterator->second);

  [observers_ tabModel:tab_model_
          didChangeTab:LegacyTabHelper::GetTabForWebState(iterator->second)];
}
