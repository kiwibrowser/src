// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_FULLSCREEN_WINDOW_FINDER_H_
#define ASH_WM_FULLSCREEN_WINDOW_FINDER_H_

#include "ash/ash_export.h"

namespace aura {
class Window;
}

namespace ash {
namespace wm {

// Returns the topmost window or one of its transient parents, if any of them
// are in fullscreen mode. This searches for a window in the root of |context|.
ASH_EXPORT aura::Window* GetWindowForFullscreenMode(aura::Window* context);

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_FULLSCREEN_WINDOW_FINDER_H_
