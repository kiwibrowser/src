// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_VIEWS_SLIDE_OUT_CONTROLLER_H_
#define UI_MESSAGE_CENTER_VIEWS_SLIDE_OUT_CONTROLLER_H_

#include "base/macros.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/events/scoped_target_handler.h"
#include "ui/message_center/message_center_export.h"
#include "ui/views/view.h"

namespace message_center {

// This class contains logic to control sliding out of a layer in response to
// swipes, i.e. gesture scroll events.
class MESSAGE_CENTER_EXPORT SlideOutController
    : public ui::EventHandler,
      public ui::ImplicitAnimationObserver {
 public:
  class Delegate {
   public:
    // Returns the layer for slide operations.
    virtual ui::Layer* GetSlideOutLayer() = 0;

    // Called when a slide starts, ends, or is updated.
    virtual void OnSlideChanged() = 0;

    // Called when user intends to close the View by sliding it out.
    virtual void OnSlideOut() = 0;
  };

  SlideOutController(ui::EventTarget* target, Delegate* delegate);
  ~SlideOutController() override;

  void set_enabled(bool enabled) { enabled_ = enabled; }
  bool enabled() { return enabled_; }

  // ui::EventHandler
  void OnGestureEvent(ui::GestureEvent* event) override;

  // ui::ImplicitAnimationObserver
  void OnImplicitAnimationsCompleted() override;

 private:
  // Restores the transform and opacity of the view.
  void RestoreVisualState();

  // Slides the view out and closes it after the animation. The sign of
  // |direction| indicates which way the slide occurs.
  void SlideOutAndClose(int direction);

  ui::ScopedTargetHandler target_handling_;
  Delegate* delegate_;

  float gesture_amount_ = 0.f;
  bool enabled_ = true;

  DISALLOW_COPY_AND_ASSIGN(SlideOutController);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_VIEWS_SLIDE_OUT_CONTROLLER_H_
