// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_SELECTION_H_
#define UI_OZONE_PLATFORM_SELECTION_H_

#include "ui/ozone/ozone_export.h"
#include "ui/ozone/platform_list.h"

namespace ui {

// Get active platform id (by parsing --ozone-platform flag).
OZONE_EXPORT int GetOzonePlatformId();

// Get active platform name.
OZONE_EXPORT const char* GetOzonePlatformName();

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_SELECTION_H_
