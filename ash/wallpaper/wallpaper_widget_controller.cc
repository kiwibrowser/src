// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wallpaper/wallpaper_widget_controller.h"

#include <utility>

#include "ash/ash_export.h"
#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ash/wallpaper/wallpaper_controller.h"
#include "base/scoped_observer.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace ash {

// Observes a wallpaper widget state, and notifies WallpaperWidgetController
// about relevant widget changes:
//  * when widget is being destroyed
//  * when the widgets implicit animations finish.
// Additionally, provides methods to manage wallpaper widget state - e.g. to
// show widget, reparent widget, or change the widget blur.
class WallpaperWidgetController::WidgetHandler
    : public ui::ImplicitAnimationObserver,
      public views::WidgetObserver,
      public aura::WindowObserver {
 public:
  WidgetHandler(WallpaperWidgetController* controller, views::Widget* widget)
      : controller_(controller),
        widget_(widget),
        parent_window_(widget->GetNativeWindow()->parent()),
        widget_observer_(this),
        window_observer_(this) {
    DCHECK(controller_);
    DCHECK(widget_);
    widget_observer_.Add(widget_);
    window_observer_.Add(parent_window_);
  }

  ~WidgetHandler() override { Reset(true /*close*/); }

  views::Widget* widget() { return widget_; }

  // ui::ImplicitAnimationObserver:
  void OnImplicitAnimationsCompleted() override {
    observing_implicit_animations_ = false;
    StopObservingImplicitAnimations();

    controller_->WidgetFinishedAnimating(this);
  }

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override {
    Reset(false /*close*/);

    // NOTE: Do not use |this| past this point - |controller_| will delete this
    // instance.
    controller_->WidgetHandlerReset(this);
  }

  // aura::WindowObserver:
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override {
    widget_->SetBounds(new_bounds);
  }

  void Show() {
    ui::ScopedLayerAnimationSettings settings(
        widget_->GetLayer()->GetAnimator());
    observing_implicit_animations_ = true;
    settings.AddObserver(this);

    // When |widget_| shows, AnimateShowWindowCommon() is called to do the
    // animation. Sets transition duration to 0 to avoid animating to the
    // show animation's initial values.
    settings.SetTransitionDuration(base::TimeDelta());
    widget_->Show();
  }

  bool Reparent(aura::Window* new_parent) {
    if (parent_window_ == new_parent)
      return false;

    window_observer_.Remove(parent_window_);
    if (has_blur_cache_)
      parent_window_->layer()->RemoveCacheRenderSurfaceRequest();

    new_parent->AddChild(widget_->GetNativeWindow());

    parent_window_ = widget_->GetNativeWindow()->parent();
    window_observer_.Add(parent_window_);

    has_blur_cache_ = widget_->GetLayer()->layer_blur() > 0.0f;
    if (has_blur_cache_)
      parent_window_->layer()->AddCacheRenderSurfaceRequest();

    return true;
  }

  void SetBlur(float blur_sigma) {
    widget_->GetLayer()->SetLayerBlur(blur_sigma);

    const bool old_has_blur_cache = has_blur_cache_;
    has_blur_cache_ = blur_sigma > 0.0f;
    if (!old_has_blur_cache && has_blur_cache_) {
      parent_window_->layer()->AddCacheRenderSurfaceRequest();
    } else if (old_has_blur_cache && !has_blur_cache_) {
      parent_window_->layer()->RemoveCacheRenderSurfaceRequest();
    }
  }

  void StopAnimating() { widget_->GetLayer()->GetAnimator()->StopAnimating(); }

 private:
  void Reset(bool close) {
    if (reset_)
      return;
    reset_ = true;

    window_observer_.RemoveAll();
    widget_observer_.RemoveAll();

    if (observing_implicit_animations_) {
      observing_implicit_animations_ = false;
      StopObservingImplicitAnimations();
    }

    if (has_blur_cache_)
      parent_window_->layer()->RemoveCacheRenderSurfaceRequest();
    parent_window_ = nullptr;

    if (close)
      widget_->CloseNow();
    widget_ = nullptr;
  }

  WallpaperWidgetController* controller_;
  views::Widget* widget_;
  aura::Window* parent_window_;

  bool reset_ = false;
  bool has_blur_cache_ = false;
  bool observing_implicit_animations_ = false;

  ScopedObserver<views::Widget, WidgetHandler> widget_observer_;
  ScopedObserver<aura::Window, WidgetHandler> window_observer_;

  DISALLOW_COPY_AND_ASSIGN(WidgetHandler);
};

