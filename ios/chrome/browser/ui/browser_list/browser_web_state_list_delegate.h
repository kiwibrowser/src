// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_WEB_STATE_LIST_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_WEB_STATE_LIST_DELEGATE_H_

#include "base/macros.h"
#import "ios/chrome/browser/web_state_list/web_state_list_delegate.h"

// BrowserWebStateListDelegate attaches tab helper to the WebStates when
// they are added to the WebStateList.
//
// If a tab helper needs to be present for all WebStates in clean skeleton,
// then it must be attached to the WebState in this class' WillAddWebState.
class BrowserWebStateListDelegate : public WebStateListDelegate {
 public:
  BrowserWebStateListDelegate();
  ~BrowserWebStateListDelegate() override;

  // WebStateListDelegate implementation.
  void WillAddWebState(web::WebState* web_state) override;
  void WebStateDetached(web::WebState* web_state) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserWebStateListDelegate);
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_WEB_STATE_LIST_DELEGATE_H_
