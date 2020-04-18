// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/aura/overscroll_window_animation.h"

#include <algorithm>
#include <utility>

#include "base/i18n/rtl.h"
#include "content/browser/web_contents/aura/shadow_layer_delegate.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace content {

namespace {

OverscrollWindowAnimation::Direction GetDirectionForMode(OverscrollMode mode) {
  if (mode == (base::i18n::IsRTL() ? OVERSCROLL_EAST : OVERSCROLL_WEST))
    return OverscrollWindowAnimation::SLIDE_FRONT;
  if (mode == (base::i18n::IsRTL() ? OVERSCROLL_WEST : OVERSCROLL_EAST))
    return OverscrollWindowAnimation::SLIDE_BACK;
  return OverscrollWindowAnimation::SLIDE_NONE;
}

}  // namespace

OverscrollWindowAnimation::OverscrollWindowAnimation(Delegate* delegate)
    : delegate_(delegate),
      direction_(SLIDE_NONE),
      overscroll_cancelled_(false) {
  DCHECK(delegate_);
}

OverscrollWindowAnimation::~OverscrollWindowAnimation() {}

void OverscrollWindowAnimation::CancelSlide() {
  overscroll_cancelled_ = true;
  // Listen to the animation of the main window.
  bool main_window_is_front = direction_ == SLIDE_BACK;
  AnimateTranslation(GetBackLayer(), 0, !main_window_is_front);
  AnimateTranslation(GetFrontLayer(), 0, main_window_is_front);
}

float OverscrollWindowAnimation::GetTranslationForOverscroll(float delta_x) {
  DCHECK(direction_ != SLIDE_NONE);
  const float bounds_width = GetContentSize().width();
  if (direction_ == SLIDE_FRONT)
    return std::max(-bounds_width, delta_x);
  else
    return std::min(bounds_width, delta_x);
}

gfx::Size OverscrollWindowAnimation::GetDisplaySize() const {
  return display::Screen::GetScreen()
      ->GetDisplayNearestView(delegate_->GetMainWindow())
      .size();
}

bool OverscrollWindowAnimation::OnOverscrollUpdate(float delta_x,
                                                   float delta_y) {
  if (direction_ == SLIDE_NONE)
    return false;
  gfx::Transform front_transform;
  gfx::Transform back_transform;
  float translate_x = GetTranslationForOverscroll(delta_x);
  front_transform.Translate(translate_x, 0);
  back_transform.Translate(translate_x / 2, 0);
  GetFrontLayer()->SetTransform(front_transform);
  GetBackLayer()->SetTransform(back_transform);
  return true;
}

void OverscrollWindowAnimation::OnImplicitAnimationsCompleted() {
  if (overscroll_cancelled_) {
    slide_window_.reset();
    delegate_->OnOverscrollCancelled();
    overscroll_cancelled_ = false;
  } else {
    delegate_->OnOverscrollCompleted(std::move(slide_window_));
  }
  overscroll_source_ = OverscrollSource::NONE;
  direction_ = SLIDE_NONE;
}

void OverscrollWindowAnimation::OnOverscrollModeChange(
    OverscrollMode old_mode,
    OverscrollMode new_mode,
    OverscrollSource source,
    cc::OverscrollBehavior behavior) {
  DCHECK_NE(old_mode, new_mode);
  Direction new_direction = GetDirectionForMode(new_mode);
  if (new_direction == SLIDE_NONE ||
      behavior.x != cc::OverscrollBehavior::OverscrollBehaviorType::
                        kOverscrollBehaviorTypeAuto) {
    // The user cancelled the in progress animation.
    if (is_active())
      CancelSlide();
    return;
  }
  if (is_active()) {
    slide_window_->layer()->GetAnimator()->StopAnimating();
    delegate_->GetMainWindow()->layer()->GetAnimator()->StopAnimating();
  }
  gfx::Rect slide_window_bounds(GetContentSize());
  if (new_direction == SLIDE_FRONT) {
    slide_window_bounds.Offset(base::i18n::IsRTL()
                                   ? -slide_window_bounds.width()
                                   : slide_window_bounds.width(),
                               0);
  } else {
    slide_window_bounds.Offset(base::i18n::IsRTL()
                                   ? slide_window_bounds.width() / 2
                                   : -slide_window_bounds.width() / 2,
                               0);
  }

  overscroll_source_ = source;
  slide_window_ = new_direction == SLIDE_FRONT
                      ? delegate_->CreateFrontWindow(slide_window_bounds)
                      : delegate_->CreateBackWindow(slide_window_bounds);
  if (!slide_window_) {
    // Cannot navigate, do not start an overscroll gesture.
    direction_ = SLIDE_NONE;
    overscroll_source_ = OverscrollSource::NONE;
    return;
  }
  overscroll_cancelled_ = false;
  direction_ = new_direction;
  shadow_.reset(new ShadowLayerDelegate(GetFrontLayer()));
}

base::Optional<float> OverscrollWindowAnimation::GetMaxOverscrollDelta() const {
  return base::nullopt;
}

void OverscrollWindowAnimation::OnOverscrollComplete(
    OverscrollMode overscroll_mode) {
  if (!is_active())
    return;
  delegate_->OnOverscrollCompleting();
  int content_width = GetContentSize().width();
  float translate_x;
  if ((base::i18n::IsRTL() && direction_ == SLIDE_FRONT) ||
      (!base::i18n::IsRTL() && direction_ == SLIDE_BACK)) {
    translate_x = content_width;
  } else {
    translate_x = -content_width;
  }
  // Listen to the animation of the main window.
  bool main_window_is_front = direction_ == SLIDE_BACK;
  AnimateTranslation(GetBackLayer(), translate_x / 2, !main_window_is_front);
  AnimateTranslation(GetFrontLayer(), translate_x, main_window_is_front);
}

void OverscrollWindowAnimation::AnimateTranslation(ui::Layer* layer,
                                                   float translate_x,
                                                   bool listen_for_completion) {
  gfx::Transform transform;
  transform.Translate(translate_x, 0);
  ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
  settings.SetPreemptionStrategy(
      ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
  settings.SetTweenType(gfx::Tween::EASE_OUT);
  if (listen_for_completion)
    settings.AddObserver(this);
  layer->SetTransform(transform);
}

ui::Layer* OverscrollWindowAnimation::GetFrontLayer() const {
  DCHECK(direction_ != SLIDE_NONE);
  if (direction_ == SLIDE_FRONT) {
    DCHECK(slide_window_);
    return slide_window_->layer();
  }
  return delegate_->GetMainWindow()->layer();
}

ui::Layer* OverscrollWindowAnimation::GetBackLayer() const {
  DCHECK(direction_ != SLIDE_NONE);
  if (direction_ == SLIDE_BACK) {
    DCHECK(slide_window_);
    return slide_window_->layer();
  }
  return delegate_->GetMainWindow()->layer();
}

gfx::Size OverscrollWindowAnimation::GetContentSize() const {
  return delegate_->GetMainWindow()->bounds().size();
}

}  // namespace content
