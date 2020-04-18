// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_TAB_MODEL_WEB_STATE_LIST_DELEGATE_H_
#define IOS_CHROME_BROWSER_TABS_TAB_MODEL_WEB_STATE_LIST_DELEGATE_H_

#import <Foundation/Foundation.h>

#include "base/macros.h"
#import "ios/chrome/browser/web_state_list/web_state_list_delegate.h"

@class TabModel;

// WebStateList delegate for the old architecture.
class TabModelWebStateListDelegate : public WebStateListDelegate {
 public:
  explicit TabModelWebStateListDelegate(TabModel* tab_model);
  ~TabModelWebStateListDelegate() override;

  // WebStateListDelegate implementation.
  void WillAddWebState(web::WebState* web_state) override;
  void WebStateDetached(web::WebState* web_state) override;

 private:
  __weak TabModel* tab_model_ = nil;

  DISALLOW_COPY_AND_ASSIGN(TabModelWebStateListDelegate);
};

#endif  // IOS_CHROME_BROWSER_TABS_TAB_MODEL_WEB_STATE_LIST_DELEGATE_H_
