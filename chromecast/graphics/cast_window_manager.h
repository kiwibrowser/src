// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_GRAPHICS_CAST_WINDOW_MANAGER_H_
#define CHROMECAST_GRAPHICS_CAST_WINDOW_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {
class Event;
}  // namespace ui

namespace chromecast {

class CastSideSwipeGestureHandlerInterface;

// Chromecast's window-manager interface.
// This declares the interface to add top-level windows to the Chromecast
// platform window.  It is owned by the UI thread, and generally one instance
// should exist per platform root window (e.g., in Ozone, one per Ozone window).
class CastWindowManager {
 public:
  // Note: these window IDs are ordered by z-order.
  enum WindowId {
    BOTTOM = -1,
    APP = BOTTOM,
    DEBUG_OVERLAY,
    INFO_OVERLAY,
    SOFT_KEYBOARD,
    VOLUME,
    MEDIA_INFO,
    SETTINGS,
    TOP = MEDIA_INFO
  };

  // Creates the platform-specific CastWindowManager.
  static std::unique_ptr<CastWindowManager> Create(bool enable_input);

  virtual ~CastWindowManager() {}

  // Remove all windows and release all graphics resources.
  // Can be called multiple times.
  virtual void TearDown() = 0;

  // Adds a window to the window manager.
  // This doesn't necessarily make the window visible.
  // If the window manager hasn't been initialized, this has the side effect of
  // causing it to initialize.
  virtual void AddWindow(gfx::NativeView window) = 0;

  // Sets a window's ID.
  virtual void SetWindowId(gfx::NativeView window, WindowId window_id) = 0;

  // Return the root window that holds all top-level windows.
  virtual gfx::NativeView GetRootWindow() = 0;

  // Inject a UI event into the Cast window.
  virtual void InjectEvent(ui::Event* event) = 0;

  // Register a new handler for a system side swipe event.
  virtual void AddSideSwipeGestureHandler(
      CastSideSwipeGestureHandlerInterface* handler) = 0;

  // Remove the registration of a system side swipe event handler.
  virtual void RemoveSideSwipeGestureHandler(
      CastSideSwipeGestureHandlerInterface* handler) = 0;

  // Enable/Disable color inversion.
  virtual void SetColorInversion(bool enable) = 0;
};

}  // namespace chromecast

#endif  // CHROMECAST_GRAPHICS_CAST_WINDOW_MANAGER_H_
