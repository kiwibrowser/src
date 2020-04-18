// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/mailto_handler_manager.h"

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/web/mailto_handler.h"
#import "ios/chrome/browser/web/mailto_handler_gmail.h"
#import "ios/chrome/browser/web/mailto_handler_inbox.h"
#import "ios/chrome/browser/web/mailto_handler_system_mail.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kMailtoHandlerManagerUserDefaultsKey =
    @"UserChosenDefaultMailApp";

@interface MailtoHandlerManager ()

// Dictionary keyed by the unique ID of the Mail client. The value is
// the MailtoHandler object that can rewrite a mailto: URL.
@property(nonatomic, strong)
    NSMutableDictionary<NSString*, MailtoHandler*>* handlers;

@end

@implementation MailtoHandlerManager
@synthesize observer = _observer;
@synthesize handlers = _handlers;

- (instancetype)init {
  self = [super init];
  if (self) {
    _handlers = [NSMutableDictionary dictionary];
  }
  return self;
}

+ (NSString*)systemMailApp {
  // This is the App Store ID for Apple Mail app.
  // See https://itunes.apple.com/us/app/mail/id1108187098?mt=8
  return @"1108187098";
}

+ (instancetype)mailtoHandlerManagerWithStandardHandlers {
  id result = [[MailtoHandlerManager alloc] init];
  [result setDefaultHandlers:@[
    [[MailtoHandlerSystemMail alloc] init], [[MailtoHandlerGmail alloc] init],
    [[MailtoHandlerInbox alloc] init]
  ]];
  return result;
}

- (NSArray<MailtoHandler*>*)defaultHandlers {
  return [[_handlers allValues]
      sortedArrayUsingComparator:^NSComparisonResult(
          MailtoHandler* _Nonnull obj1, MailtoHandler* _Nonnull obj2) {
        return [[obj1 appName] compare:[obj2 appName]];
      }];
}

- (void)setDefaultHandlers:(NSArray<MailtoHandler*>*)defaultHandlers {
  for (MailtoHandler* app in defaultHandlers) {
    [_handlers setObject:app forKey:[app appStoreID]];
  }
}

- (NSString*)defaultHandlerID {
  NSString* value = [[NSUserDefaults standardUserDefaults]
      stringForKey:kMailtoHandlerManagerUserDefaultsKey];
  if (value) {
    if ([_handlers[value] isAvailable])
      return value;
    return [[self class] systemMailApp];
  }
  // User has not made a choice.
  NSMutableArray* availableHandlers = [NSMutableArray array];
  for (MailtoHandler* handler in [_handlers allValues]) {
    if ([handler isAvailable])
      [availableHandlers addObject:handler];
  }
  if ([availableHandlers count] == 1)
    return [[availableHandlers firstObject] appStoreID];
  return nil;
}

- (void)setDefaultHandlerID:(NSString*)appStoreID {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  if (appStoreID) {
    NSString* handlerID =
        [defaults objectForKey:kMailtoHandlerManagerUserDefaultsKey];
    if ([appStoreID isEqual:handlerID])
      return;
    [defaults setObject:appStoreID forKey:kMailtoHandlerManagerUserDefaultsKey];
  } else {
    [defaults removeObjectForKey:kMailtoHandlerManagerUserDefaultsKey];
  }
  [self.observer handlerDidChangeForMailtoHandlerManager:self];
}

- (NSString*)defaultHandlerName {
  NSString* handlerID = [self defaultHandlerID];
  if (!handlerID)
    return nil;
  MailtoHandler* handler = _handlers[handlerID];
  return [handler appName];
}

- (MailtoHandler*)defaultHandlerByID:(NSString*)handlerID {
  return _handlers[handlerID];
}

- (NSString*)rewriteMailtoURL:(const GURL&)gURL {
  NSString* value = [self defaultHandlerID];
  if ([value length]) {
    MailtoHandler* handler = _handlers[value];
    if ([handler isAvailable]) {
      return [handler rewriteMailtoURL:gURL];
    }
  }
  return nil;
}

@end
