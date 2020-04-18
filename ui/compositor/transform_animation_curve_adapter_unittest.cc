// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/transform_animation_curve_adapter.h"

#include <sstream>

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/compositor/test/test_utils.h"

namespace ui {

namespace {

// Check that the inverse transform curve gives the gives a transform that when
// applied on top of the parent transform gives the original transform
TEST(InverseTransformCurveAdapterTest, InversesTransform) {
  gfx::Transform parent_start, parent_target;
  parent_start.Scale(0.5, 3.0);
  parent_start.Translate(-20.0, 30.0);
  parent_target.Translate(0, 100);

  gfx::Transform child_transform;
  child_transform.Rotate(-30.0);

  base::TimeDelta duration = base::TimeDelta::FromSeconds(1);

  const gfx::Transform effective_child_transform =
      parent_start * child_transform;

  TransformAnimationCurveAdapter parent_curve(gfx::Tween::LINEAR, parent_start,
                                              parent_target, duration);

  InverseTransformCurveAdapter child_curve(parent_curve, child_transform,
                                           duration);
  static const int kSteps = 1000;
  double step = 1.0 / kSteps;
  for (int i = 0; i <= kSteps; ++i) {
    base::TimeDelta time_step = duration * (i * step);
    std::ostringstream message;
    message << "Step " << i << " of " << kSteps;
    SCOPED_TRACE(message.str());
    gfx::Transform progress_parent_transform =
        parent_curve.GetValue(time_step).Apply();
    gfx::Transform progress_child_transform =
        child_curve.GetValue(time_step).Apply();
    CheckApproximatelyEqual(
        effective_child_transform,
        progress_parent_transform * progress_child_transform);
  }
}

}  // namespace

}  // namespace ui
