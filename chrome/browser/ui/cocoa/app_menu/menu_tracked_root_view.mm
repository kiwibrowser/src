// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/app_menu/menu_tracked_root_view.h"

@implementation MenuTrackedRootView

@synthesize menuItem = menuItem_;

- (void)mouseUp:(NSEvent*)theEvent {
  [[menuItem_ menu] cancelTracking];
}

@end
