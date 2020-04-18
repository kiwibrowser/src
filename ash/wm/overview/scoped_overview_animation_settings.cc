// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/scoped_overview_animation_settings.h"

#include "base/lazy_instance.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/animation/tween.h"

namespace ash {

namespace {

// The time duration for transformation animations.
constexpr int kTransitionMilliseconds = 300;

// The time duration for fading out when closing an item.
constexpr int kCloseFadeOutMilliseconds = 50;

// The time duration for scaling down when an item is closed.
constexpr int kCloseScaleMilliseconds = 100;

// The time duration for widgets to fade in.
constexpr int kFadeInMilliseconds = 60;

// The time duration for widgets to fade in in tablet mode.
constexpr int kFadeInTabletMs = 300;

// The time duration for widgets to fade out.
constexpr int kFadeOutDelayMilliseconds = kTransitionMilliseconds * 1 / 5;
constexpr int kFadeOutMilliseconds = kTransitionMilliseconds * 3 / 5;

base::TimeDelta GetAnimationDuration(OverviewAnimationType animation_type) {
  switch (animation_type) {
    case OVERVIEW_ANIMATION_NONE:
      return base::TimeDelta();
    case OVERVIEW_ANIMATION_ENTER_OVERVIEW_MODE_FADE_IN:
      return base::TimeDelta::FromMilliseconds(kFadeInMilliseconds);
    case OVERVIEW_ANIMATION_ENTER_OVERVIEW_MODE_TABLET_FADE_IN:
      return base::TimeDelta::FromMilliseconds(kFadeInTabletMs);
    case OVERVIEW_ANIMATION_EXIT_OVERVIEW_MODE_FADE_OUT:
      return base::TimeDelta::FromMilliseconds(kFadeOutMilliseconds);
    case OVERVIEW_ANIMATION_LAY_OUT_SELECTOR_ITEMS:
    case OVERVIEW_ANIMATION_RESTORE_WINDOW:
      return base::TimeDelta::FromMilliseconds(kTransitionMilliseconds);
    case OVERVIEW_ANIMATION_CLOSING_SELECTOR_ITEM:
      return base::TimeDelta::FromMilliseconds(kCloseScaleMilliseconds);
    case OVERVIEW_ANIMATION_CLOSE_SELECTOR_ITEM:
      return base::TimeDelta::FromMilliseconds(kCloseFadeOutMilliseconds);
  }
  NOTREACHED();
  return base::TimeDelta();
}

class OverviewEnterMetricsReporter : public ui::AnimationMetricsReporter {
 public:
  OverviewEnterMetricsReporter() = default;
  ~OverviewEnterMetricsReporter() override = default;

  void Report(int value) override {
    UMA_HISTOGRAM_PERCENTAGE("Ash.WindowSelector.AnimationSmoothness.Enter",
                             value);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(OverviewEnterMetricsReporter);
};

class OverviewExitMetricsReporter : public ui::AnimationMetricsReporter {
 public:
  OverviewExitMetricsReporter() = default;
  ~OverviewExitMetricsReporter() override = default;

  void Report(int value) override {
    UMA_HISTOGRAM_PERCENTAGE("Ash.WindowSelector.AnimationSmoothness.Exit",
                             value);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(OverviewExitMetricsReporter);
};

class OverviewCloseMetricsReporter : public ui::AnimationMetricsReporter {
 public:
  OverviewCloseMetricsReporter() = default;
  ~OverviewCloseMetricsReporter() override = default;

