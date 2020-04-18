// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/min_max_size.h"

#include <algorithm>

namespace blink {

LayoutUnit MinMaxSize::ShrinkToFit(LayoutUnit available_size) const {
  DCHECK_GE(max_size, min_size);
  return std::min(max_size, std::max(min_size, available_size));
}

LayoutUnit MinMaxSize::ClampSizeToMinAndMax(LayoutUnit size) const {
  return std::max(min_size, std::min(size, max_size));
}

MinMaxSize& MinMaxSize::operator+=(const LayoutUnit length) {
  min_size += length;
  max_size += length;
  return *this;
}

std::ostream& operator<<(std::ostream& stream, const MinMaxSize& value) {
  return stream << "(" << value.min_size << ", " << value.max_size << ")";
}

}  // namespace blink
