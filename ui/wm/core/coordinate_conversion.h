// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_WM_CORE_COORDINATE_CONVERSION_H_
#define UI_WM_CORE_COORDINATE_CONVERSION_H_

#include "ui/wm/core/wm_core_export.h"

namespace aura {
class Window;
}  // namespace aura

namespace gfx {
class Point;
class Rect;
}  // namespace gfx

namespace wm {

// Converts the |point| from a given |window|'s coordinates into the screen
// coordinates.
WM_CORE_EXPORT void ConvertPointToScreen(const aura::Window* window,
                                         gfx::Point* point);

// Converts the |point| from the screen coordinates to a given |window|'s
// coordinates.
WM_CORE_EXPORT void ConvertPointFromScreen(const aura::Window* window,
                                           gfx::Point* point_in_screen);

// Converts |rect| from |window|'s coordinates to the virtual screen
// coordinates.
WM_CORE_EXPORT void ConvertRectToScreen(const aura::Window* window,
                                        gfx::Rect* rect);

// Converts |rect| from virtual screen coordinates to the |window|'s
// coordinates.
WM_CORE_EXPORT void ConvertRectFromScreen(const aura::Window* window,
                                          gfx::Rect* rect_in_screen);

}  // namespace wm

#endif  // UI_WM_CORE_COORDINATE_CONVERSION_H_
