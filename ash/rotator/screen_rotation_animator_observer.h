// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ROTATOR_SCREEN_ROTATION_ANIMATOR_OBSERVER_H
#define ASH_ROTATOR_SCREEN_ROTATION_ANIMATOR_OBSERVER_H

#include "ash/ash_export.h"

namespace ash {

class ScreenRotationAnimator;

class ASH_EXPORT ScreenRotationAnimatorObserver {
 public:
  ScreenRotationAnimatorObserver() {}

  // This will be called when the animation is ended or aborted.
  virtual void OnScreenRotationAnimationFinished(
      ScreenRotationAnimator* animator) = 0;

 protected:
  virtual ~ScreenRotationAnimatorObserver() {}
};

}  // namespace ash

#endif  // ASH_ROTATOR_SCREEN_ROTATION_ANIMATOR_OBSERVER_H
