// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/geometry/float_rect_outsets.h"

#include <algorithm>

namespace blink {

// Change outsets to be at least as large as |other|.
void FloatRectOutsets::Unite(const FloatRectOutsets& other) {
  top_ = std::max(top_, other.top_);
  right_ = std::max(right_, other.right_);
  bottom_ = std::max(bottom_, other.bottom_);
  left_ = std::max(left_, other.left_);
}

}  // namespace blink
