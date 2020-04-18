// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/cells/accessibility_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation AccessibilityLabelBuilder {
  NSMutableArray* components_;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    components_ = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void)appendItem:(NSString*)item {
  if (item)
    [components_ addObject:item];
}

- (NSString*)buildAccessibilityLabel {
  return [components_ componentsJoinedByString:@", "];
}

@end
