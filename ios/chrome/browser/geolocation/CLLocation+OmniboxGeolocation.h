// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GEOLOCATION_CLLOCATION_OMNIBOXGEOLOCATION_H_
#define IOS_CHROME_BROWSER_GEOLOCATION_CLLOCATION_OMNIBOXGEOLOCATION_H_

#import <CoreLocation/CoreLocation.h>

@interface CLLocation (OmniboxGeolocation)

// The elapsed time in seconds to acquire this location. Defaults to 0 if not
// set.
@property(nonatomic, assign, setter=cr_setAcquisitionInterval:)
    NSTimeInterval cr_acquisitionInterval;

// Returns YES if and only if this location is fresh enough to use for an
// Omnibox query.
- (BOOL)cr_isFreshEnough;

// Returns YES if and only if this location is old enough to try refreshing the
// device location.
- (BOOL)cr_shouldRefresh;

@end

#endif  // IOS_CHROME_BROWSER_GEOLOCATION_CLLOCATION_OMNIBOXGEOLOCATION_H_
