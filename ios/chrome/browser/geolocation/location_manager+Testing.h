// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GEOLOCATION_LOCATION_MANAGER_TESTING_H_
#define IOS_CHROME_BROWSER_GEOLOCATION_LOCATION_MANAGER_TESTING_H_

#import "ios/chrome/browser/geolocation/location_manager.h"

@protocol GeolocationUpdater;

// Private methods for unit tests.
@interface LocationManager (Testing)

// Sets the GeolocationUpdater for the receiver to use.
- (void)setGeolocationUpdater:(id<GeolocationUpdater>)geolocationUpdater;

@end

#endif  // IOS_CHROME_BROWSER_GEOLOCATION_LOCATION_MANAGER_TESTING_H_
