// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_TAB_MODEL_WEB_USAGE_ENABLED_OBSERVER_H_
#define IOS_CHROME_BROWSER_TABS_TAB_MODEL_WEB_USAGE_ENABLED_OBSERVER_H_

#include "base/macros.h"
#import "ios/chrome/browser/web_state_list/web_state_list_observer.h"

@class TabModel;

class TabModelWebUsageEnabledObserver : public WebStateListObserver {
 public:
  explicit TabModelWebUsageEnabledObserver(TabModel* tab_model);
  ~TabModelWebUsageEnabledObserver() override;

  // WebStateListObserver implementation.
  void WebStateInsertedAt(WebStateList* web_state_list,
                          web::WebState* web_state,
                          int index,
                          bool activating) override;
  void WebStateReplacedAt(WebStateList* web_state_list,
                          web::WebState* old_web_state,
                          web::WebState* new_web_state,
                          int index) override;

 private:
  __weak TabModel* tab_model_;

  DISALLOW_COPY_AND_ASSIGN(TabModelWebUsageEnabledObserver);
};

#endif  // IOS_CHROME_BROWSER_TABS_TAB_MODEL_WEB_USAGE_ENABLED_OBSERVER_H_
