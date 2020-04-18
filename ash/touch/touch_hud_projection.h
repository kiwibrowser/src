// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TOUCH_TOUCH_HUD_PROJECTION_H_
#define ASH_TOUCH_TOUCH_HUD_PROJECTION_H_

#include <map>

#include "ash/touch/touch_observer_hud.h"
#include "base/macros.h"

namespace ash {
class TouchHudRenderer;

// A heads-up display to show active touch points on the screen. As a derivative
// of TouchObserverHUD, objects of this class manage their own lifetime.
class TouchHudProjection : public TouchObserverHUD {
 public:
  explicit TouchHudProjection(aura::Window* initial_root);

  // Overriden from TouchObserverHUD.
  void Clear() override;

 private:
  friend class TouchHudProjectionTest;

  ~TouchHudProjection() override;

  // Overriden from TouchObserverHUD.
  void OnTouchEvent(ui::TouchEvent* event) override;
  void SetHudForRootWindowController(RootWindowController* controller) override;
  void UnsetHudForRootWindowController(
      RootWindowController* controller) override;

  // TouchHudRenderer draws out the touch points.
  std::unique_ptr<TouchHudRenderer> touch_hud_renderer_;

  DISALLOW_COPY_AND_ASSIGN(TouchHudProjection);
};

}  // namespace ash

#endif  // ASH_TOUCH_TOUCH_HUD_PROJECTION_H_
