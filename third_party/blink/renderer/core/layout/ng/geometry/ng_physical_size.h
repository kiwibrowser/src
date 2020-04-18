// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGPhysicalSize_h
#define NGPhysicalSize_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/layout_unit.h"
#include "third_party/blink/renderer/platform/text/writing_mode.h"

namespace blink {

class LayoutSize;
struct NGLogicalSize;

// NGPhysicalSize is the size of a rect (typically a fragment) in the physical
// coordinate system.
struct CORE_EXPORT NGPhysicalSize {
  NGPhysicalSize() = default;
  NGPhysicalSize(LayoutUnit width, LayoutUnit height)
      : width(width), height(height) {}

  LayoutUnit width;
  LayoutUnit height;

  NGLogicalSize ConvertToLogical(WritingMode mode) const;

  bool operator==(const NGPhysicalSize& other) const;

  bool IsEmpty() const {
    return width == LayoutUnit() || height == LayoutUnit();
  }

  // Conversions from/to existing code. New code prefers type safety for
  // logical/physical distinctions.
  LayoutSize ToLayoutSize() const;

  String ToString() const;
};

CORE_EXPORT std::ostream& operator<<(std::ostream&, const NGPhysicalSize&);

}  // namespace blink

#endif  // NGPhysicalSize_h
