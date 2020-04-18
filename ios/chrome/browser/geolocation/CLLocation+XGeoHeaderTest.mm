// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <CoreLocation/CoreLocation.h>

#import "ios/chrome/browser/geolocation/CLLocation+XGeoHeader.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

NSString* const kEncoded550BatterySt =
    @"a cm9sZTogQ1VSUkVOVF9MT0NBVElPTgpwcm9kdWNlcjogREVWSUNFX0xPQ0FUSU9OCnRpbWV"
     "zdGFtcDogMTM4OTAwMDAwMDAwMDAwMApyYWRpdXM6IDEwMDAwCmxhdGxuZyA8CiAgbGF0aXR1"
     "ZGVfZTc6IDM3Nzk2MzIyMAogIGxvbmdpdHVkZV9lNzogLTEyMjQwMDI5MTAKPg==";

using CLLocationXGeoHeaderTest = PlatformTest;

TEST_F(CLLocationXGeoHeaderTest, TestXGeoString) {
  CLLocationCoordinate2D coordinate =
      CLLocationCoordinate2DMake(37.796322, -122.400291);
  // Picked a fixed timestamp. This one is 2014-01-06 09:20:00 +0000.
  NSDate* timestamp = [NSDate dateWithTimeIntervalSince1970:1389000000];
  CLLocation* location = [[CLLocation alloc] initWithCoordinate:coordinate
                                                       altitude:0
                                             horizontalAccuracy:10
                                               verticalAccuracy:100
                                                         course:0
                                                          speed:0
                                                      timestamp:timestamp];
  NSString* xGeoString = [location cr_xGeoString];
  EXPECT_TRUE([xGeoString isEqualToString:kEncoded550BatterySt]);
}

}  // namespace
