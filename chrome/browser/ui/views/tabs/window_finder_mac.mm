// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/window_finder.h"

#import <AppKit/AppKit.h>

#include "ui/gfx/geometry/point.h"
#import "ui/gfx/mac/coordinate_conversion.h"

gfx::NativeWindow WindowFinder::GetLocalProcessWindowAtPoint(
    const gfx::Point& screen_point,
    const std::set<gfx::NativeWindow>& ignore) {
  const NSPoint ns_point = gfx::ScreenPointToNSPoint(screen_point);

  // Note: [NSApp orderedWindows] doesn't include NSPanels.
  for (NSWindow* window : [NSApp orderedWindows]) {
    if (ignore.count(window))
      continue;

    if (![window isOnActiveSpace])
      continue;

    // NativeWidgetMac::Close() calls -orderOut: on NSWindows before actually
    // closing them.
    if (![window isVisible])
      continue;

    if (NSPointInRect(ns_point, [window frame]))
      return window;
  }

  return nil;
}
