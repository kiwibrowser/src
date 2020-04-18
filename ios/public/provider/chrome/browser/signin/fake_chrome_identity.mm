// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation FakeChromeIdentity {
  NSString* _userEmail;
  NSString* _gaiaID;
  NSString* _userFullName;
  NSString* _hashedGaiaID;
}

+ (FakeChromeIdentity*)identityWithEmail:(NSString*)email
                                  gaiaID:(NSString*)gaiaID
                                    name:(NSString*)name {
  return
      [[FakeChromeIdentity alloc] initWithEmail:email gaiaID:gaiaID name:name];
}

- (instancetype)initWithEmail:(NSString*)email
                       gaiaID:(NSString*)gaiaID
                         name:(NSString*)name {
  self = [super init];
  if (self) {
    _userEmail = [email copy];
    _gaiaID = [gaiaID copy];
    _userFullName = [name copy];
    _hashedGaiaID = [NSString stringWithFormat:@"%@_hashID", name];
  }
  return self;
}

- (NSString*)userEmail {
  return _userEmail;
}

- (NSString*)gaiaID {
  return _gaiaID;
}

- (NSString*)userFullName {
  return _userFullName;
}

- (NSString*)hashedGaiaID {
  return _hashedGaiaID;
}

@end
