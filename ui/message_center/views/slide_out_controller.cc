// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/slide_out_controller.h"

#include "ui/compositor/layer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/transform.h"

namespace message_center {

SlideOutController::SlideOutController(ui::EventTarget* target,
                                       Delegate* delegate)
    : target_handling_(target, this), delegate_(delegate) {}

SlideOutController::~SlideOutController() {}

void SlideOutController::OnGestureEvent(ui::GestureEvent* event) {
  const float kScrollRatioForClosingNotification = 0.5f;

  if (event->type() == ui::ET_SCROLL_FLING_START) {
    // The threshold for the fling velocity is computed empirically.
    // The unit is in pixels/second.
    const float kFlingThresholdForClose = 800.f;
    if (enabled_ &&
        fabsf(event->details().velocity_x()) > kFlingThresholdForClose) {
      SlideOutAndClose(event->details().velocity_x());
      event->StopPropagation();
      return;
    }
    RestoreVisualState();
    return;
  }

  if (!event->IsScrollGestureEvent())
    return;

  ui::Layer* layer = delegate_->GetSlideOutLayer();
  int width = layer->bounds().width();
  if (event->type() == ui::ET_GESTURE_SCROLL_BEGIN) {
    gesture_amount_ = 0.f;
  } else if (event->type() == ui::ET_GESTURE_SCROLL_UPDATE) {
    // The scroll-update events include the incremental scroll amount.
    gesture_amount_ += event->details().scroll_x();

    float scroll_amount;
    if (enabled_) {
      scroll_amount = gesture_amount_;
      layer->SetOpacity(1.f - std::min(fabsf(scroll_amount) / width, 1.f));
    } else {
      if (gesture_amount_ >= 0) {
        scroll_amount = std::min(0.5f * gesture_amount_,
                                 width * kScrollRatioForClosingNotification);
      } else {
        scroll_amount =
            std::max(0.5f * gesture_amount_,
                     -1.f * width * kScrollRatioForClosingNotification);
      }
    }

    gfx::Transform transform;
    transform.Translate(scroll_amount, 0.0);
    layer->SetTransform(transform);
  } else if (event->type() == ui::ET_GESTURE_SCROLL_END) {
    float scrolled_ratio = fabsf(gesture_amount_) / width;
    if (enabled_ && scrolled_ratio >= kScrollRatioForClosingNotification) {
      SlideOutAndClose(gesture_amount_);
      event->StopPropagation();
      return;
    }
    RestoreVisualState();
  }

  delegate_->OnSlideChanged();
  event->SetHandled();
}

void SlideOutController::RestoreVisualState() {
  ui::Layer* layer = delegate_->GetSlideOutLayer();
  // Restore the layer state.
  const int kSwipeRestoreDurationMS = 150;
  ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
  settings.SetTransitionDuration(
      base::TimeDelta::FromMilliseconds(kSwipeRestoreDurationMS));
  layer->SetTransform(gfx::Transform());
  layer->SetOpacity(1.f);
}

void SlideOutController::SlideOutAndClose(int direction) {
  ui::Layer* layer = delegate_->GetSlideOutLayer();
  const int kSwipeOutTotalDurationMS = 150;
  int swipe_out_duration = kSwipeOutTotalDurationMS * layer->opacity();
  ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
  settings.SetTransitionDuration(
      base::TimeDelta::FromMilliseconds(swipe_out_duration));
  settings.AddObserver(this);

  gfx::Transform transform;
  int width = layer->bounds().width();
  transform.Translate(direction < 0 ? -width : width, 0.0);
  layer->SetTransform(transform);
  layer->SetOpacity(0.f);
  delegate_->OnSlideChanged();
}

void SlideOutController::OnImplicitAnimationsCompleted() {
  delegate_->OnSlideOut();
}

}  // namespace message_center
