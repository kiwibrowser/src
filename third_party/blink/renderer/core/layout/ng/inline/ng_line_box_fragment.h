// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGLineBoxFragment_h
#define NGLineBoxFragment_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_line_box_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/ng_fragment.h"
#include "third_party/blink/renderer/platform/text/writing_mode.h"

namespace blink {

class CORE_EXPORT NGLineBoxFragment final : public NGFragment {
 public:
  NGLineBoxFragment(WritingMode writing_mode,
                    const NGPhysicalLineBoxFragment& physical_fragment)
      : NGFragment(writing_mode, physical_fragment) {}
};

DEFINE_TYPE_CASTS(NGLineBoxFragment,
                  NGFragment,
                  fragment,
                  fragment->Type() == NGPhysicalFragment::kFragmentLineBox,
                  fragment.Type() == NGPhysicalFragment::kFragmentLineBox);

}  // namespace blink

#endif  // NGLineBoxFragment_h
