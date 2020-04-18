// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/custom/custom_layout_fragment.h"

#include "third_party/blink/renderer/core/layout/custom/custom_layout_fragment_request.h"

namespace blink {

CustomLayoutFragment::CustomLayoutFragment(
    CustomLayoutFragmentRequest* fragment_request,
    const LayoutUnit inline_size,
    const LayoutUnit block_size)
    : fragment_request_(fragment_request),
      inline_size_(inline_size.ToDouble()),
      block_size_(block_size.ToDouble()) {}

LayoutBox* CustomLayoutFragment::GetLayoutBox() const {
  return fragment_request_->GetLayoutBox();
}

bool CustomLayoutFragment::IsValid() const {
  return fragment_request_->IsValid();
}

void CustomLayoutFragment::Trace(blink::Visitor* visitor) {
  visitor->Trace(fragment_request_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
