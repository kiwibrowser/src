// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_AUTOFILL_AUTOFILL_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_AUTOFILL_AUTOFILL_TAB_HELPER_H_

#include "base/macros.h"
#import "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@class AutofillController;
@protocol FormSuggestionProvider;
@class UIViewController;

namespace password_manager {
class PasswordGenerationManager;
}

// Class binding an AutofillController to a WebState.
class AutofillTabHelper : public web::WebStateObserver,
                          public web::WebStateUserData<AutofillTabHelper> {
 public:
  ~AutofillTabHelper() override;

  // Create an AutofillTabHelper and attaches it to the given |web_state|.
  static void CreateForWebState(
      web::WebState* web_state,
      password_manager::PasswordGenerationManager* password_generation_manager);

  // Sets a weak reference to the view controller used to present UI.
  void SetBaseViewController(UIViewController* base_view_controller);

  // Returns an object that can provide suggestions from the PasswordController.
  // May return nil.
  id<FormSuggestionProvider> GetSuggestionProvider();

 private:
  AutofillTabHelper(
      web::WebState* web_state,
      password_manager::PasswordGenerationManager* password_generation_manager);

  // web::WebStateObserver implementation.
  void WebStateDestroyed(web::WebState* web_state) override;

  // The Objective-C autofill controller instance.
  __strong AutofillController* controller_;

  DISALLOW_COPY_AND_ASSIGN(AutofillTabHelper);
};

#endif  // IOS_CHROME_BROWSER_AUTOFILL_AUTOFILL_TAB_HELPER_H_
