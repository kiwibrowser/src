// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/layout_paged_flow_thread.h"

#include "third_party/blink/renderer/core/layout/layout_multi_column_set.h"

namespace blink {

LayoutPagedFlowThread* LayoutPagedFlowThread::CreateAnonymous(
    Document& document,
    const ComputedStyle& parent_style) {
  LayoutPagedFlowThread* paged_flow_thread = new LayoutPagedFlowThread();
  paged_flow_thread->SetDocumentForAnonymous(&document);
  paged_flow_thread->SetStyle(ComputedStyle::CreateAnonymousStyleWithDisplay(
      parent_style, EDisplay::kBlock));
  return paged_flow_thread;
}

int LayoutPagedFlowThread::PageCount() {
  if (LayoutMultiColumnSet* column_set = FirstMultiColumnSet())
    return column_set->ActualColumnCount();
  return 1;
}

bool LayoutPagedFlowThread::NeedsNewWidth() const {
  return ProgressionIsInline() !=
         PagedBlockFlow()->Style()->HasInlinePaginationAxis();
}

void LayoutPagedFlowThread::UpdateLogicalWidth() {
  // As long as we inherit from LayoutMultiColumnFlowThread, we need to bypass
  // its implementation here. We're not split into columns, so the flow thread
  // width will just be whatever is available in the containing block.
  LayoutFlowThread::UpdateLogicalWidth();
}

void LayoutPagedFlowThread::UpdateLayout() {
  // There should either be zero or one of those for paged layout.
  DCHECK_EQ(FirstMultiColumnBox(), LastMultiColumnBox());
  SetProgressionIsInline(PagedBlockFlow()->Style()->HasInlinePaginationAxis());
  LayoutMultiColumnFlowThread::UpdateLayout();

  LayoutMultiColumnSet* column_set = FirstMultiColumnSet();
  if (!column_set)
    return;
  if (!IsPageLogicalHeightKnown()) {
    // Page height not calculated yet. Happens in the first layout pass when
    // height is auto.
    return;
  }
  LayoutUnit page_logical_height =
      column_set->PageLogicalHeightForOffset(LayoutUnit());
  // Ensure uniform page height. We don't want the last page to be shorter than
  // the others, or it'll be impossible to scroll that whole page into view.
  LayoutUnit padded_logical_bottom_in_flow_thread =
      page_logical_height * PageCount();
  // In some cases we clamp the page count (see
  // MultiColumnFragmentainerGroup::ActualColumnCount()), so that the padded
  // offset will be less than what we already have. Forget about uniform page
  // height in that case.
  if (padded_logical_bottom_in_flow_thread >
      column_set->LogicalBottomInFlowThread())
    column_set->EndFlow(padded_logical_bottom_in_flow_thread);
}

}  // namespace blink
