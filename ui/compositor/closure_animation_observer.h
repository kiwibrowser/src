// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_CLOSURE_ANIMATION_OBSERVER_H_
#define UI_COMPOSITOR_CLOSURE_ANIMATION_OBSERVER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "ui/compositor/compositor_export.h"
#include "ui/compositor/layer_animation_observer.h"

namespace ui {

// Runs a callback at the end of the animation. This observe also destroys
// itself afterwards.
class COMPOSITOR_EXPORT ClosureAnimationObserver
    : public ImplicitAnimationObserver {
 public:
  explicit ClosureAnimationObserver(const base::Closure& closure);

 private:
  ~ClosureAnimationObserver() override;

  // ImplicitAnimationObserver:
  void OnImplicitAnimationsCompleted() override;

  const base::Closure closure_;

  DISALLOW_COPY_AND_ASSIGN(ClosureAnimationObserver);
};

}  // namespace ui

#endif  // UI_COMPOSITOR_CLOSURE_ANIMATION_OBSERVER_H_