WallpaperWidgetController::WallpaperWidgetController(
    base::OnceClosure wallpaper_set_callback)
    : wallpaper_set_callback_(std::move(wallpaper_set_callback)) {}

WallpaperWidgetController::~WallpaperWidgetController() = default;

views::Widget* WallpaperWidgetController::GetWidget() {
  if (!active_widget_)
    return nullptr;
  return active_widget_->widget();
}

views::Widget* WallpaperWidgetController::GetAnimatingWidget() {
  if (!animating_widget_)
    return nullptr;
  return animating_widget_->widget();
}

bool WallpaperWidgetController::IsAnimating() const {
  return animating_widget_.get();
}

void WallpaperWidgetController::EndPendingAnimation() {
  if (!IsAnimating())
    return;
  animating_widget_->StopAnimating();
}

void WallpaperWidgetController::AddPendingAnimationEndCallback(
    base::OnceClosure callback) {
  DCHECK(IsAnimating());
  pending_animation_end_callbacks_.emplace_back(std::move(callback));
}

void WallpaperWidgetController::SetWallpaperWidget(views::Widget* widget,
                                                   float blur_sigma) {
  DCHECK(widget);

  // If there is a widget currently being shown, finish the animation and set it
  // as the primary widget, before starting transition to the new wallpaper.
  if (animating_widget_) {
    SetAnimatingWidgetAsActive();
    active_widget_->StopAnimating();
  }

  animating_widget_ = std::make_unique<WidgetHandler>(this, widget);
  animating_widget_->SetBlur(blur_sigma);
  animating_widget_->Show();
}

bool WallpaperWidgetController::Reparent(aura::Window* root_window,
                                         int container) {
  aura::Window* new_parent = root_window->GetChildById(container);

  bool moved_widget = active_widget_ && active_widget_->Reparent(new_parent);
  bool moved_animating_widget =
      animating_widget_ && animating_widget_->Reparent(new_parent);
  return moved_widget || moved_animating_widget;
}

void WallpaperWidgetController::SetWallpaperBlur(float blur_sigma) {
  if (animating_widget_)
    animating_widget_->SetBlur(blur_sigma);
  if (active_widget_)
    active_widget_->SetBlur(blur_sigma);
}

void WallpaperWidgetController::ResetWidgetsForTesting() {
  animating_widget_.reset();
  active_widget_.reset();
}

void WallpaperWidgetController::WidgetHandlerReset(WidgetHandler* widget) {
  if (widget == active_widget_.get()) {
    SetAnimatingWidgetAsActive();
    if (active_widget_)
      active_widget_->StopAnimating();
  } else if (widget == animating_widget_.get()) {
    animating_widget_.reset();
  }
}

void WallpaperWidgetController::WidgetFinishedAnimating(WidgetHandler* widget) {
  if (widget != animating_widget_.get())
    return;

  SetAnimatingWidgetAsActive();
}

void WallpaperWidgetController::SetAnimatingWidgetAsActive() {
  active_widget_ = std::move(animating_widget_);

  if (!active_widget_)
    return;

  if (wallpaper_set_callback_)
    std::move(wallpaper_set_callback_).Run();

  // Notify observers that animation finished.
  RunPendingAnimationEndCallbacks();
  Shell::Get()->wallpaper_controller()->OnWallpaperAnimationFinished();
}

void WallpaperWidgetController::RunPendingAnimationEndCallbacks() {
  std::list<base::OnceClosure> callbacks;
  pending_animation_end_callbacks_.swap(callbacks);
  for (auto& callback : callbacks)
    std::move(callback).Run();
}

}  // namespace ash
