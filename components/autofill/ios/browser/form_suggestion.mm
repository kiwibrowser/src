// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/autofill/ios/browser/form_suggestion.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface FormSuggestion ()
// Local initializer for a FormSuggestion.
- (instancetype)initWithValue:(NSString*)value
           displayDescription:(NSString*)displayDescription
                         icon:(NSString*)icon
                   identifier:(NSInteger)identifier;
@end

@implementation FormSuggestion

@synthesize value = _value;
@synthesize displayDescription = _displayDescription;
@synthesize icon = _icon;
@synthesize identifier = _identifier;

- (instancetype)initWithValue:(NSString*)value
           displayDescription:(NSString*)displayDescription
                         icon:(NSString*)icon
                   identifier:(NSInteger)identifier {
  self = [super init];
  if (self) {
    _value = [value copy];
    _displayDescription = [displayDescription copy];
    _icon = [icon copy];
    _identifier = identifier;
  }
  return self;
}

+ (FormSuggestion*)suggestionWithValue:(NSString*)value
                    displayDescription:(NSString*)displayDescription
                                  icon:(NSString*)icon
                            identifier:(NSInteger)identifier {
  return [[FormSuggestion alloc] initWithValue:value
                            displayDescription:displayDescription
                                          icon:icon
                                    identifier:identifier];
}

@end
