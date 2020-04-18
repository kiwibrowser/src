// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GEOLOCATION_CLLOCATION_XGEOHEADER_H_
#define IOS_CHROME_BROWSER_GEOLOCATION_CLLOCATION_XGEOHEADER_H_

#import <CoreLocation/CoreLocation.h>

@interface CLLocation (XGeoHeader)

// Returns a string that encodes the location for use with the "X-Geo" HTTP
// header field. The string is a base-64-encoded ASCII proto.
- (NSString*)cr_xGeoString;

@end

#endif  // IOS_CHROME_BROWSER_GEOLOCATION_CLLOCATION_XGEOHEADER_H_
