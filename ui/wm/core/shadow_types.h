// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_WM_CORE_SHADOW_TYPES_H_
#define UI_WM_CORE_SHADOW_TYPES_H_

#include "ui/aura/window.h"
#include "ui/wm/core/wm_core_export.h"

namespace wm {

// Indicates an elevation should be chosen based on the window. This is used
// by wm::ShadowController, but is not a valid elevation to pass to wm::Shadow.
constexpr int kShadowElevationDefault = -1;

// Different types of drop shadows that can be drawn under a window by the
// shell. Used as a value for the kShadowElevationKey property.
constexpr int kShadowElevationNone = 0;

// Standard shadow elevations used by the the aura window manager. The value is
// used to initialize an instance of wm::Shadow and controls the offset and blur
// of the shadow style created by gfx::ShadowValue::MakeMdShadowValues().
constexpr int kShadowElevationMenuOrTooltip = 6;
constexpr int kShadowElevationInactiveWindow = 8;
constexpr int kShadowElevationActiveWindow = 24;

WM_CORE_EXPORT void SetShadowElevation(aura::Window* window, int elevation);

// A property key describing the drop shadow that should be displayed under the
// window. A null value is interpreted as using the default.
WM_CORE_EXPORT extern const aura::WindowProperty<int>* const
    kShadowElevationKey;

}  // namespace wm

#endif  // UI_WM_CORE_SHADOW_TYPES_H_
