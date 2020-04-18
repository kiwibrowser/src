// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_GRAPHICS_CAST_WINDOW_MANAGER_DEFAULT_H_
#define CHROMECAST_GRAPHICS_CAST_WINDOW_MANAGER_DEFAULT_H_

#include <memory>

#include "base/macros.h"
#include "chromecast/graphics/cast_window_manager.h"

namespace chromecast {

class CastWindowManagerDefault : public CastWindowManager {
 public:
  ~CastWindowManagerDefault() override;

  // CastWindowManager implementation:
  void TearDown() override;
  void AddWindow(gfx::NativeView window) override;
  void SetWindowId(gfx::NativeView window, WindowId window_id) override;
  gfx::NativeView GetRootWindow() override;
  void InjectEvent(ui::Event* event) override;

  void AddSideSwipeGestureHandler(
      CastSideSwipeGestureHandlerInterface* handler) override;

  void RemoveSideSwipeGestureHandler(
      CastSideSwipeGestureHandlerInterface* handler) override;

  void SetColorInversion(bool enable) override;

 private:
  friend class CastWindowManager;

  // This class should only be instantiated by CastWindowManager::Create.
  CastWindowManagerDefault();

  DISALLOW_COPY_AND_ASSIGN(CastWindowManagerDefault);
};

}  // namespace chromecast

#endif  // CHROMECAST_GRAPHICS_CAST_WINDOW_MANAGER_DEFAULT_H_
