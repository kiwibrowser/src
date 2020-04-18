// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_MIN_MAX_SIZE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_MIN_MAX_SIZE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/layout_unit.h"

namespace blink {

// A struct that holds a pair of two sizes, a "min" size and a "max" size.
// Useful for holding a {min,max}-content size pair or a
// {min,max}-{width,height}.
struct CORE_EXPORT MinMaxSize {
  LayoutUnit min_size;
  LayoutUnit max_size;

  // Interprets the sizes as a min-content/max-content pair and computes the
  // "shrink-to-fit" size based on them for the given available size.
  LayoutUnit ShrinkToFit(LayoutUnit available_size) const;

  // Interprets the sizes as a {min-max}-size pair and clamps the given input
  // size to that.
  LayoutUnit ClampSizeToMinAndMax(LayoutUnit) const;

  bool operator==(const MinMaxSize& other) const {
    return min_size == other.min_size && max_size == other.max_size;
  }

  MinMaxSize& operator+=(const LayoutUnit);
};

CORE_EXPORT std::ostream& operator<<(std::ostream&, const MinMaxSize&);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_MIN_MAX_SIZE_H_
