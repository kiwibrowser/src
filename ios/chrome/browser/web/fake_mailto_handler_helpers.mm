// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/fake_mailto_handler_helpers.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation FakeMailtoHandlerGmailNotInstalled
- (BOOL)isAvailable {
  return NO;
}
@end

@implementation FakeMailtoHandlerGmailInstalled
- (BOOL)isAvailable {
  return YES;
}
@end

@implementation FakeMailtoHandlerForTesting
- (instancetype)init {
  return [super initWithName:@"FakeMail" appStoreID:@"12345678"];
}
- (BOOL)isAvailable {
  return NO;
}
- (NSString*)beginningScheme {
  return @"fakemail:/compose?";
}
@end

@implementation CountingMailtoHandlerManagerObserver
@synthesize changeCount = _changeCount;
- (void)handlerDidChangeForMailtoHandlerManager:(MailtoHandlerManager*)manager {
  ++_changeCount;
}
@end
