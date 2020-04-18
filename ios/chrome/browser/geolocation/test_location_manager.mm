// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/geolocation/test_location_manager.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TestLocationManager ()

@end

@implementation TestLocationManager
@synthesize authorizationStatus = _authorizationStatus;
@synthesize locationServicesEnabled = _locationServicesEnabled;
@synthesize started = _started;
@synthesize stopped = _stopped;
@synthesize currentLocation = _currentLocation;

- (id)init {
  self = [super init];
  if (self) {
    [self reset];
  }
  return self;
}

- (void)setAuthorizationStatus:(CLAuthorizationStatus)authorizationStatus {
  if (_authorizationStatus != authorizationStatus) {
    _authorizationStatus = authorizationStatus;
    [self.delegate locationManagerDidChangeAuthorizationStatus:self];
  }
}

- (void)reset {
  _authorizationStatus = kCLAuthorizationStatusNotDetermined;
  _currentLocation = nil;
  _locationServicesEnabled = YES;
  _started = NO;
  _stopped = NO;
}

#pragma mark - LocationManager overrides

- (void)startUpdatingLocation {
  _started = YES;
}

- (void)stopUpdatingLocation {
  _stopped = YES;
}

@end
