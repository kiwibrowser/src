// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_offset.h"

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_size.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_offset.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_size.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

NGPhysicalOffset NGLogicalOffset::ConvertToPhysical(
    WritingMode mode,
    TextDirection direction,
    NGPhysicalSize outer_size,
    NGPhysicalSize inner_size) const {
  switch (mode) {
    case WritingMode::kHorizontalTb:
      if (direction == TextDirection::kLtr)
        return NGPhysicalOffset(inline_offset, block_offset);
      else
        return NGPhysicalOffset(
            outer_size.width - inline_offset - inner_size.width, block_offset);
    case WritingMode::kVerticalRl:
    case WritingMode::kSidewaysRl:
      if (direction == TextDirection::kLtr)
        return NGPhysicalOffset(
            outer_size.width - block_offset - inner_size.width, inline_offset);
      else
        return NGPhysicalOffset(
            outer_size.width - block_offset - inner_size.width,
            outer_size.height - inline_offset - inner_size.height);
    case WritingMode::kVerticalLr:
      if (direction == TextDirection::kLtr)
        return NGPhysicalOffset(block_offset, inline_offset);
      else
        return NGPhysicalOffset(
            block_offset,
            outer_size.height - inline_offset - inner_size.height);
    case WritingMode::kSidewaysLr:
      if (direction == TextDirection::kLtr)
        return NGPhysicalOffset(
            block_offset,
            outer_size.height - inline_offset - inner_size.height);
      else
        return NGPhysicalOffset(block_offset, inline_offset);
    default:
      NOTREACHED();
      return NGPhysicalOffset();
  }
}

bool NGLogicalOffset::operator==(const NGLogicalOffset& other) const {
  return std::tie(other.inline_offset, other.block_offset) ==
         std::tie(inline_offset, block_offset);
}

bool NGLogicalOffset::operator!=(const NGLogicalOffset& other) const {
  return !operator==(other);
}

NGLogicalOffset NGLogicalOffset::operator+(const NGLogicalOffset& other) const {
  NGLogicalOffset result;
  result.inline_offset = this->inline_offset + other.inline_offset;
  result.block_offset = this->block_offset + other.block_offset;
  return result;
}

NGLogicalOffset NGLogicalOffset::operator+(const NGLogicalSize& size) const {
  return {inline_offset + size.inline_size, block_offset + size.block_size};
}

NGLogicalOffset& NGLogicalOffset::operator+=(const NGLogicalOffset& other) {
  *this = *this + other;
  return *this;
}

bool NGLogicalOffset::operator>(const NGLogicalOffset& other) const {
  return inline_offset > other.inline_offset &&
         block_offset > other.block_offset;
}

bool NGLogicalOffset::operator>=(const NGLogicalOffset& other) const {
  return inline_offset >= other.inline_offset &&
         block_offset >= other.block_offset;
}

bool NGLogicalOffset::operator<(const NGLogicalOffset& other) const {
  return inline_offset < other.inline_offset &&
         block_offset < other.block_offset;
}

bool NGLogicalOffset::operator<=(const NGLogicalOffset& other) const {
  return inline_offset <= other.inline_offset &&
         block_offset <= other.block_offset;
}

NGLogicalDelta NGLogicalOffset::operator-(const NGLogicalOffset& other) const {
  return {inline_offset - other.inline_offset,
          block_offset - other.block_offset};
}

NGLogicalOffset& NGLogicalOffset::operator-=(const NGLogicalOffset& other) {
  *this = *this - other;
  return *this;
}

String NGLogicalOffset::ToString() const {
  return String::Format("%d,%d", inline_offset.ToInt(), block_offset.ToInt());
}

std::ostream& operator<<(std::ostream& os, const NGLogicalOffset& value) {
  return os << value.ToString();
}

}  // namespace blink
