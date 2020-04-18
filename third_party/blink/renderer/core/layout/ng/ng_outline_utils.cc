// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_outline_utils.h"

#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_fragment_traversal.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_box_fragment.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"

namespace blink {

class LayoutObject;

void NGOutlineUtils::CollectDescendantOutlines(
    const NGPhysicalBoxFragment& container,
    const NGPhysicalOffset& paint_offset,
    FragmentMap* anchor_fragment_map,
    OutlineRectMap* outline_rect_map) {
  DCHECK(anchor_fragment_map->IsEmpty());
  DCHECK(outline_rect_map->IsEmpty());
  if (!container.ChildrenInline())
    return;
  for (auto& descendant : NGInlineFragmentTraversal::DescendantsOf(container)) {
    if (!descendant.fragment->IsBox() || descendant.fragment->IsAtomicInline())
      continue;

    const ComputedStyle& descendant_style = descendant.fragment->Style();
    if (!descendant_style.HasOutline() ||
        descendant_style.Visibility() != EVisibility::kVisible)
      continue;
    if (descendant_style.OutlineStyleIsAuto() &&
        !LayoutTheme::GetTheme().ShouldDrawDefaultFocusRing(
            descendant.fragment->GetNode(), descendant_style))
      continue;

    const LayoutObject* layout_object = descendant.fragment->GetLayoutObject();
    Vector<LayoutRect>* outline_rects;
    auto iter = outline_rect_map->find(layout_object);
    if (iter == outline_rect_map->end()) {
      anchor_fragment_map->insert(layout_object, descendant.fragment.get());
      outline_rects =
          &outline_rect_map->insert(layout_object, Vector<LayoutRect>())
               .stored_value->value;
    } else {
      outline_rects = &iter->value;
    }
    NGPhysicalOffsetRect outline(
        paint_offset + descendant.offset_to_container_box,
        descendant.fragment->Size());
    outline_rects->push_back(outline.ToLayoutRect());
  }
}

NGPhysicalOffsetRect NGOutlineUtils::ComputeEnclosingOutline(
    const ComputedStyle& style,
    const Vector<LayoutRect>& outlines) {
  LayoutRect combined_outline;
  for (auto& outline : outlines) {
    combined_outline.Unite(outline);
  }
  combined_outline.Inflate(style.OutlineOffset() + style.OutlineWidth());
  return NGPhysicalOffsetRect(
      NGPhysicalOffset(combined_outline.X(), combined_outline.Y()),
      NGPhysicalSize(combined_outline.Width(), combined_outline.Height()));
}

}  // namespace blink
