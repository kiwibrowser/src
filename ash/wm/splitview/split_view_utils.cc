// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/splitview/split_view_utils.h"

#include "ash/wm/splitview/split_view_constants.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/compositor/layer_animator.h"
#include "ui/compositor/scoped_layer_animation_settings.h"

namespace ash {

namespace {

// The animation speed at which the highlights fade in or out.
constexpr base::TimeDelta kHighlightsFadeInOutMs =
    base::TimeDelta::FromMilliseconds(250);
// The animation speed which the other highlight fades in or out.
constexpr base::TimeDelta kOtherFadeInOutMs =
    base::TimeDelta::FromMilliseconds(133);
// The delay before the other highlight starts fading in or out.
constexpr base::TimeDelta kOtherFadeOutDelayMs =
    base::TimeDelta::FromMilliseconds(117);
// The animation speed for any animation on the indicator labels.
constexpr base::TimeDelta kLabelAnimationMs =
    base::TimeDelta::FromMilliseconds(83);
// The delay before the indicator labels start animating.
constexpr base::TimeDelta kLabelAnimationDelayMs =
    base::TimeDelta::FromMilliseconds(167);
// The time duration for the window transformation animations.
constexpr base::TimeDelta kWindowTransformMs =
    base::TimeDelta::FromMilliseconds(300);

constexpr float kHighlightOpacity = 0.3f;
constexpr float kPreviewAreaHighlightOpacity = 0.18f;

// Gets the duration, tween type and delay before animation based on |type|.
void GetAnimationValuesForType(
    SplitviewAnimationType type,
    base::TimeDelta* out_duration,
    gfx::Tween::Type* out_tween_type,
    ui::LayerAnimator::PreemptionStrategy* out_preemption_strategy,
    base::TimeDelta* out_delay) {
  *out_preemption_strategy = ui::LayerAnimator::IMMEDIATELY_SET_NEW_TARGET;
  switch (type) {
    case SPLITVIEW_ANIMATION_HIGHLIGHT_FADE_IN:
    case SPLITVIEW_ANIMATION_HIGHLIGHT_FADE_OUT:
    case SPLITVIEW_ANIMATION_PREVIEW_AREA_FADE_IN:
    case SPLITVIEW_ANIMATION_PREVIEW_AREA_FADE_OUT:
    case SPLITVIEW_ANIMATION_SELECTOR_ITEM_FADE_IN:
    case SPLITVIEW_ANIMATION_SELECTOR_ITEM_FADE_OUT:
    case SPLITVIEW_ANIMATION_TEXT_FADE_IN_WITH_HIGHLIGHT:
    case SPLITVIEW_ANIMATION_TEXT_FADE_OUT_WITH_HIGHLIGHT:
    case SPLITVIEW_ANIMATION_PREVIEW_AREA_SLIDE_IN_OUT:
      *out_duration = kHighlightsFadeInOutMs;
      *out_tween_type = gfx::Tween::FAST_OUT_SLOW_IN;
      return;
    case SPLITVIEW_ANIMATION_OTHER_HIGHLIGHT_FADE_IN:
    case SPLITVIEW_ANIMATION_OTHER_HIGHLIGHT_SLIDE_IN:
      *out_duration = kOtherFadeInOutMs;
      *out_tween_type = gfx::Tween::LINEAR_OUT_SLOW_IN;
      return;
    case SPLITVIEW_ANIMATION_TEXT_FADE_IN:
    case SPLITVIEW_ANIMATION_TEXT_FADE_OUT:
    case SPLITVIEW_ANIMATION_TEXT_SLIDE_IN:
    case SPLITVIEW_ANIMATION_TEXT_SLIDE_OUT:
      if (type == SPLITVIEW_ANIMATION_TEXT_SLIDE_IN)
        *out_delay = kLabelAnimationDelayMs;
      *out_duration = kLabelAnimationMs;
      *out_tween_type = gfx::Tween::LINEAR_OUT_SLOW_IN;
      return;
    case SPLITVIEW_ANIMATION_OTHER_HIGHLIGHT_FADE_OUT:
    case SPLITVIEW_ANIMATION_OTHER_HIGHLIGHT_SLIDE_OUT:
      *out_delay = kOtherFadeOutDelayMs;
      *out_duration = kOtherFadeInOutMs;
      *out_tween_type = gfx::Tween::LINEAR_OUT_SLOW_IN;
      return;
    case SPLITVIEW_ANIMATION_RESTORE_OVERVIEW_WINDOW:
      *out_duration = kWindowTransformMs;
      *out_tween_type = gfx::Tween::EASE_OUT;
      *out_preemption_strategy =
          ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET;
      return;
  }

  NOTREACHED();
}

// Helper function to apply animation values to |settings|.
void ApplyAnimationSettings(
    ui::ScopedLayerAnimationSettings* settings,
    ui::LayerAnimator* animator,
    base::TimeDelta duration,
    gfx::Tween::Type tween,
    ui::LayerAnimator::PreemptionStrategy preemption_strategy,
    base::TimeDelta delay) {
  DCHECK_EQ(settings->GetAnimator(), animator);
  settings->SetTransitionDuration(duration);
  settings->SetTweenType(tween);
  settings->SetPreemptionStrategy(preemption_strategy);
  if (!delay.is_zero()) {
    settings->SetPreemptionStrategy(
        ui::LayerAnimator::REPLACE_QUEUED_ANIMATIONS);
    animator->SchedulePauseForProperties(
        delay, ui::LayerAnimationElement::OPACITY |
                   ui::LayerAnimationElement::TRANSFORM);
  }
}

}  // namespace

void DoSplitviewOpacityAnimation(ui::Layer* layer,
                                 SplitviewAnimationType type) {
  float target_opacity = 0.f;
  switch (type) {
    case SPLITVIEW_ANIMATION_HIGHLIGHT_FADE_OUT:
    case SPLITVIEW_ANIMATION_OTHER_HIGHLIGHT_FADE_OUT:
    case SPLITVIEW_ANIMATION_SELECTOR_ITEM_FADE_OUT:
    case SPLITVIEW_ANIMATION_TEXT_FADE_OUT:
    case SPLITVIEW_ANIMATION_TEXT_FADE_OUT_WITH_HIGHLIGHT:
      target_opacity = 0.f;
      break;
    case SPLITVIEW_ANIMATION_HIGHLIGHT_FADE_IN:
    case SPLITVIEW_ANIMATION_OTHER_HIGHLIGHT_FADE_IN:
    case SPLITVIEW_ANIMATION_PREVIEW_AREA_FADE_IN:
      target_opacity = kPreviewAreaHighlightOpacity;
      break;
    case SPLITVIEW_ANIMATION_PREVIEW_AREA_FADE_OUT:
      target_opacity = kHighlightOpacity;
      break;
    case SPLITVIEW_ANIMATION_SELECTOR_ITEM_FADE_IN:
    case SPLITVIEW_ANIMATION_TEXT_FADE_IN:
    case SPLITVIEW_ANIMATION_TEXT_FADE_IN_WITH_HIGHLIGHT:
      target_opacity = 1.f;
      break;
    default:
      NOTREACHED() << "Not a valid split view opacity animation type.";
      return;
  }

  if (layer->GetTargetOpacity() == target_opacity)
    return;

  base::TimeDelta duration;
  gfx::Tween::Type tween;
  ui::LayerAnimator::PreemptionStrategy preemption_strategy;
  base::TimeDelta delay;
  GetAnimationValuesForType(type, &duration, &tween, &preemption_strategy,
                            &delay);

  ui::LayerAnimator* animator = layer->GetAnimator();
  ui::ScopedLayerAnimationSettings settings(animator);
  ApplyAnimationSettings(&settings, animator, duration, tween,
                         preemption_strategy, delay);
  layer->SetOpacity(target_opacity);
}

void DoSplitviewTransformAnimation(ui::Layer* layer,
                                   SplitviewAnimationType type,
                                   const gfx::Transform& target_transform,
                                   ui::ImplicitAnimationObserver* observer) {
  if (layer->GetTargetTransform() == target_transform)
    return;

  switch (type) {
    case SPLITVIEW_ANIMATION_OTHER_HIGHLIGHT_SLIDE_IN:
    case SPLITVIEW_ANIMATION_OTHER_HIGHLIGHT_SLIDE_OUT:
    case SPLITVIEW_ANIMATION_PREVIEW_AREA_SLIDE_IN_OUT:
    case SPLITVIEW_ANIMATION_RESTORE_OVERVIEW_WINDOW:
    case SPLITVIEW_ANIMATION_TEXT_SLIDE_IN:
    case SPLITVIEW_ANIMATION_TEXT_SLIDE_OUT:
      break;
    default:
      NOTREACHED() << "Not a valid split view transform type.";
      return;
  }

  base::TimeDelta duration;
  gfx::Tween::Type tween;
  ui::LayerAnimator::PreemptionStrategy preemption_strategy;
  base::TimeDelta delay;
  GetAnimationValuesForType(type, &duration, &tween, &preemption_strategy,
                            &delay);

  ui::LayerAnimator* animator = layer->GetAnimator();
  ui::ScopedLayerAnimationSettings settings(animator);
  ApplyAnimationSettings(&settings, animator, duration, tween,
                         preemption_strategy, delay);
  if (observer)
    settings.AddObserver(observer);
  layer->SetTransform(target_transform);
}

}  // namespace ash
