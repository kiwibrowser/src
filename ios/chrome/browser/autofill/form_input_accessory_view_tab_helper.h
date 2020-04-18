// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_AUTOFILL_FORM_INPUT_ACCESSORY_VIEW_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_AUTOFILL_FORM_INPUT_ACCESSORY_VIEW_TAB_HELPER_H_

#import <Foundation/Foundation.h>

#include "base/macros.h"
#import "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@protocol FormInputAccessoryViewProvider;
@class FormInputAccessoryViewController;

// Class binding a FormInputAccessoryViewController to a WebState.
class FormInputAccessoryViewTabHelper
    : public web::WebStateObserver,
      public web::WebStateUserData<FormInputAccessoryViewTabHelper> {
 public:
  ~FormInputAccessoryViewTabHelper() override;

  // Creates a FormInputAccessoryViewTabHelper and attaches it to the given
  // |web_state|.
  static void CreateForWebState(
      web::WebState* web_state,
      NSArray<id<FormInputAccessoryViewProvider>>* providers);

  // Closes the input keyboard. Can be called even if the input keyboard
  // is not currently showing.
  void CloseKeyboard();

 private:
  FormInputAccessoryViewTabHelper(
      web::WebState* web_state,
      NSArray<id<FormInputAccessoryViewProvider>>* providers);

  // web::WebStateObserver implementation.
  void WasShown(web::WebState* web_state) override;
  void WasHidden(web::WebState* web_state) override;
  void WebStateDestroyed(web::WebState* web_state) override;

  // The Objective-C form input accessory view controller instance.
  __strong FormInputAccessoryViewController* controller_;

  DISALLOW_COPY_AND_ASSIGN(FormInputAccessoryViewTabHelper);
};

#endif  // IOS_CHROME_BROWSER_AUTOFILL_FORM_INPUT_ACCESSORY_VIEW_TAB_HELPER_H_
