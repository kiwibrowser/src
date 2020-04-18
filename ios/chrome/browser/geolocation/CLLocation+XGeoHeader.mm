// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/geolocation/CLLocation+XGeoHeader.h"

#include <stdint.h>

#import "third_party/google_toolbox_for_mac/src/Foundation/GTMStringEncoding.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kGMOLocationDescriptorFormat =
    @"role: CURRENT_LOCATION\n"
    @"producer: DEVICE_LOCATION\n"
    @"timestamp: %lld\n"
    @"radius: %ld\n"
    @"latlng <\n"
    @"  latitude_e7: %.f\n"
    @"  longitude_e7: %.f\n"
    @">";

@implementation CLLocation (XGeoHeader)

- (NSString*)cr_serializeStringToWebSafeBase64String:(NSString*)data {
  GTMStringEncoding* encoder =
      [GTMStringEncoding rfc4648Base64WebsafeStringEncoding];
  NSString* base64 =
      [encoder encode:[data dataUsingEncoding:NSUTF8StringEncoding]
                error:nullptr];
  if (base64) {
    return base64;
  }
  return @"";
}

// Returns the timestamp of this location in microseconds since the UNIX epoch.
// Returns 0 if the timestamp is unavailable or invalid.
- (int64_t)cr_timestampInMicroseconds {
  NSTimeInterval seconds = [self.timestamp timeIntervalSince1970];
  if (seconds > 0) {
    const int64_t kSecondsToMicroseconds = 1000000;
    return (int64_t)(seconds * kSecondsToMicroseconds);
  }
  return 0;
}

// Returns the horizontal accuracy radius of |location|. The smaller the value,
// the more accurate the location. A value -1 is returned if accuracy is
// unavailable.
- (long)cr_accuracyInMillimeters {
  const long kMetersToMillimeters = 1000;
  if (self.horizontalAccuracy > 0) {
    return (long)(self.horizontalAccuracy * kMetersToMillimeters);
  }
  return -1L;
}

// Returns the LocationDescriptor as an ASCII proto.
- (NSString*)cr_locationDescriptor {
  // Construct the location descriptor using its format string.
  return [NSString stringWithFormat:kGMOLocationDescriptorFormat,
                                    [self cr_timestampInMicroseconds],
                                    [self cr_accuracyInMillimeters],
                                    floor(self.coordinate.latitude * 1e7),
                                    floor(self.coordinate.longitude * 1e7)];
}

- (NSString*)cr_xGeoString {
  NSString* locationDescriptor = [self cr_locationDescriptor];
  // The "a" indicates that it is an ASCII proto.
  return [NSString
      stringWithFormat:@"a %@", [self cr_serializeStringToWebSafeBase64String:
                                          locationDescriptor]];
}

@end
