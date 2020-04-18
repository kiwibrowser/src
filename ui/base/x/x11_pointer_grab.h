// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_X_X11_POINTER_GRAB_H_
#define UI_BASE_X_X11_POINTER_GRAB_H_

#include "ui/base/x/ui_base_x_export.h"
#include "ui/gfx/x/x11_types.h"

typedef unsigned long Cursor;

namespace ui {

// Grabs the pointer. It is unnecessary to ungrab the pointer prior to grabbing
// it.
UI_BASE_X_EXPORT int GrabPointer(XID window,
                                 bool owner_events,
                                 ::Cursor cursor);

// Sets the cursor to use for the duration of the active pointer grab.
UI_BASE_X_EXPORT void ChangeActivePointerGrabCursor(::Cursor cursor);

// Ungrabs the pointer.
UI_BASE_X_EXPORT void UngrabPointer();

}  // namespace ui

#endif  // UI_BASE_X_X11_POINTER_GRAB_H_
