// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef IOS_CHROME_BROWSER_AUTOFILL_AUTOFILL_CONTROLLER_H_
#define IOS_CHROME_BROWSER_AUTOFILL_AUTOFILL_CONTROLLER_H_

#import <UIKit/UIKit.h>
#include <vector>

#include "components/autofill/core/common/form_data_predictions.h"

@class AutofillAgent;
@protocol FormSuggestionProvider;

namespace ios {
class ChromeBrowserState;
}

namespace password_manager {
class PasswordGenerationManager;
}

namespace web {
class WebState;
}

// Handles Autofill.
@interface AutofillController : NSObject

@property(nonatomic, readonly) ios::ChromeBrowserState* browserState;
@property(weak, nonatomic, readonly) id<FormSuggestionProvider>
    suggestionProvider;

// Designated initializer. Neither |browserState| nor |webState| should be null.
// |downloadEnabled| should be NO for tests to stop the system making external
// network requests.
- (instancetype)
     initWithBrowserState:(ios::ChromeBrowserState*)browserState
                 webState:(web::WebState*)webState
            autofillAgent:(AutofillAgent*)autofillAgent
passwordGenerationManager:
    (password_manager::PasswordGenerationManager*)passwordGenerationManager
          downloadEnabled:(BOOL)downloadEnabled NS_DESIGNATED_INITIALIZER;

// Convenience initializer. The autofill agent will be created from the
// given webstate.
// The system will start making external network requests.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
           passwordGenerationManager:
               (password_manager::PasswordGenerationManager*)
                   passwordGenerationManager
                            webState:(web::WebState*)webState;

- (instancetype)init NS_UNAVAILABLE;

// Detaches itself from the supplied |webState|.
- (void)detachFromWebState;

// Sends the field type predictions specified in |forms| to the renderer. This
// method is a no-op if the appropriate experiment is not set.
- (void)sendAutofillTypePredictionsToRenderer:
    (const std::vector<autofill::FormDataPredictions>&)forms;

// Sets a weak reference to the view controller used to present UI.
- (void)setBaseViewController:(UIViewController*)baseViewController;

@end

#endif  // IOS_CHROME_BROWSER_AUTOFILL_AUTOFILL_CONTROLLER_H_
