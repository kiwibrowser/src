// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/grid/grid_item.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "base/logging.h"

@implementation GridItem
@synthesize identifier = _identifier;
@synthesize title = _title;

- (instancetype)initWithIdentifier:(NSString*)identifier {
  DCHECK(identifier);
  if ((self = [super init])) {
    _identifier = identifier;
  }
  return self;
}

@end
