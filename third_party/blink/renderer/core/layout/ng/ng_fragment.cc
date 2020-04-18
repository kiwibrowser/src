// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_fragment.h"

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_border_edges.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_size.h"

namespace blink {

LayoutUnit NGFragment::InlineSize() const {
  return GetWritingMode() == WritingMode::kHorizontalTb
             ? physical_fragment_.Size().width
             : physical_fragment_.Size().height;
}

LayoutUnit NGFragment::BlockSize() const {
  return GetWritingMode() == WritingMode::kHorizontalTb
             ? physical_fragment_.Size().height
             : physical_fragment_.Size().width;
}

NGLogicalSize NGFragment::Size() const {
  return physical_fragment_.Size().ConvertToLogical(
      static_cast<WritingMode>(writing_mode_));
}

NGBorderEdges NGFragment::BorderEdges() const {
  return NGBorderEdges::FromPhysical(physical_fragment_.BorderEdges(),
                                     GetWritingMode());
}

NGPhysicalFragment::NGFragmentType NGFragment::Type() const {
  return physical_fragment_.Type();
}

}  // namespace blink
