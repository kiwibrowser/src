// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGBoxStrut_h
#define NGBoxStrut_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_offset.h"
#include "third_party/blink/renderer/platform/layout_unit.h"
#include "third_party/blink/renderer/platform/text/text_direction.h"
#include "third_party/blink/renderer/platform/text/writing_mode.h"

namespace blink {

struct NGPhysicalBoxStrut;

// This struct is used for storing margins, borders or padding of a box on all
// four edges.
struct CORE_EXPORT NGBoxStrut {
  NGBoxStrut() = default;
  NGBoxStrut(LayoutUnit inline_start,
             LayoutUnit inline_end,
             LayoutUnit block_start,
             LayoutUnit block_end)
      : inline_start(inline_start),
        inline_end(inline_end),
        block_start(block_start),
        block_end(block_end) {}

  LayoutUnit LineLeft(TextDirection direction) const {
    return IsLtr(direction) ? inline_start : inline_end;
  }
  LayoutUnit LineRight(TextDirection direction) const {
    return IsLtr(direction) ? inline_end : inline_start;
  }

  LayoutUnit InlineSum() const { return inline_start + inline_end; }
  LayoutUnit BlockSum() const { return block_start + block_end; }

  NGLogicalOffset StartOffset() const { return {inline_start, block_start}; }

  bool IsEmpty() const;

  NGPhysicalBoxStrut ConvertToPhysical(WritingMode, TextDirection) const;

  // The following two operators exist primarily to have an easy way to access
  // the sum of border and padding.
  NGBoxStrut& operator+=(const NGBoxStrut& other) {
    inline_start += other.inline_start;
    inline_end += other.inline_end;
    block_start += other.block_start;
    block_end += other.block_end;
    return *this;
  }

  NGBoxStrut operator+(const NGBoxStrut& other) {
    NGBoxStrut result(*this);
    result += other;
    return result;
  }

  NGBoxStrut& operator-=(const NGBoxStrut& other) {
    inline_start -= other.inline_start;
    inline_end -= other.inline_end;
    block_start -= other.block_start;
    block_end -= other.block_end;
    return *this;
  }

  NGBoxStrut operator-(const NGBoxStrut& other) {
    NGBoxStrut result(*this);
    result -= other;
    return result;
  }

  bool operator==(const NGBoxStrut& other) const;

  String ToString() const;

  LayoutUnit inline_start;
  LayoutUnit inline_end;
  LayoutUnit block_start;
  LayoutUnit block_end;
};

CORE_EXPORT std::ostream& operator<<(std::ostream&, const NGBoxStrut&);

struct NGPixelSnappedPhysicalBoxStrut;

// Struct to store physical dimensions, independent of writing mode and
// direction.
// See https://drafts.csswg.org/css-writing-modes-3/#abstract-box
struct CORE_EXPORT NGPhysicalBoxStrut {
  NGPhysicalBoxStrut() = default;
  NGPhysicalBoxStrut(LayoutUnit top,
                     LayoutUnit right,
                     LayoutUnit bottom,
                     LayoutUnit left)
      : top(top), right(right), bottom(bottom), left(left) {}

  NGBoxStrut ConvertToLogical(WritingMode, TextDirection) const;
  NGPixelSnappedPhysicalBoxStrut SnapToDevicePixels() const;

  LayoutUnit HorizontalSum() const { return left + right; }
  LayoutUnit VerticalSum() const { return top + bottom; }

  LayoutUnit top;
  LayoutUnit right;
  LayoutUnit bottom;
  LayoutUnit left;
};

// Struct to store pixel snapped physical dimensions.
struct CORE_EXPORT NGPixelSnappedPhysicalBoxStrut {
  NGPixelSnappedPhysicalBoxStrut() = default;
  NGPixelSnappedPhysicalBoxStrut(int top, int right, int bottom, int left)
      : top(top), right(right), bottom(bottom), left(left) {}
  int top;
  int right;
  int bottom;
  int left;
};

}  // namespace blink

#endif  // NGBoxStrut_h
