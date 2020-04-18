// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_AUTOFILL_FORM_SUGGESTION_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_AUTOFILL_FORM_SUGGESTION_TAB_HELPER_H_

#import <Foundation/Foundation.h>

#include "base/macros.h"
#import "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@protocol FormInputAccessoryViewProvider;
@protocol FormSuggestionProvider;
@class FormSuggestionController;

// Class binding a FormSuggestionController to a WebState.
class FormSuggestionTabHelper
    : public web::WebStateObserver,
      public web::WebStateUserData<FormSuggestionTabHelper> {
 public:
  ~FormSuggestionTabHelper() override;

  // Creates a FormSuggestionTabHelper and attaches it to the given |web_state|.
  static void CreateForWebState(web::WebState* web_state,
                                NSArray<id<FormSuggestionProvider>>* providers);

  // Returns an object that can provide an input accessory view from the
  // FormSuggestionController.
  id<FormInputAccessoryViewProvider> GetAccessoryViewProvider();

 private:
  FormSuggestionTabHelper(web::WebState* web_state,
                          NSArray<id<FormSuggestionProvider>>* providers);

  // web::WebStateObserver implementation.
  void WebStateDestroyed(web::WebState* web_state) override;

  // The Objective-C password controller instance.
  __strong FormSuggestionController* controller_;

  DISALLOW_COPY_AND_ASSIGN(FormSuggestionTabHelper);
};

#endif  // IOS_CHROME_BROWSER_AUTOFILL_FORM_SUGGESTION_TAB_HELPER_H_
