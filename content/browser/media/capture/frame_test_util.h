// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_FRAME_TEST_UTIL_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_FRAME_TEST_UTIL_H_

#include <ostream>

#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"

namespace gfx {
class Rect;
class RectF;
}  // namespace gfx

namespace content {

class FrameTestUtil {
 public:
  struct RGB {
    double r;
    double g;
    double b;
  };

  // Returns the average RGB color in |include_rect| except for pixels also in
  // |exclude_rect|.
  static RGB ComputeAverageColor(SkBitmap frame,
                                 const gfx::Rect& include_rect,
                                 const gfx::Rect& exclude_rect);

  // Returns true if the red, green, and blue components are all within
  // |max_diff| of each other.
  static bool IsApproximatelySameColor(SkColor color,
                                       const RGB& rgb,
                                       int max_diff = kMaxColorDifference);

  // Determines how |original| has been scaled and translated to become
  // |transformed|, and then applies the same transform on |rect| and returns
  // the result.
  static gfx::RectF TransformSimilarly(const gfx::Rect& original,
                                       const gfx::RectF& transformed,
                                       const gfx::Rect& rect);

  // The default maximum color value difference, assuming there will be a little
  // error due to pixel boundaries being rounded after coordinate system
  // transforms.
  static constexpr int kMaxColorDifference = 16;

  // A much more-relaxed maximum color value difference, assuming errors caused
  // by indifference towards color space concerns (and also "studio" versus
  // "jpeg" YUV ranges).
  // TODO(crbug/810131): Once color space issues are fixed, remove this.
  static constexpr int kMaxInaccurateColorDifference = 48;
};

// A convenience for logging and gtest expectations output.
std::ostream& operator<<(std::ostream& out, const FrameTestUtil::RGB& rgb);

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_FRAME_TEST_UTIL_H_
