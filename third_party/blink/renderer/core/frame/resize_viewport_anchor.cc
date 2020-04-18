// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/resize_viewport_anchor.h"

#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/root_frame_viewport.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/geometry/double_rect.h"
#include "third_party/blink/renderer/platform/geometry/float_size.h"

namespace blink {

void ResizeViewportAnchor::ResizeFrameView(const IntSize& size) {
  LocalFrameView* frame_view = RootFrameView();
  if (!frame_view)
    return;

  ScrollableArea* root_viewport = frame_view->GetScrollableArea();
  ScrollOffset offset = root_viewport->GetScrollOffset();

  frame_view->Resize(size);
  if (scope_count_ > 0)
    drift_ += root_viewport->GetScrollOffset() - offset;
}

void ResizeViewportAnchor::EndScope() {
  if (--scope_count_ > 0)
    return;

  LocalFrameView* frame_view = RootFrameView();
  if (!frame_view)
    return;

  ScrollOffset visual_viewport_in_document =
      frame_view->GetScrollableArea()->GetScrollOffset() - drift_;

  // TODO(bokan): Don't use RootFrameViewport::setScrollPosition since it
  // assumes we can just set a sub-pixel precision offset on the LocalFrameView.
  // While we "can" do this, the offset that will be shipped to CC will be the
  // truncated number and this class is used to handle TopControl movement
  // which needs the two threads to match exactly pixel-for-pixel. We can
  // replace this with RFV::setScrollPosition once Blink is sub-pixel scroll
  // offset aware. crbug.com/414283.
  DCHECK(frame_view->GetRootFrameViewport());
  frame_view->GetRootFrameViewport()->RestoreToAnchor(
      visual_viewport_in_document);

  drift_ = ScrollOffset();
}

LocalFrameView* ResizeViewportAnchor::RootFrameView() {
  if (Frame* frame = page_->MainFrame()) {
    if (frame->IsLocalFrame())
      return ToLocalFrame(frame)->View();
  }
  return nullptr;
}

}  // namespace blink
