// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_DISPLAY_SWITCHES_H_
#define UI_DISPLAY_DISPLAY_SWITCHES_H_

#include "base/compiler_specific.h"
#include "base/feature_list.h"
#include "build/build_config.h"
#include "ui/display/display_export.h"

namespace switches {

// Keep sorted.
DISPLAY_EXPORT extern const char kDisableMultiMirroring[];
DISPLAY_EXPORT extern const char kEnableSoftwareMirroring[];
DISPLAY_EXPORT extern const char kEnsureForcedColorProfile[];
DISPLAY_EXPORT extern const char kForceColorProfile[];
DISPLAY_EXPORT extern const char kForceDeviceScaleFactor[];
// TODO(kylechar): This overlaps with --screen-config. Unify flags and remove.
DISPLAY_EXPORT extern const char kHostWindowBounds[];
DISPLAY_EXPORT extern const char kScreenConfig[];
DISPLAY_EXPORT extern const char kSecondaryDisplayLayout[];
DISPLAY_EXPORT extern const char kUseFirstDisplayAsInternal[];

#if defined(OS_CHROMEOS)
DISPLAY_EXPORT extern const char kEnableUnifiedDesktop[];
#endif

}  // namespace switches

namespace features {

DISPLAY_EXPORT extern const base::Feature kHighDynamicRange;

#if defined(OS_CHROMEOS)
DISPLAY_EXPORT extern const base::Feature kUseMonitorColorSpace;
#endif

DISPLAY_EXPORT extern const base::Feature kEnableDisplayZoomSetting;

// Returns true if experimental display zoom setting is enabled.
DISPLAY_EXPORT bool IsDisplayZoomSettingEnabled();

}  // namespace features

#endif  // UI_DISPLAY_DISPLAY_SWITCHES_H_
