// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/scroll_timeline.h"

#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"

namespace blink {

namespace {
using ActiveScrollTimelineSet = PersistentHeapHashCountedSet<WeakMember<Node>>;
ActiveScrollTimelineSet& GetActiveScrollTimelineSet() {
  DEFINE_STATIC_LOCAL(ActiveScrollTimelineSet, set, ());
  return set;
}

bool StringToScrollDirection(String scroll_direction,
                             ScrollTimeline::ScrollDirection& result) {
  // TODO(smcgruer): Support 'auto' value.
  if (scroll_direction == "block") {
    result = ScrollTimeline::Block;
    return true;
  }
  if (scroll_direction == "inline") {
    result = ScrollTimeline::Inline;
    return true;
  }
  return false;
}
}  // namespace

ScrollTimeline* ScrollTimeline::Create(Document& document,
                                       ScrollTimelineOptions options,
                                       ExceptionState& exception_state) {
  Element* scroll_source = options.scrollSource() ? options.scrollSource()
                                                  : document.scrollingElement();

  ScrollDirection orientation;
  if (!StringToScrollDirection(options.orientation(), orientation)) {
    exception_state.ThrowDOMException(kNotSupportedError,
                                      "Invalid orientation");
    return nullptr;
  }

  // TODO(smcgruer): Support 'auto' value.
  if (options.timeRange().IsScrollTimelineAutoKeyword()) {
    exception_state.ThrowDOMException(
        kNotSupportedError, "'auto' value for timeRange not yet supported");
    return nullptr;
  }

  return new ScrollTimeline(scroll_source, orientation,
                            options.timeRange().GetAsDouble());
}

ScrollTimeline::ScrollTimeline(Element* scroll_source,
                               ScrollDirection orientation,
                               double time_range)
    : scroll_source_(scroll_source),
      orientation_(orientation),
      time_range_(time_range) {
  DCHECK(scroll_source_);
}

double ScrollTimeline::currentTime(bool& is_null) {
  // 1. If scrollSource does not currently have a CSS layout box, or if its
  // layout box is not a scroll container, return an unresolved time value.
  LayoutBox* layout_box = scroll_source_->GetLayoutBox();
  if (!layout_box || !layout_box->HasOverflowClip()) {
    is_null = false;
    return std::numeric_limits<double>::quiet_NaN();
  }

  // 2. Otherwise, let current scroll offset be the current scroll offset of
  // scrollSource in the direction specified by orientation.

  // Depending on the writing-mode and direction, the scroll origin shifts and
  // the scroll offset may be negative. The easiest way to deal with this is to
  // use only the magnitude of the scroll offset, and compare it to (max-offset
  // - min_offset).
  PaintLayerScrollableArea* scrollable_area = layout_box->GetScrollableArea();
  // Using the absolute value of the scroll offset only makes sense if either
  // the max or min scroll offset for a given axis is 0. This should be
  // guaranteed by the scroll origin code, but these DCHECKs ensure that.
  DCHECK(scrollable_area->MaximumScrollOffset().Height() == 0 ||
         scrollable_area->MinimumScrollOffset().Height() == 0);
  DCHECK(scrollable_area->MaximumScrollOffset().Width() == 0 ||
         scrollable_area->MinimumScrollOffset().Width() == 0);
  ScrollOffset scroll_offset = scrollable_area->GetScrollOffset();
  ScrollOffset scroll_dimensions = scrollable_area->MaximumScrollOffset() -
                                   scrollable_area->MinimumScrollOffset();

  double current_offset;
  double max_offset;
  bool is_horizontal = layout_box->IsHorizontalWritingMode();
  if (orientation_ == Block) {
    current_offset =
        is_horizontal ? scroll_offset.Height() : scroll_offset.Width();
    max_offset =
        is_horizontal ? scroll_dimensions.Height() : scroll_dimensions.Width();
  } else {
    DCHECK(orientation_ == Inline);
    current_offset =
        is_horizontal ? scroll_offset.Width() : scroll_offset.Height();
    max_offset =
        is_horizontal ? scroll_dimensions.Width() : scroll_dimensions.Height();
  }

  // 3. If current scroll offset is less than startScrollOffset, return an
  // unresolved time value if fill is none or forwards, or 0 otherwise.
  // TODO(smcgruer): Implement |startScrollOffset| and |fill|.

  // 4. If current scroll offset is greater than or equal to endScrollOffset,
  // return an unresolved time value if fill is none or backwards, or the
  // effective time range otherwise.
  // TODO(smcgruer): Implement |endScrollOffset| and |fill|.

  // 5. Return the result of evaluating the following expression:
  //   ((current scroll offset - startScrollOffset) /
  //      (endScrollOffset - startScrollOffset)) * effective time range

  is_null = false;
  return (std::abs(current_offset) / max_offset) * time_range_;
}

Element* ScrollTimeline::scrollSource() {
  return scroll_source_.Get();
}

String ScrollTimeline::orientation() {
  switch (orientation_) {
    case Block:
      return "block";
    case Inline:
      return "inline";
    default:
      NOTREACHED();
      return "";
  }
}

void ScrollTimeline::timeRange(DoubleOrScrollTimelineAutoKeyword& result) {
  result.SetDouble(time_range_);
}

void ScrollTimeline::AttachAnimation() {
  GetActiveScrollTimelineSet().insert(scroll_source_);
  scroll_source_->GetDocument()
      .GetLayoutView()
      ->Compositor()
      ->SetNeedsCompositingUpdate(kCompositingUpdateRebuildTree);
}

void ScrollTimeline::DetachAnimation() {
  GetActiveScrollTimelineSet().erase(scroll_source_);
  auto* layout_view = scroll_source_->GetDocument().GetLayoutView();
  if (layout_view && layout_view->Compositor()) {
    layout_view->Compositor()->SetNeedsCompositingUpdate(
        kCompositingUpdateRebuildTree);
  }
}

void ScrollTimeline::Trace(blink::Visitor* visitor) {
  visitor->Trace(scroll_source_);
  AnimationTimeline::Trace(visitor);
}

bool ScrollTimeline::HasActiveScrollTimeline(Node* node) {
  ActiveScrollTimelineSet& set = GetActiveScrollTimelineSet();
  auto it = set.find(node);
  return it != set.end() && it->value > 0;
}

}  // namespace blink
