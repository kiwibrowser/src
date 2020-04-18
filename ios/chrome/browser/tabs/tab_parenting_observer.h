// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_TAB_PARENTING_OBSERVER_H_
#define IOS_CHROME_BROWSER_TABS_TAB_PARENTING_OBSERVER_H_

#include "base/macros.h"
#import "ios/chrome/browser/web_state_list/web_state_list_observer.h"

class TabParentingObserver : public WebStateListObserver {
 public:
  TabParentingObserver();
  ~TabParentingObserver() override;

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
  DISALLOW_COPY_AND_ASSIGN(TabParentingObserver);
};

#endif  // IOS_CHROME_BROWSER_TABS_TAB_PARENTING_OBSERVER_H_
