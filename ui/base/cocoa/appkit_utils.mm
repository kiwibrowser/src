// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/appkit_utils.h"

#include <cmath>

#include "base/mac/mac_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

// Double-click in window title bar actions.
enum class DoubleClickAction {
  NONE,
  MINIMIZE,
  MAXIMIZE,
};

// Values of com.apple.trackpad.forceClick corresponding to "Look up & data
// detectors" in System Preferences -> Trackpad -> Point & Click.
enum class ForceTouchAction {
  NONE = 0,        // Unchecked or set to "Tap with three fingers".
  QUICK_LOOK = 1,  // Set to "Force Click with one finger".
};

// Gets an NSImage given an image id.
NSImage* GetImage(int image_id) {
  return ui::ResourceBundle::GetSharedInstance()
      .GetNativeImageNamed(image_id)
      .ToNSImage();
}

// The action to take when the user double-clicks in the window title bar.
DoubleClickAction WindowTitleBarDoubleClickAction() {
  // El Capitan introduced a Dock preference to configure the window title bar
  // double-click action (Minimize, Maximize, or nothing).
  if (base::mac::IsAtLeastOS10_11()) {
    NSString* doubleClickAction = [[NSUserDefaults standardUserDefaults]
                                      objectForKey:@"AppleActionOnDoubleClick"];

    if ([doubleClickAction isEqualToString:@"Minimize"]) {
      return DoubleClickAction::MINIMIZE;
    } else if ([doubleClickAction isEqualToString:@"Maximize"]) {
      return DoubleClickAction::MAXIMIZE;
    }

    return DoubleClickAction::NONE;
  }

  // Determine minimize using an undocumented method in Cocoa. If we're
  // running on an earlier version of the OS that doesn't implement it,
  // just default to the minimize action.
  BOOL methodImplemented =
      [NSWindow respondsToSelector:@selector(_shouldMiniaturizeOnDoubleClick)];
  if (!methodImplemented ||
      [NSWindow performSelector:@selector(_shouldMiniaturizeOnDoubleClick)]) {
    return DoubleClickAction::MINIMIZE;
  }

  // At this point _shouldMiniaturizeOnDoubleClick has returned |NO|. On
  // Yosemite, that means a double-click should Maximize the window, and on
  // all prior OSes a double-click should do nothing.
  return base::mac::IsOS10_10() ? DoubleClickAction::MAXIMIZE
                                : DoubleClickAction::NONE;
}

}  // namespace

namespace ui {

void DrawNinePartImage(NSRect frame,
                       const NinePartImageIds& image_ids,
                       NSCompositingOperation operation,
                       CGFloat alpha,
                       BOOL flipped) {
  NSDrawNinePartImage(frame,
                      GetImage(image_ids.top_left),
                      GetImage(image_ids.top),
                      GetImage(image_ids.top_right),
                      GetImage(image_ids.left),
                      GetImage(image_ids.center),
                      GetImage(image_ids.right),
                      GetImage(image_ids.bottom_left),
                      GetImage(image_ids.bottom),
                      GetImage(image_ids.bottom_right),
                      operation,
                      alpha,
                      flipped);
}

void WindowTitlebarReceivedDoubleClick(NSWindow* window, id sender) {
  switch (WindowTitleBarDoubleClickAction()) {
    case DoubleClickAction::MINIMIZE:
      [window performMiniaturize:sender];
      break;

    case DoubleClickAction::MAXIMIZE:
      [window performZoom:sender];
      break;

    case DoubleClickAction::NONE:
      break;
  }
}

bool ForceClickInvokesQuickLook() {
  return [[NSUserDefaults standardUserDefaults]
             integerForKey:@"com.apple.trackpad.forceClick"] ==
         static_cast<NSInteger>(ForceTouchAction::QUICK_LOOK);
}

bool IsCGFloatEqual(CGFloat a, CGFloat b) {
  return std::fabs(a - b) <= std::numeric_limits<CGFloat>::epsilon();
}

}  // namespace ui
