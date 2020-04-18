// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/public/cwv_identity.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation CWVIdentity

@synthesize email = _email;
@synthesize fullName = _fullName;
@synthesize gaiaID = _gaiaID;

- (instancetype)initWithEmail:(NSString*)email
                     fullName:(nullable NSString*)fullName
                       gaiaID:(NSString*)gaiaID {
  self = [super init];
  if (self) {
    _email = [email copy];
    _fullName = [fullName copy];
    _gaiaID = [gaiaID copy];
  }
  return self;
}

@end
