// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/snapshots/snapshot_overlay.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SnapshotOverlay

@synthesize view = _view;
@synthesize yOffset = _yOffset;

- (instancetype)initWithView:(UIView*)view yOffset:(CGFloat)yOffset {
  self = [super init];
  if (self) {
    DCHECK(view);
    _view = view;
    _yOffset = yOffset;
  }
  return self;
}

@end
