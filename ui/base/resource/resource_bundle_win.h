// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_RESOURCE_RESOURCE_BUNDLE_WIN_
#define UI_BASE_RESOURCE_RESOURCE_BUNDLE_WIN_

#include "build/build_config.h"

#include <windows.h>

#include "ui/base/ui_base_export.h"

namespace ui {

// NOTE: This needs to be called before initializing ResourceBundle if your
// resources are not stored in the executable.
UI_BASE_EXPORT void SetResourcesDataDLL(HINSTANCE handle);

// Loads and returns an icon from the app module.
UI_BASE_EXPORT HICON LoadThemeIconFromResourcesDataDLL(int icon_id);

}  // namespace ui

#endif  // UI_BASE_RESOURCE_RESOURCE_DATA_DLL_WIN_H_
