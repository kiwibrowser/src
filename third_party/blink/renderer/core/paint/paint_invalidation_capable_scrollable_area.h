// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_INVALIDATION_CAPABLE_SCROLLABLE_AREA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_INVALIDATION_CAPABLE_SCROLLABLE_AREA_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/scroll/scrollable_area.h"

namespace blink {

class LayoutScrollbarPart;
struct PaintInvalidatorContext;

// Base class of LocalFrameView and PaintLayerScrollableArea to share paint
// invalidation code.
// TODO(wangxianzhu): Combine this into PaintLayerScrollableArea when
// root-layer-scrolls launches.
class CORE_EXPORT PaintInvalidationCapableScrollableArea
    : public ScrollableArea {
 public:
  PaintInvalidationCapableScrollableArea()
      : horizontal_scrollbar_previously_was_overlay_(false),
        vertical_scrollbar_previously_was_overlay_(false) {}

  void WillRemoveScrollbar(Scrollbar&, ScrollbarOrientation) override;

  void InvalidatePaintOfScrollControlsIfNeeded(const PaintInvalidatorContext&);

  // Should be called when the previous visual rects are no longer valid.
  void ClearPreviousVisualRects();

  virtual IntRect ScrollCornerAndResizerRect() const {
    return ScrollCornerRect();
  }

  void DidScrollWithScrollbar(ScrollbarPart, ScrollbarOrientation) override;
  CompositorElementId GetCompositorElementId() const override;

 private:
  virtual LayoutScrollbarPart* ScrollCorner() const = 0;
  virtual LayoutScrollbarPart* Resizer() const = 0;

  void ScrollControlWasSetNeedsPaintInvalidation() override;

  void SetHorizontalScrollbarVisualRect(const LayoutRect&);
  void SetVerticalScrollbarVisualRect(const LayoutRect&);
  void SetScrollCornerAndResizerVisualRect(const LayoutRect&);

  bool horizontal_scrollbar_previously_was_overlay_;
  bool vertical_scrollbar_previously_was_overlay_;
  LayoutRect horizontal_scrollbar_visual_rect_;
  LayoutRect vertical_scrollbar_visual_rect_;
  LayoutRect scroll_corner_and_resizer_visual_rect_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_INVALIDATION_CAPABLE_SCROLLABLE_AREA_H_
