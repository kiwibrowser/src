// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/public/provider/chrome/browser/geolocation_updater_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios {

GeolocationUpdaterProvider::GeolocationUpdaterProvider() {
}

GeolocationUpdaterProvider::~GeolocationUpdaterProvider() {
}

id<GeolocationUpdater> GeolocationUpdaterProvider::CreateGeolocationUpdater(
    bool enabled) {
  return nil;
}

NSString* GeolocationUpdaterProvider::GetUpdateNotificationName() {
  return nil;
}

NSString* GeolocationUpdaterProvider::GetStopNotificationName() {
  return nil;
}

NSString* GeolocationUpdaterProvider::GetAuthorizationChangeNotificationName() {
  return nil;
}

NSString* GeolocationUpdaterProvider::GetUpdateNewLocationKey() {
  return nil;
}

}  // namespace ios
