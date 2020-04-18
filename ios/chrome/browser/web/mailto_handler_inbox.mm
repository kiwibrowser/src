// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/mailto_handler_inbox.h"

#import <UIKit/UIKit.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation MailtoHandlerInbox

- (instancetype)init {
  // App Store product name is "Inbox by Gmail" and it is not localized.
  // https://itunes.apple.com/us/app/inbox-by-gmail/id905060486?mt=8
  self = [super initWithName:@"Inbox by Gmail" appStoreID:@"905060486"];
  return self;
}

- (NSString*)beginningScheme {
  return @"inbox-gmail://co?";
}

@end