  void Report(int value) override {
    UMA_HISTOGRAM_PERCENTAGE("Ash.WindowSelector.AnimationSmoothness.Close",
                             value);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(OverviewCloseMetricsReporter);
};

base::LazyInstance<OverviewEnterMetricsReporter>::Leaky g_reporter_enter =
    LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<OverviewExitMetricsReporter>::Leaky g_reporter_exit =
    LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<OverviewCloseMetricsReporter>::Leaky g_reporter_close =
    LAZY_INSTANCE_INITIALIZER;

ui::AnimationMetricsReporter* GetMetricsReporter(
    OverviewAnimationType animation_type) {
  switch (animation_type) {
    case OVERVIEW_ANIMATION_NONE:
      return nullptr;
    case OVERVIEW_ANIMATION_ENTER_OVERVIEW_MODE_FADE_IN:
    case OVERVIEW_ANIMATION_ENTER_OVERVIEW_MODE_TABLET_FADE_IN:
    case OVERVIEW_ANIMATION_LAY_OUT_SELECTOR_ITEMS:
      return g_reporter_enter.Pointer();
    case OVERVIEW_ANIMATION_EXIT_OVERVIEW_MODE_FADE_OUT:
    case OVERVIEW_ANIMATION_RESTORE_WINDOW:
      return g_reporter_exit.Pointer();
    case OVERVIEW_ANIMATION_CLOSING_SELECTOR_ITEM:
    case OVERVIEW_ANIMATION_CLOSE_SELECTOR_ITEM:
      return g_reporter_close.Pointer();
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace

ScopedOverviewAnimationSettings::ScopedOverviewAnimationSettings(
    OverviewAnimationType animation_type,
    aura::Window* window)
    : animation_settings_(new ui::ScopedLayerAnimationSettings(
          window->layer()->GetAnimator())) {
  switch (animation_type) {
    case OVERVIEW_ANIMATION_NONE:
      animation_settings_->SetPreemptionStrategy(
          ui::LayerAnimator::REPLACE_QUEUED_ANIMATIONS);
      break;
    case OVERVIEW_ANIMATION_ENTER_OVERVIEW_MODE_FADE_IN:
    case OVERVIEW_ANIMATION_ENTER_OVERVIEW_MODE_TABLET_FADE_IN:
      animation_settings_->SetTweenType(gfx::Tween::EASE_IN);
      animation_settings_->SetPreemptionStrategy(
          ui::LayerAnimator::REPLACE_QUEUED_ANIMATIONS);
      break;
    case OVERVIEW_ANIMATION_EXIT_OVERVIEW_MODE_FADE_OUT:
      window->layer()->GetAnimator()->SchedulePauseForProperties(
          base::TimeDelta::FromMilliseconds(kFadeOutDelayMilliseconds),
          ui::LayerAnimationElement::OPACITY);
      animation_settings_->SetTweenType(gfx::Tween::EASE_OUT);
      animation_settings_->SetPreemptionStrategy(
          ui::LayerAnimator::REPLACE_QUEUED_ANIMATIONS);
      break;
    case OVERVIEW_ANIMATION_LAY_OUT_SELECTOR_ITEMS:
    case OVERVIEW_ANIMATION_RESTORE_WINDOW:
      animation_settings_->SetPreemptionStrategy(
          ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
      animation_settings_->SetTweenType(gfx::Tween::EASE_OUT);
      break;
    case OVERVIEW_ANIMATION_CLOSING_SELECTOR_ITEM:
    case OVERVIEW_ANIMATION_CLOSE_SELECTOR_ITEM:
      animation_settings_->SetPreemptionStrategy(
          ui::LayerAnimator::ENQUEUE_NEW_ANIMATION);
      animation_settings_->SetTweenType(gfx::Tween::EASE_OUT);
      break;
  }
  animation_settings_->SetTransitionDuration(
      GetAnimationDuration(animation_type));
  animation_settings_->SetAnimationMetricsReporter(
      GetMetricsReporter(animation_type));
}

ScopedOverviewAnimationSettings::~ScopedOverviewAnimationSettings() = default;

void ScopedOverviewAnimationSettings::AddObserver(
    ui::ImplicitAnimationObserver* observer) {
  animation_settings_->AddObserver(observer);
}

void ScopedOverviewAnimationSettings::CacheRenderSurface() {
  animation_settings_->CacheRenderSurface();
}

void ScopedOverviewAnimationSettings::DeferPaint() {
  animation_settings_->DeferPaint();
}

void ScopedOverviewAnimationSettings::TrilinearFiltering() {
  animation_settings_->TrilinearFiltering();
}

}  // namespace ash
