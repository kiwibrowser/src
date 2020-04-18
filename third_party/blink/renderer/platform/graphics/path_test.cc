// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/path.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(PathTest, PointAtEndOfPath) {
  Path path;
  path.MoveTo(FloatPoint(70, -48));
  path.AddBezierCurveTo(FloatPoint(70, -48), FloatPoint(136, 136),
                        FloatPoint(230, 166));
  path.MoveTo(FloatPoint(230, 166));
  path.AddBezierCurveTo(FloatPoint(324, 196), FloatPoint(472, 370),
                        FloatPoint(460, 470));

  FloatPoint point;
  float angle;
  path.PointAndNormalAtLength(path.length(), point, angle);
  EXPECT_EQ(point, FloatPoint(460, 470));
}

}  // namespace blink
