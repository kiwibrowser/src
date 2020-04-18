// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_SCREEN_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_SCREEN_H_

#include "ui/views/views_export.h"

namespace display {
class Screen;
}

namespace views {

// Creates a Screen that represents the screen of the environment that hosts
// a WindowTreeHost. Caller owns the result.
VIEWS_EXPORT display::Screen* CreateDesktopScreen();

VIEWS_EXPORT void InstallDesktopScreenIfNecessary();

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_SCREEN_H_
