// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_ROOT_WINDOW_FINDER_H_
#define ASH_WM_ROOT_WINDOW_FINDER_H_

#include "ash/ash_export.h"

namespace aura {
class Window;
}  // namespace aura

namespace gfx {
class Point;
class Rect;
}  // namespace gfx

namespace ash {

namespace wm {

// Returns the RootWindow at |point| in the virtual screen coordinates.
// Returns nullptr if the root window does not exist at the given point.
ASH_EXPORT aura::Window* GetRootWindowAt(const gfx::Point& point);

// Returns the RootWindow that shares the most area with |rect| in the virtual
// screen coordinates.
ASH_EXPORT aura::Window* GetRootWindowMatching(const gfx::Rect& rect);

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_ROOT_WINDOW_FINDER_H_
