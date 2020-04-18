// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_WM_CORE_WINDOW_PROPERTIES_H_
#define UI_WM_CORE_WINDOW_PROPERTIES_H_

#include "ui/base/class_property.h"
#include "ui/wm/core/wm_core_export.h"

namespace wm {

// Property to tell if the container uses screen coordinates for the child
// windows.
WM_CORE_EXPORT extern const ui::ClassProperty<bool>* const
    kUsesScreenCoordinatesKey;

}  // namespace wm

#endif  // UI_WM_CORE_WINDOW_PROPERTIES_H_
