// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_size.h"

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_size.h"
#include "third_party/blink/renderer/platform/geometry/layout_size.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

bool NGPhysicalSize::operator==(const NGPhysicalSize& other) const {
  return std::tie(other.width, other.height) == std::tie(width, height);
}

NGLogicalSize NGPhysicalSize::ConvertToLogical(WritingMode mode) const {
  return mode == WritingMode::kHorizontalTb ? NGLogicalSize(width, height)
                                            : NGLogicalSize(height, width);
}

LayoutSize NGPhysicalSize::ToLayoutSize() const {
  return {width, height};
}

String NGPhysicalSize::ToString() const {
  return String::Format("%dx%d", width.ToInt(), height.ToInt());
}

std::ostream& operator<<(std::ostream& os, const NGPhysicalSize& value) {
  return os << value.ToString();
}

}  // namespace blink
