// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/geometry/double_size.h"
#include "third_party/blink/renderer/platform/geometry/layout_size.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

#include <limits>
#include <math.h>

namespace blink {

DoubleSize::DoubleSize(const LayoutSize& size)
    : width_(size.Width().ToDouble()), height_(size.Height().ToDouble()) {}

bool DoubleSize::IsZero() const {
  return fabs(width_) < std::numeric_limits<double>::epsilon() &&
         fabs(height_) < std::numeric_limits<double>::epsilon();
}

std::ostream& operator<<(std::ostream& ostream, const DoubleSize& size) {
  return ostream << size.ToString();
}

String DoubleSize::ToString() const {
  return String::Format("%lgx%lg", Width(), Height());
}

}  // namespace blink
