// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/cwv_autofill_suggestion_internal.h"

#include "components/autofill/core/browser/popup_item_ids.h"
#import "components/autofill/ios/browser/form_suggestion.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation CWVAutofillSuggestion

@synthesize formSuggestion = _formSuggestion;
@synthesize formName = _formName;
@synthesize fieldName = _fieldName;
@synthesize fieldIdentifier = _fieldIdentifier;

- (instancetype)initWithFormSuggestion:(FormSuggestion*)formSuggestion
                              formName:(NSString*)formName
                             fieldName:(NSString*)fieldName
                       fieldIdentifier:(NSString*)fieldIdentifier {
  self = [super init];
  if (self) {
    _formSuggestion = formSuggestion;
    _formName = [formName copy];
    _fieldName = [fieldName copy];
    _fieldIdentifier = [fieldIdentifier copy];
  }
  return self;
}

#pragma mark - Public Methods

- (NSString*)value {
  return [_formSuggestion.value copy];
}

- (NSString*)displayDescription {
  return [_formSuggestion.displayDescription copy];
}

#pragma mark - NSObject

- (NSString*)debugDescription {
  return [NSString stringWithFormat:@"%@ value: %@, displayDescription: %@",
                                    super.debugDescription, self.value,
                                    self.displayDescription];
}

@end
