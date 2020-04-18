// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_WIN_TOUCH_INPUT_H_
#define UI_BASE_WIN_TOUCH_INPUT_H_

#include <windows.h>

#include "ui/base/ui_base_export.h"

namespace ui {

// Wrapper for GetTouchInputInfo, which is not defined before Win7. For
// earlier OS's, this function returns FALSE.
UI_BASE_EXPORT BOOL GetTouchInputInfoWrapper(HTOUCHINPUT handle,
                                             UINT count,
                                             PTOUCHINPUT pointer,
                                             int size);

}  // namespace ui

#endif  // UI_BASE_WIN_TOUCH_INPUT_H_
