// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tab_contents/overlayable_contents_controller.h"

@implementation OverlayableContentsController

- (id)init {
  if ((self = [super init])) {
    base::scoped_nsobject<NSView> view(
        [[NSView alloc] initWithFrame:NSZeroRect]);
    [view setAutoresizingMask:NSViewHeightSizable | NSViewWidthSizable];
    [self setView:view];

    activeContainer_.reset([[NSView alloc] initWithFrame:NSZeroRect]);
    [activeContainer_ setAutoresizingMask:NSViewHeightSizable |
                                          NSViewWidthSizable];
    [view addSubview:activeContainer_];
  }
  return self;
}

- (NSView*)activeContainer {
  return activeContainer_.get();
}

@end
