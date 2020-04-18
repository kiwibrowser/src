// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_APPKIT_UTILS_H
#define UI_BASE_COCOA_APPKIT_UTILS_H

#import <Cocoa/Cocoa.h>

#include "ui/base/ui_base_export.h"

namespace ui {

struct NinePartImageIds {
  int top_left;
  int top;
  int top_right;
  int left;
  int center;
  int right;
  int bottom_left;
  int bottom;
  int bottom_right;
};

// A macro to define arrays of IDR constants used with DrawNinePartImage.
#define IMAGE_GRID(x) { x ## _TOP_LEFT,    x ## _TOP,    x ## _TOP_RIGHT, \
                        x ## _LEFT,        x ## _CENTER, x ## _RIGHT, \
                        x ## _BOTTOM_LEFT, x ## _BOTTOM, x ## _BOTTOM_RIGHT, }

// Utility method to draw a nine part image using image ids.
UI_BASE_EXPORT void DrawNinePartImage(NSRect frame,
                                      const NinePartImageIds& image_ids,
                                      NSCompositingOperation operation,
                                      CGFloat alpha,
                                      BOOL flipped);

// Minimizes or zooms the window, or does nothing, depending on OS version and
// system configuration.
// |window| is the receiver of the double click.
// |sender| is the object that sends the action, if one is sent.
UI_BASE_EXPORT void WindowTitlebarReceivedDoubleClick(NSWindow* window,
                                                      id sender);

// Whether a force-click event on the touchpad should invoke Quick Look.
UI_BASE_EXPORT bool ForceClickInvokesQuickLook();

// Returns true if both CGFloat values are equal.
UI_BASE_EXPORT bool IsCGFloatEqual(CGFloat a, CGFloat b);

}  // namespace ui

#endif  // UI_BASE_COCOA_APPKIT_UTILS_H
