// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/frame_test_util.h"

#include <stdint.h>

#include <cmath>

#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/transform.h"

namespace content {

// static
FrameTestUtil::RGB FrameTestUtil::ComputeAverageColor(
    SkBitmap frame,
    const gfx::Rect& raw_include_rect,
    const gfx::Rect& raw_exclude_rect) {
  // Clip the rects to the valid region within |frame|. Also, only the subregion
  // of |exclude_rect| within |include_rect| is relevant.
  gfx::Rect include_rect = raw_include_rect;
  include_rect.Intersect(gfx::Rect(0, 0, frame.width(), frame.height()));
  gfx::Rect exclude_rect = raw_exclude_rect;
  exclude_rect.Intersect(include_rect);

  // Sum up the color values in each color channel for all pixels in
  // |include_rect| not contained by |exclude_rect|.
  int64_t include_sums[3] = {0};
  for (int y = include_rect.y(), bottom = include_rect.bottom(); y < bottom;
       ++y) {
    for (int x = include_rect.x(), right = include_rect.right(); x < right;
         ++x) {
      const SkColor color = frame.getColor(x, y);
      if (exclude_rect.Contains(x, y)) {
        continue;
      }
      include_sums[0] += SkColorGetR(color);
      include_sums[1] += SkColorGetG(color);
      include_sums[2] += SkColorGetB(color);
    }
  }

  // Divide the sums by the area to compute the average color.
  const int include_area =
      include_rect.size().GetArea() - exclude_rect.size().GetArea();
  if (include_area <= 0) {
    return RGB{NAN, NAN, NAN};
  } else {
    const auto include_area_f = static_cast<double>(include_area);
    return RGB{include_sums[0] / include_area_f,
               include_sums[1] / include_area_f,
               include_sums[2] / include_area_f};
  }
}

// static
bool FrameTestUtil::IsApproximatelySameColor(SkColor color,
                                             const RGB& rgb,
                                             int max_diff) {
  const double r_diff = std::abs(SkColorGetR(color) - rgb.r);
  const double g_diff = std::abs(SkColorGetG(color) - rgb.g);
  const double b_diff = std::abs(SkColorGetB(color) - rgb.b);
  return r_diff < max_diff && g_diff < max_diff && b_diff < max_diff;
}

// static
gfx::RectF FrameTestUtil::TransformSimilarly(const gfx::Rect& original,
                                             const gfx::RectF& transformed,
                                             const gfx::Rect& rect) {
  if (original.IsEmpty()) {
    return gfx::RectF(transformed.x() - original.x(),
                      transformed.y() - original.y(), 0.0f, 0.0f);
  }
  // The following is the scale-then-translate 2D matrix.
  const gfx::Transform transform(transformed.width() / original.width(), 0.0f,
                                 0.0f, transformed.height() / original.height(),
                                 transformed.x() - original.x(),
                                 transformed.y() - original.y());
  gfx::RectF result(rect);
  transform.TransformRect(&result);
  return result;
}

std::ostream& operator<<(std::ostream& out, const FrameTestUtil::RGB& rgb) {
  return (out << "{r=" << rgb.r << ",g=" << rgb.g << ",b=" << rgb.b << '}');
}

}  // namespace content
