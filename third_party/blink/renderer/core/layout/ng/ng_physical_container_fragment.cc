// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_physical_container_fragment.h"

namespace blink {

NGPhysicalContainerFragment::NGPhysicalContainerFragment(
    LayoutObject* layout_object,
    const ComputedStyle& style,
    NGStyleVariant style_variant,
    NGPhysicalSize size,
    NGFragmentType type,
    unsigned sub_type,
    Vector<scoped_refptr<NGPhysicalFragment>>& children,
    const NGPhysicalOffsetRect& contents_visual_rect,
    scoped_refptr<NGBreakToken> break_token)
    : NGPhysicalFragment(layout_object,
                         style,
                         style_variant,
                         size,
                         type,
                         sub_type,
                         std::move(break_token)),
      children_(std::move(children)),
      contents_visual_rect_(contents_visual_rect) {
  DCHECK(children.IsEmpty());  // Ensure move semantics is used.
}

}  // namespace blink
