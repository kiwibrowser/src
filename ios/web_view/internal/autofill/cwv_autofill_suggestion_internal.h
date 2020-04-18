// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_SUGGESTION_INTERNAL_H_
#define IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_SUGGESTION_INTERNAL_H_

#import "ios/web_view/public/cwv_autofill_suggestion.h"

NS_ASSUME_NONNULL_BEGIN

@class FormSuggestion;

@interface CWVAutofillSuggestion ()

- (instancetype)initWithFormSuggestion:(FormSuggestion*)formSuggestion
                              formName:(NSString*)formName
                             fieldName:(NSString*)fieldName
                       fieldIdentifier:(NSString*)fieldIdentifier
    NS_DESIGNATED_INITIALIZER;

// The internal autofill form suggestion.
@property(nonatomic, readonly) FormSuggestion* formSuggestion;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_SUGGESTION_INTERNAL_H_
