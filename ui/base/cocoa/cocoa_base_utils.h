// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_COCOA_BASE_UTILS_H_
#define UI_BASE_COCOA_COCOA_BASE_UTILS_H_

#import <Cocoa/Cocoa.h>

#include "ui/base/ui_base_export.h"
#include "ui/base/window_open_disposition.h"

namespace ui {

// Retrieves the WindowOpenDisposition used to open a link from a user gesture
// represented by |event|. For example, a Cmd+Click would mean open the
// associated link in a background tab.
UI_BASE_EXPORT WindowOpenDisposition
    WindowOpenDispositionFromNSEvent(NSEvent* event);

// Retrieves the WindowOpenDisposition used to open a link from a user gesture
// represented by |event|, but instead use the modifier flags given by |flags|,
// which is the same format as |-NSEvent modifierFlags|. This allows
// substitution of the modifiers without having to create a new event from
// scratch.
UI_BASE_EXPORT WindowOpenDisposition
    WindowOpenDispositionFromNSEventWithFlags(NSEvent* event, NSUInteger flags);

// Converts a point from window coordinates to screen coordinates.
UI_BASE_EXPORT NSPoint ConvertPointFromWindowToScreen(NSWindow* window,
                                                      NSPoint point);

// Converts a point from screen coordinates to window coordinates.
UI_BASE_EXPORT NSPoint ConvertPointFromScreenToWindow(NSWindow* window,
                                                      NSPoint point);

}  // namespace ui

#endif  // UI_BASE_COCOA_COCOA_BASE_UTILS_H_
