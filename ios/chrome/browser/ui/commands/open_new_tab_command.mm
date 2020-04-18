// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/commands/open_new_tab_command.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation OpenNewTabCommand

@synthesize incognito = _incognito;
@synthesize originPoint = _originPoint;
@synthesize userInitiated = _userInitiated;
@synthesize shouldFocusOmnibox = _shouldFocusOmnibox;

- (instancetype)initWithIncognito:(BOOL)incognito
                      originPoint:(CGPoint)originPoint {
  if ((self = [super init])) {
    _incognito = incognito;
    _originPoint = originPoint;
    _userInitiated = YES;
  }
  return self;
}

+ (instancetype)commandWithIncognito:(BOOL)incognito {
  return [[self alloc] initWithIncognito:incognito originPoint:CGPointZero];
}

+ (instancetype)command {
  return [self commandWithIncognito:NO];
}

+ (instancetype)incognitoTabCommand {
  return [self commandWithIncognito:YES];
}

@end
