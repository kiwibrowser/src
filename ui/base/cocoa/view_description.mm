// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/view_description.h"

#if !NDEBUG

@implementation NSView (CrDebugging)

- (NSString*)cr_recursiveDescriptionWithPrefix:(NSString*)prefix {
  NSString* description =
      [NSString stringWithFormat:@"%@ <%@ %p, frame=%@, hidden=%d>\n",
          prefix, [self class], self, NSStringFromRect([self frame]),
          [self isHidden]];
  prefix = [prefix stringByAppendingString:@"--"];

  for (NSView* subview in [self subviews]) {
    description = [description stringByAppendingString:
        [subview cr_recursiveDescriptionWithPrefix:prefix]];
  }
  return description;
}

- (NSString*)cr_recursiveDescription {
  return [self cr_recursiveDescriptionWithPrefix:@""];
}

@end

#endif  // !NDEBUG
