// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GEOLOCATION_TEST_LOCATION_MANAGER_H_
#define IOS_CHROME_BROWSER_GEOLOCATION_TEST_LOCATION_MANAGER_H_

#import "ios/chrome/browser/geolocation/location_manager.h"

// Implements a fake LocationManager for unit tests and KIF tests.
@interface TestLocationManager : LocationManager

// Writable version of the LocationManager |authorizationStatus| property. If
// the new value is different from the previous value, then invokes the
// LocationManagerDelegate |locationManagerDidChangeAuthorizationStatus:|
// method.
@property(nonatomic, assign) CLAuthorizationStatus authorizationStatus;

// Writable version of the LocationManager |currentLocation| property.
@property(nonatomic, strong) CLLocation* currentLocation;

// Writable version of the LocationManager |locationServicesEnabled| property.
@property(nonatomic, assign) BOOL locationServicesEnabled;

// Returns YES if and only if LocationManager |startUpdatingLocation| has been
// invoked at least once since initialization or the last call to |reset|.
@property(nonatomic, readonly) BOOL started;

// Returns YES if and only if LocationManager |stopUpdatingLocation| has been
// invoked at least once since initialization or the last call to |reset|.
@property(nonatomic, readonly) BOOL stopped;

// Resets |authorizationStatus|, |currentLocation|, |locationServicesEnabled|,
// |started|, and |stopped| to the initialized state. Does not reset
// |delegate|.
- (void)reset;

@end

#endif  // IOS_CHROME_BROWSER_GEOLOCATION_TEST_LOCATION_MANAGER_H_
