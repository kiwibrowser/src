// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_FOCUS_WINDOW_SET_H_
#define UI_BASE_COCOA_FOCUS_WINDOW_SET_H_

#include <set>

#include "ui/base/ui_base_export.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {

// Brings a group of windows to the front without changing their order, and
// makes the frontmost one key and main. If none are visible, the frontmost
// miniaturized window is deminiaturized.
UI_BASE_EXPORT void FocusWindowSet(const std::set<gfx::NativeWindow>& windows);

// Brings a group of windows to the front without changing their
// order, and makes the frontmost one key and main. If none are
// visible, the frontmost miniaturized window is deminiaturized. This
// variant is meant to clean up after the system-default Dock icon
// behavior. Unlike FocusWindowSet, only windows on the current space
// are considered. It also ignores the hidden state of windows; the
// window system may be in the middle of unhiding the application.
UI_BASE_EXPORT void FocusWindowSetOnCurrentSpace(
    const std::set<gfx::NativeWindow>& windows);

}  // namespace ui

#endif  // UI_BASE_COCOA_FOCUS_WINDOW_SET_H_
