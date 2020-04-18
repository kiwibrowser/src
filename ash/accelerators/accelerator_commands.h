// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCELERATORS_ACCELERATOR_COMMANDS_H_
#define ASH_ACCELERATORS_ACCELERATOR_COMMANDS_H_

#include "ash/ash_export.h"

// This file contains implementations of commands that are bound to keyboard
// shortcuts in Ash or in the embedding application (e.g. Chrome).
namespace ash {
namespace accelerators {

// True if the desktop zoom accelerator is enabled.
ASH_EXPORT bool IsInternalDisplayZoomEnabled();

// Change internal display zooming up or down.
ASH_EXPORT bool ZoomDisplay(bool up);

// Reset the display zooming to the default state.
ASH_EXPORT void ResetDisplayZoom();

// Minimizes the active window, if present. If no windows are active, restores
// the first unminimized window. Returns true if a window was minimized or
// restored.
ASH_EXPORT bool ToggleMinimized();

// Toggles the maxmized state. If the window is in fulllscreen, it exits
// fullscreen mode.
ASH_EXPORT void ToggleMaximized();

// Toggles the fullscreen state. The behavior can be overridden
// by WindowStateDelegate::ToggleFullscreen().
ASH_EXPORT void ToggleFullscreen();

// True if the user can press a key to exit pinned mode (aka forced
// fullscreen).
ASH_EXPORT bool CanUnpinWindow();

// If a window is pinned (aka forced fullscreen), exit from pinned mode.
ASH_EXPORT void UnpinWindow();

}  // namespace accelerators
}  // namespace ash

#endif  // ASH_ACCELERATORS_ACCELERATOR_COMMANDS_H_
