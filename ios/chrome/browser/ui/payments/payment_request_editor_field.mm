// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/payment_request_editor_field.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation EditorField

@synthesize autofillUIType = _autofillUIType;
@synthesize fieldType = _fieldType;
@synthesize label = _label;
@synthesize value = _value;
@synthesize displayValue = _displayValue;
@synthesize required = _required;
@synthesize enabled = _enabled;
@synthesize returnKeyType = _returnKeyType;
@synthesize keyboardType = _keyboardType;
@synthesize autoCapitalizationType = _autoCapitalizationType;
@synthesize item = _item;
@synthesize sectionIdentifier = _sectionIdentifier;
@synthesize pristine = _pristine;

- (instancetype)initWithAutofillUIType:(AutofillUIType)autofillUIType
                             fieldType:(EditorFieldType)fieldType
                                 label:(NSString*)label
                                 value:(NSString*)value
                              required:(BOOL)required {
  self = [super init];
  if (self) {
    _autofillUIType = autofillUIType;
    _fieldType = fieldType;
    _label = label;
    _value = value;
    _required = required;
    _enabled = YES;
    _returnKeyType = UIReturnKeyNext;
    _keyboardType = UIKeyboardTypeDefault;
    _autoCapitalizationType = UITextAutocapitalizationTypeWords;
    _pristine = YES;
  }
  return self;
}

- (NSString*)description {
  return [NSString stringWithFormat:@"Label: %@, Value: %@", _label, _value];
}

@end
