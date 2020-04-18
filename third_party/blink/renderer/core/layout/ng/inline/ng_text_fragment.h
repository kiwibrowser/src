// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGTextFragment_h
#define NGTextFragment_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_text_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/ng_fragment.h"

namespace blink {

class CORE_EXPORT NGTextFragment final : public NGFragment {
 public:
  NGTextFragment(WritingMode writing_mode,
                 const NGPhysicalTextFragment& physical_text_fragment)
      : NGFragment(writing_mode, physical_text_fragment) {}
};

}  // namespace blink

#endif  // NGTextFragment_h
