// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_info.h"

#include "base/mac/foundation_util.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet.h"
#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"

@implementation ConstrainedWindowSheetInfo

@synthesize sheetDidShow = sheetDidShow_;

- (id)initWithSheet:(id<ConstrainedWindowSheet>)sheet
         parentView:(NSView*)parentView
      overlayWindow:(NSWindow*)overlayWindow {
  if ((self = [super init])) {
    sheet_.reset([sheet retain]);
    parentView_.reset([parentView retain]);
    overlayWindow_.reset([overlayWindow retain]);
  }
  return self;
}

- (id<ConstrainedWindowSheet>)sheet {
  return sheet_;
}

- (NSView*)parentView {
  return parentView_;
}

- (NSWindow*)overlayWindow {
  return overlayWindow_;
}

- (void)hideSheet {
  [sheet_ hideSheet];

  // Overlay window is already invisible so just stop accepting mouse events.
  [overlayWindow_ setIgnoresMouseEvents:YES];

  // Make sure the now invisible sheet doesn't keep keyboard focus
  [[overlayWindow_ parentWindow] makeKeyWindow];
}

- (void)showSheet {
  [overlayWindow_ setIgnoresMouseEvents:NO];
  if (sheetDidShow_) {
    [sheet_ unhideSheet];
  } else {
    [sheet_ showSheetForWindow:overlayWindow_];
    sheetDidShow_ = YES;
  }

  // The call to -addChildWindow:ordered: below works around a macOS bug
  // (rdar://35418050) in 10.12 through (at least) 10.13.1: If a window (A) has
  // a child window (B), and B has a sheet (C), then adding another child
  // window (D) to A causes the sheet (C) to move behind its parent (B).
  //
  // This happens when the client certificate selector appears and then, after
  // a timeout, the status bubble appears to indicate that the page is still
  // loading.
  //
  //       ╭────────────────────────────╮
  //       │     Browser window (A)     │
  //       │ ┌───┬───────────────────┬──┼─┐
  //       │ │   │ Cert selector (C) │  │ │
  //       │ │   ╰───────────────────╯  │ │
  //     ╭ ├─┼─────╮ Overlay window (B) │ │
  //     ├ ╰─┼─────┴────────────────────╯ │
  //     │   └────────────────────────────┘
  //     ╰ Status bubble (D)
  //
  // Explicitly adding the sheet as a child window seems to let it participate
  // in the window ordering process that happens when the root window gains a
  // child so that it stays in front of its parent as expected.

  if (NSWindow* sheet = [overlayWindow_ attachedSheet])
    [overlayWindow_ addChildWindow:sheet ordered:NSWindowAbove];

  [sheet_ makeSheetKeyAndOrderFront];
}

@end
