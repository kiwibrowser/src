// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/workspace_controller.h"

#include <utility>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/root_window_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/wm/fullscreen_window_finder.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/window_state.h"
#include "ash/wm/wm_window_animations.h"
#include "ash/wm/workspace/backdrop_controller.h"
#include "ash/wm/workspace/backdrop_delegate.h"
#include "ash/wm/workspace/workspace_event_handler.h"
#include "ash/wm/workspace/workspace_layout_manager.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/wm/core/window_animations.h"

namespace ash {
namespace {

// Amount of time to pause before animating anything. Only used during initial
// animation (when logging in).
const int kInitialPauseTimeMS = 750;

// The duration of the animation that occurs on first login.
const int kInitialAnimationDurationMS = 200;

}  // namespace

WorkspaceController::WorkspaceController(aura::Window* viewport)
    : viewport_(viewport),
      event_handler_(ShellPort::Get()->CreateWorkspaceEventHandler(viewport)),
      layout_manager_(new WorkspaceLayoutManager(viewport)) {
  viewport_->AddObserver(this);
  ::wm::SetWindowVisibilityAnimationTransition(viewport_, ::wm::ANIMATE_NONE);
  viewport_->SetLayoutManager(layout_manager_);
}

WorkspaceController::~WorkspaceController() {
  if (!viewport_)
    return;

  viewport_->RemoveObserver(this);
  viewport_->SetLayoutManager(nullptr);
}

wm::WorkspaceWindowState WorkspaceController::GetWindowState() const {
  if (!viewport_)
    return wm::WORKSPACE_WINDOW_STATE_DEFAULT;

  const aura::Window* fullscreen = wm::GetWindowForFullscreenMode(viewport_);
  if (fullscreen && !wm::GetWindowState(fullscreen)->ignored_by_shelf())
    return wm::WORKSPACE_WINDOW_STATE_FULL_SCREEN;

  const gfx::Rect shelf_bounds(Shelf::ForWindow(viewport_)->GetIdealBounds());
  bool window_overlaps_launcher = false;
  auto mru_list =
      Shell::Get()->mru_window_tracker()->BuildWindowListIgnoreModal();

  for (aura::Window* window : mru_list) {
    if (window->GetRootWindow() != viewport_->GetRootWindow())
      continue;
    wm::WindowState* window_state = wm::GetWindowState(window);
    if (window_state->ignored_by_shelf() ||
        (window->layer() && !window->layer()->GetTargetVisibility())) {
      continue;
    }
    if (window_state->IsMaximized())
      return wm::WORKSPACE_WINDOW_STATE_MAXIMIZED;
    window_overlaps_launcher |= window->bounds().Intersects(shelf_bounds);
  }

  return window_overlaps_launcher
             ? wm::WORKSPACE_WINDOW_STATE_WINDOW_OVERLAPS_SHELF
             : wm::WORKSPACE_WINDOW_STATE_DEFAULT;
}

void WorkspaceController::DoInitialAnimation() {
  viewport_->Show();

  ui::Layer* layer = viewport_->layer();
  layer->SetOpacity(0.0f);
  SetTransformForScaleAnimation(layer, LAYER_SCALE_ANIMATION_ABOVE);

  // In order for pause to work we need to stop animations.
  layer->GetAnimator()->StopAnimating();

  {
    ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());

    settings.SetPreemptionStrategy(ui::LayerAnimator::ENQUEUE_NEW_ANIMATION);
    layer->GetAnimator()->SchedulePauseForProperties(
        base::TimeDelta::FromMilliseconds(kInitialPauseTimeMS),
        ui::LayerAnimationElement::TRANSFORM |
            ui::LayerAnimationElement::OPACITY |
            ui::LayerAnimationElement::BRIGHTNESS |
            ui::LayerAnimationElement::VISIBILITY);
    settings.SetTweenType(gfx::Tween::EASE_OUT);
    settings.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(kInitialAnimationDurationMS));
    layer->SetTransform(gfx::Transform());
    layer->SetOpacity(1.0f);
  }
}

void WorkspaceController::SetBackdropDelegate(
    std::unique_ptr<BackdropDelegate> delegate) {
  layout_manager_->SetBackdropDelegate(std::move(delegate));
}

void WorkspaceController::OnWindowDestroying(aura::Window* window) {
  DCHECK_EQ(window, viewport_);
  viewport_->RemoveObserver(this);
  viewport_ = nullptr;
  // Destroy |event_handler_| too as it depends upon |window|.
  event_handler_.reset();
  layout_manager_ = nullptr;
}

}  // namespace ash
