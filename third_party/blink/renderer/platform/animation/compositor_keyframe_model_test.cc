// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/animation/compositor_keyframe_model.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/animation/compositor_float_animation_curve.h"

namespace blink {

TEST(WebCompositorAnimationTest, DefaultSettings) {
  std::unique_ptr<CompositorAnimationCurve> curve =
      CompositorFloatAnimationCurve::Create();
  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      CompositorKeyframeModel::Create(*curve, CompositorTargetProperty::OPACITY,
                                      1, 0);

  // Ensure that the defaults are correct.
  EXPECT_EQ(1, keyframe_model->Iterations());
  EXPECT_EQ(0, keyframe_model->StartTime());
  EXPECT_EQ(0, keyframe_model->TimeOffset());
  EXPECT_EQ(CompositorKeyframeModel::Direction::NORMAL,
            keyframe_model->GetDirection());
}

TEST(WebCompositorAnimationTest, ModifiedSettings) {
  std::unique_ptr<CompositorFloatAnimationCurve> curve =
      CompositorFloatAnimationCurve::Create();
  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      CompositorKeyframeModel::Create(*curve, CompositorTargetProperty::OPACITY,
                                      1, 0);
  keyframe_model->SetIterations(2);
  keyframe_model->SetStartTime(2);
  keyframe_model->SetTimeOffset(2);
  keyframe_model->SetDirection(CompositorKeyframeModel::Direction::REVERSE);

  EXPECT_EQ(2, keyframe_model->Iterations());
  EXPECT_EQ(2, keyframe_model->StartTime());
  EXPECT_EQ(2, keyframe_model->TimeOffset());
  EXPECT_EQ(CompositorKeyframeModel::Direction::REVERSE,
            keyframe_model->GetDirection());
}

}  // namespace blink
