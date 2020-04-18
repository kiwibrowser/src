// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/geolocation/location_manager.h"

#import "ios/chrome/browser/geolocation/CLLocation+OmniboxGeolocation.h"
#import "ios/chrome/browser/geolocation/location_manager+Testing.h"
#import "ios/public/provider/chrome/browser/geolocation_updater_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class LocationManagerTest : public PlatformTest {
 public:
  LocationManagerTest() {}

 protected:
  void SetUp() override {
    PlatformTest::SetUp();

    mock_geolocation_updater_ =
        [OCMockObject mockForProtocol:@protocol(GeolocationUpdater)];

    // Set up LocationManager with a mock GeolocationUpdater.
    location_manager_ = [[LocationManager alloc] init];
    [location_manager_ setGeolocationUpdater:mock_geolocation_updater_];
  }

  void TearDown() override {
    [location_manager_ setGeolocationUpdater:nil];

    PlatformTest::TearDown();
  }

  id mock_geolocation_updater_;
  LocationManager* location_manager_;
};

// Verifies that -[LocationManager startUpdatingLocation] calls
// -[GeolocationUpdater setEnabled:] when GeolocationUpdater does not yet have
// a current location.
TEST_F(LocationManagerTest, StartUpdatingLocationNilCurrentLocation) {
  [[[mock_geolocation_updater_ expect] andReturn:nil] currentLocation];

  // Also expect the call to -[GeolocationUpdater isEnabled];
  BOOL no = NO;
  [[[mock_geolocation_updater_ expect]
      andReturnValue:OCMOCK_VALUE(no)] isEnabled];
  [[mock_geolocation_updater_ expect] requestWhenInUseAuthorization];
  [[mock_geolocation_updater_ expect] setEnabled:YES];

  [location_manager_ startUpdatingLocation];
  EXPECT_OCMOCK_VERIFY(mock_geolocation_updater_);
}

// Verifies that -[LocationManager startUpdatingLocation] calls
// -[GeolocationUpdater setEnabled:] when GeolocationUpdater
// |currentLocation| is stale.
TEST_F(LocationManagerTest, StartUpdatingLocationStaleCurrentLocation) {
  // Set up to return a stale mock CLLocation from -[GeolocationUpdater
  // currentLocation].
  id mock_location = [OCMockObject mockForClass:[CLLocation class]];
  BOOL yes = YES;
  [[[mock_location expect] andReturnValue:OCMOCK_VALUE(yes)] cr_shouldRefresh];

  [[[mock_geolocation_updater_ expect] andReturn:mock_location]
      currentLocation];

  // Also expect the call to -[GeolocationUpdater isEnabled];
  BOOL no = NO;
  [[[mock_geolocation_updater_ expect]
      andReturnValue:OCMOCK_VALUE(no)] isEnabled];
  [[mock_geolocation_updater_ expect] requestWhenInUseAuthorization];
  [[mock_geolocation_updater_ expect] setEnabled:YES];

  [location_manager_ startUpdatingLocation];
  EXPECT_OCMOCK_VERIFY(mock_geolocation_updater_);
  EXPECT_OCMOCK_VERIFY(mock_location);
}

// Verifies that -[LocationManager startUpdatingLocation] does not call
// -[GeolocationUpdater setEnabled:] when GeolocationUpdater's
// |currentLocation| is fresh.
TEST_F(LocationManagerTest, StartUpdatingLocationFreshCurrentLocation) {
  // Set up to return a fresh mock CLLocation from -[GeolocationUpdater
  // currentLocation].
  id mock_location = [OCMockObject mockForClass:[CLLocation class]];
  BOOL no = NO;
  [[[mock_location expect] andReturnValue:OCMOCK_VALUE(no)] cr_shouldRefresh];

  [[[mock_geolocation_updater_ expect] andReturn:mock_location]
      currentLocation];

  [location_manager_ startUpdatingLocation];
  EXPECT_OCMOCK_VERIFY(mock_geolocation_updater_);
  EXPECT_OCMOCK_VERIFY(mock_location);
}

}  // namespace
