// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_SCOPED_OVERVIEW_ANIMATION_SETTINGS_H_
#define ASH_WM_OVERVIEW_SCOPED_OVERVIEW_ANIMATION_SETTINGS_H_

#include <memory>

#include "ash/wm/overview/overview_animation_type.h"
#include "base/macros.h"

namespace aura {
class Window;
}  // namespace aura

namespace ui {
class ImplicitAnimationObserver;
class ScopedLayerAnimationSettings;
}  // namespace ui

namespace ash {

// ScopedOverviewAnimationSettings correctly configures the animation
// settings for an aura::Window given an OverviewAnimationType.
class ScopedOverviewAnimationSettings {
 public:
  ScopedOverviewAnimationSettings(OverviewAnimationType animation_type,
                                  aura::Window* window);
  ~ScopedOverviewAnimationSettings();
  void AddObserver(ui::ImplicitAnimationObserver* observer);
  void CacheRenderSurface();
  void DeferPaint();
  void TrilinearFiltering();

 private:
  // The managed animation settings.
  std::unique_ptr<ui::ScopedLayerAnimationSettings> animation_settings_;

  DISALLOW_COPY_AND_ASSIGN(ScopedOverviewAnimationSettings);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_SCOPED_OVERVIEW_ANIMATION_SETTINGS_H_
