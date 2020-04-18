// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/accessibility/ax_selection.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/position_with_affinity.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/set_selection_options.h"
#include "third_party/blink/renderer/core/editing/text_affinity.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object.h"

namespace blink {

AXSelection::Builder& AXSelection::Builder::SetBase(const AXPosition& base) {
  DCHECK(base.IsValid());
  selection_.base_ = base;
  return *this;
}

AXSelection::Builder& AXSelection::Builder::SetBase(const Position& base) {
  const auto ax_base = AXPosition::FromPosition(base);
  DCHECK(ax_base.IsValid());
  selection_.base_ = ax_base;
  return *this;
}

AXSelection::Builder& AXSelection::Builder::SetExtent(
    const AXPosition& extent) {
  DCHECK(extent.IsValid());
  selection_.extent_ = extent;
  return *this;
}

AXSelection::Builder& AXSelection::Builder::SetExtent(const Position& extent) {
  const auto ax_extent = AXPosition::FromPosition(extent);
  DCHECK(ax_extent.IsValid());
  selection_.extent_ = ax_extent;
  return *this;
}

AXSelection::Builder& AXSelection::Builder::SetSelection(
    const SelectionInDOMTree& selection) {
  if (selection.IsNone())
    return *this;

  selection_.base_ = AXPosition::FromPosition(selection.Base());
  selection_.extent_ = AXPosition::FromPosition(selection.Extent());
  return *this;
}

const AXSelection AXSelection::Builder::Build() {
  if (!selection_.Base().IsValid() || !selection_.Extent().IsValid())
    return {};

  const Document* document = selection_.Base().ContainerObject()->GetDocument();
  DCHECK(document);
  DCHECK(document->IsActive());
  DCHECK(!document->NeedsLayoutTreeUpdate());
#if DCHECK_IS_ON()
  selection_.dom_tree_version_ = document->DomTreeVersion();
  selection_.style_version_ = document->StyleVersion();
#endif
  return selection_;
}

AXSelection::AXSelection() : base_(), extent_() {
#if DCHECK_IS_ON()
  dom_tree_version_ = 0;
  style_version_ = 0;
#endif
}

bool AXSelection::IsValid() const {
  if (!base_.IsValid() || !extent_.IsValid())
    return false;

  // We don't support selections that span across documents.
  if (base_.ContainerObject()->GetDocument() !=
      extent_.ContainerObject()->GetDocument()) {
    return false;
  }

  DCHECK(!base_.ContainerObject()->GetDocument()->NeedsLayoutTreeUpdate());
#if DCHECK_IS_ON()
  DCHECK_EQ(base_.ContainerObject()->GetDocument()->DomTreeVersion(),
            dom_tree_version_);
  DCHECK_EQ(base_.ContainerObject()->GetDocument()->StyleVersion(),
            style_version_);
#endif  // DCHECK_IS_ON()
  return true;
}

const SelectionInDOMTree AXSelection::AsSelection(
    const AXSelectionBehavior selection_behavior) const {
  if (!IsValid())
    return {};

  AXPositionAdjustmentBehavior base_adjustment =
      AXPositionAdjustmentBehavior::kMoveLeft;
  AXPositionAdjustmentBehavior extent_adjustment =
      AXPositionAdjustmentBehavior::kMoveLeft;
  switch (selection_behavior) {
    case AXSelectionBehavior::kShrinkToValidDOMRange:
      if (base_ <= extent_) {
        base_adjustment = AXPositionAdjustmentBehavior::kMoveRight;
        extent_adjustment = AXPositionAdjustmentBehavior::kMoveLeft;
      } else {
        base_adjustment = AXPositionAdjustmentBehavior::kMoveLeft;
        extent_adjustment = AXPositionAdjustmentBehavior::kMoveRight;
      }
      break;
    case AXSelectionBehavior::kExtendToValidDOMRange:
      if (base_ <= extent_) {
        base_adjustment = AXPositionAdjustmentBehavior::kMoveLeft;
        extent_adjustment = AXPositionAdjustmentBehavior::kMoveRight;
      } else {
        base_adjustment = AXPositionAdjustmentBehavior::kMoveRight;
        extent_adjustment = AXPositionAdjustmentBehavior::kMoveLeft;
      }
      break;
  }

  const auto dom_base = base_.ToPositionWithAffinity(base_adjustment);
  const auto dom_extent = extent_.ToPositionWithAffinity(extent_adjustment);
  SelectionInDOMTree::Builder selection_builder;
  selection_builder.SetBaseAndExtent(dom_base.GetPosition(),
                                     dom_extent.GetPosition());
  selection_builder.SetAffinity(extent_.Affinity());
  return selection_builder.Build();
}

void AXSelection::Select(const AXSelectionBehavior selection_behavior) {
  if (!IsValid()) {
    NOTREACHED();
    return;
  }

  const SelectionInDOMTree selection = AsSelection(selection_behavior);
  DCHECK(selection.AssertValid());
  Document* document = selection.Base().GetDocument();
  if (!document) {
    NOTREACHED();
    return;
  }
  LocalFrame* frame = document->GetFrame();
  if (!frame) {
    NOTREACHED();
    return;
  }
  FrameSelection& frame_selection = frame->Selection();
  frame_selection.SetSelection(selection, SetSelectionOptions());
}

bool operator==(const AXSelection& a, const AXSelection& b) {
  DCHECK(a.IsValid() && b.IsValid());
  return a.Base() == b.Base() && a.Extent() == b.Extent();
}

bool operator!=(const AXSelection& a, const AXSelection& b) {
  return !(a == b);
}

std::ostream& operator<<(std::ostream& ostream, const AXSelection& selection) {
  if (!selection.IsValid())
    return ostream << "Invalid AXSelection";
  return ostream << "AXSelection from " << selection.Base() << " to "
                 << selection.Extent();
}

}  // namespace blink
