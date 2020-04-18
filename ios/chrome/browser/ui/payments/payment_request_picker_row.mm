// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/payment_request_picker_row.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation PickerRow

@synthesize label = _label;
@synthesize value = _value;

- (instancetype)initWithLabel:(NSString*)label value:(NSString*)value {
  self = [super init];
  if (self) {
    _label = label;
    _value = value;
  }
  return self;
}

- (NSString*)description {
  return [NSString stringWithFormat:@"Label: %@, Value: %@", _label, _value];
}

@end
