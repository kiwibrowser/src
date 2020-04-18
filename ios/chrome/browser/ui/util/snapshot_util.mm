// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/util/snapshot_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace snapshot_util {

UIView* GenerateSnapshot(UIView* view) {
  UIView* snapshot;
  if (view.window) {
    // |view| is in a view hierarchy.
    snapshot = [view snapshotViewAfterScreenUpdates:NO];
  } else {
    UIGraphicsBeginImageContextWithOptions(view.bounds.size, NO, 0);
    [view.layer renderInContext:UIGraphicsGetCurrentContext()];
    UIImage* screenshot = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    snapshot = [[UIView alloc] initWithFrame:CGRectZero];
    snapshot.layer.contents = static_cast<id>(screenshot.CGImage);
  }
  return snapshot;
}

}  // namespace snapshot_util
