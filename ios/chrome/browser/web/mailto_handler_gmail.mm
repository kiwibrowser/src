// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/mailto_handler_gmail.h"

#import <UIKit/UIKit.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation MailtoHandlerGmail

- (instancetype)init {
  // Gmail product name is not supposed to be localized.
  self = [super initWithName:@"Gmail" appStoreID:@"422689480"];
  return self;
}

- (NSString*)beginningScheme {
  return @"googlegmail:/co?";
}

@end
