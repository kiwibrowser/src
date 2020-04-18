// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_
#define IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_

#import "ios/web_view/public/cwv_autofill_controller.h"

NS_ASSUME_NONNULL_BEGIN

namespace web {
class WebState;
}  // namespace web

@class AutofillAgent;
@class JsAutofillManager;
@class JsSuggestionManager;

@interface CWVAutofillController ()

- (instancetype)initWithWebState:(web::WebState*)webState
                   autofillAgent:(AutofillAgent*)autofillAgent
               JSAutofillManager:(JsAutofillManager*)JSAutofillManager
             JSSuggestionManager:(JsSuggestionManager*)JSSuggestionManager
    NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_
