// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_ANIMATED_ICON_VIEW_H_
#define UI_VIEWS_CONTROLS_ANIMATED_ICON_VIEW_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "ui/compositor/compositor_animation_observer.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/controls/image_view.h"

namespace views {

// This class hosts a vector icon that defines transitions. It can be in the
// start steady state, the end steady state, or transitioning in between.
class VIEWS_EXPORT AnimatedIconView : public views::ImageView,
                                      public ui::CompositorAnimationObserver {
 public:
  enum State {
    START,
    END,
  };

  explicit AnimatedIconView(const gfx::VectorIcon& icon);
  ~AnimatedIconView() override;

  void SetColor(SkColor color);

  // Animates to the end or start state.
  void Animate(State target);

  // Jumps to the end or start state.
  void SetState(State state);

  bool IsAnimating() const;

  // views::ImageView
  void OnPaint(gfx::Canvas* canvas) override;

  // ui::CompositorAnimationObserver
  void OnAnimationStep(base::TimeTicks timestamp) override;
  void OnCompositingShuttingDown(ui::Compositor* compositor) override;

 private:
  void UpdateStaticImage();

  const gfx::VectorIcon& icon_;
  SkColor color_;

  // Tracks the last time Animate() was called.
  base::TimeTicks start_time_;

  // The amount of time that must elapse until all transitions are done, i.e.
  // the length of the animation.
  const base::TimeDelta duration_;

  // The current state, or when transitioning the goal state.
  State state_ = START;

  // The compositor that |this| is observing, if and when there is an active
  // animation. Otherwise it is null.
  ui::Compositor* compositor_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AnimatedIconView);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_ANIMATED_ICON_VIEW_H_
