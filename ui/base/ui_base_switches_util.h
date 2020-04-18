// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_UI_BASE_SWITCHES_UTIL_H_
#define UI_BASE_UI_BASE_SWITCHES_UTIL_H_

#include "ui/base/ui_base_export.h"

namespace switches {

UI_BASE_EXPORT bool IsLinkDisambiguationPopupEnabled();
UI_BASE_EXPORT bool IsTouchDragDropEnabled();

// Returns whether the touchable app context menu switch has been set. Prefer
// features::IsTouchableAppContextMenuEnabled().
UI_BASE_EXPORT bool IsTouchableAppContextMenuEnabled();

}  // namespace switches

#endif  // UI_BASE_UI_BASE_SWITCHES_UTIL_H_
