// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_WIN_HIDDEN_WINDOW_H_
#define UI_BASE_WIN_HIDDEN_WINDOW_H_

#include <windows.h>

#include "ui/base/ui_base_export.h"

namespace ui {

// Returns an HWND that can be used as a temporary parent. The returned HWND is
// never destroyed.
UI_BASE_EXPORT HWND GetHiddenWindow();

}  // namespace ui

#endif  // UI_BASE_WIN_HIDDEN_WINDOW_H_
