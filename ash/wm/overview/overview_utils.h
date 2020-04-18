// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_OVERVIEW_UTILS_H_
#define ASH_WM_OVERVIEW_OVERVIEW_UTILS_H_

#include "ash/ash_export.h"
#include "ash/wm/overview/overview_animation_type.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/compositor/layer_type.h"

namespace aura {
class Window;
}  // namespace aura

namespace views {
class Widget;
}  // namespace views

namespace aura {
class Window;
}

namespace ash {

// Returns true if |window| can cover available workspace.
bool CanCoverAvailableWorkspace(aura::Window* window);

// Returns true if overview mode should use the new animations.
// TODO(wutao): Remove this function when the old overview mode animations
// become obsolete. See https://crbug.com/801465.
bool IsNewOverviewAnimationsEnabled();

bool IsOverviewSwipeToCloseEnabled();

// Fades |widget| to opacity zero with animation settings depending on
// |animation_type|. Used by several classes which need to be destroyed on
// exiting overview, but have some widgets which need to continue animating.
// |widget| is destroyed after finishing animation.
void FadeOutWidgetOnExit(std::unique_ptr<views::Widget> widget,
                         OverviewAnimationType animation_type);

// Creates and returns a background translucent widget parented in
// |root_window|'s default container and having |background_color|.
// When |border_thickness| is non-zero, a border is created having
// |border_color|, otherwise |border_color| parameter is ignored.
// The new background widget starts with |initial_opacity| and then fades in.
// If |parent| is prvoided the return widget will be parented to that window,
// otherwise its parent will be in kShellWindowId_WallpaperContainer of
// |root_window|.
std::unique_ptr<views::Widget> CreateBackgroundWidget(aura::Window* root_window,
                                                      ui::LayerType layer_type,
                                                      SkColor background_color,
                                                      int border_thickness,
                                                      int border_radius,
                                                      SkColor border_color,
                                                      float initial_opacity,
                                                      aura::Window* parent,
                                                      bool stack_on_top);
}  // namespace ash

#endif  // ASH_WM_OVERVIEW_OVERVIEW_UTILS_H_
