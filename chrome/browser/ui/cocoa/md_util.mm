// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/md_util.h"

@implementation CAMediaTimingFunction (ChromeBrowserMDUtil)
+ (CAMediaTimingFunction*)cr_materialEaseInTimingFunction {
  return [[[CAMediaTimingFunction alloc] initWithControlPoints:0.4:0.0:1.0:1]
      autorelease];
}

+ (CAMediaTimingFunction*)cr_materialEaseOutTimingFunction {
  return [[[CAMediaTimingFunction alloc] initWithControlPoints:0.0:0.0:0.2:1]
      autorelease];
}

+ (CAMediaTimingFunction*)cr_materialEaseInOutTimingFunction {
  return [[[CAMediaTimingFunction alloc] initWithControlPoints:0.4:0.0:0.2:1]
      autorelease];
}
@end
