// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_GEOLOCATION_UPDATER_PROVIDER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_GEOLOCATION_UPDATER_PROVIDER_H_

#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>

#include "base/macros.h"

// GeolocationUpdater tracks the current location of the user and sends
// NSNotifications when it changes.
// The notification names and keys can be obtained using
// GeolocationUpdaterProvider.
@protocol GeolocationUpdater<NSObject>

// Returns the most recently fetched location.  Returns nil if disabled.
- (CLLocation*)currentLocation;

// Prompts the user to authorize access to location information while the app is
// in use. Only affects iOS 8+.
- (void)requestWhenInUseAuthorization;

// Set the |timeout| the before the manager gives up fetching the location
// information. If a timer has already been set to stop fetching location,
// it will be reset to |timeout| seconds from now.
- (void)setStopUpdateDelay:(NSTimeInterval)timeout;

// Sets how often should the location manager check for a fresh location.
- (void)setUpdateInterval:(NSTimeInterval)interval;

// Sets the desired accuracy, distance and update interval.
// Desired Accuracy defaults to: kCLLocationAccuracyBest.
// Distance Filter defaults to: 20m.
- (void)setDesiredAccuracy:(CLLocationAccuracy)desiredAccuracy
            distanceFilter:(CLLocationDistance)distanceFilter;

// Does not take into consideration whether location is disabled by the OS.
- (BOOL)isEnabled;

// Turns the geolocation updater on or off, depending on the value of flag.
// if flag is false, then no more location updates will be requested, and
// currentLocation will return nil.
- (void)setEnabled:(BOOL)flag;

@end

namespace ios {

class GeolocationUpdaterProvider {
 public:
  GeolocationUpdaterProvider();
  virtual ~GeolocationUpdaterProvider();

  // Creates a new GeolocationUpdater.
  // The returned object is retained and it is the responsibility of the caller
  // to release it.
  virtual id<GeolocationUpdater> CreateGeolocationUpdater(bool enabled)
      NS_RETURNS_RETAINED;

  // Notification names:

  // Name of NSNotificationCenter notification posted on user location update.
  // Passes a |CLLocation| for the new location in the userInfo dictionary with
  // the key returned by GetUpdateNewLocationKey().
  virtual NSString* GetUpdateNotificationName();
  // Name of NSNotificationCenter notification posted on when the location
  // manager's stops.
  virtual NSString* GetStopNotificationName();
  // Name of NSNotificationCenter notification posted when the location
  // manager's authorization status changes. For example when the user turns
  // off Locations Services in Settings. The new status is passed as a
  // |NSNumber| representing the |CLAuthorizationStatus| enum value.
  virtual NSString* GetAuthorizationChangeNotificationName();

  // Notification keys:

  // Key used in the userInfo dictionaries of this class' notifications.
  // Contains a |CLLocation *| and is used in the update notification.
  virtual NSString* GetUpdateNewLocationKey();

 private:
  DISALLOW_COPY_AND_ASSIGN(GeolocationUpdaterProvider);
};

}  // namespace ios

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_GEOLOCATION_UPDATER_PROVIDER_H_
