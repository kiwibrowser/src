// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SCREEN_UTIL_H_
#define ASH_SCREEN_UTIL_H_

#include "ash/ash_export.h"

namespace aura {
class Window;
}  // namespace aura

namespace gfx {
class Rect;
}  // namespace gfx

namespace ash {

namespace screen_util {

// Returns the bounds for maximized windows in parent coordinates.
// Maximized windows trigger auto-hiding the shelf.
ASH_EXPORT gfx::Rect GetMaximizedWindowBoundsInParent(aura::Window* window);

// Returns the display bounds in parent coordinates.
ASH_EXPORT gfx::Rect GetDisplayBoundsInParent(aura::Window* window);

// Returns the display's work area bounds in parent coordinates.
ASH_EXPORT gfx::Rect GetDisplayWorkAreaBoundsInParent(aura::Window* window);

// Returns the display's work area bounds in parent coordinates on lock
// screen, i.e. for work area with forced bottom alignment.
// Note that unlike |GetDisplayWorkAreaBoundsInParent|, this method uses
// work area bounds that are updated when the screen is locked. For example
// if shelf alignment is set to right before screen lock,
// |GetDisplayWorkAreaBoundsInParent| will return work are bounds for right
// shelf alignment - this method will return work area for bottom shelf
// alignment (which is always used on lock screen).
ASH_EXPORT gfx::Rect GetDisplayWorkAreaBoundsInParentForLockScreen(
    aura::Window* window);

// Returns the bounds of the physical display containing the shelf for
// |window|. Physical displays can differ from logical displays in unified
// desktop mode.
// TODO(oshima): Consider using physical displays in window layout, instead of
// root windows, and only use logical display in display management code.
ASH_EXPORT gfx::Rect GetDisplayBoundsWithShelf(aura::Window* window);

}  // namespace screen_util

}  // namespace ash

#endif  // ASH_SCREEN_UTIL_H_
