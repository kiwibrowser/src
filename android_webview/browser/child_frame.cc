// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/child_frame.h"

#include <utility>

#include "base/trace_event/trace_event.h"
#include "components/viz/common/quads/compositor_frame.h"

namespace android_webview {

ChildFrame::ChildFrame(
    scoped_refptr<content::SynchronousCompositor::FrameFuture> frame_future,
    const CompositorID& compositor_id,
    bool viewport_rect_for_tile_priority_empty,
    const gfx::Transform& transform_for_tile_priority,
    bool offscreen_pre_raster,
    bool is_layer)
    : frame_future(std::move(frame_future)),
      compositor_id(compositor_id),
      viewport_rect_for_tile_priority_empty(
          viewport_rect_for_tile_priority_empty),
      transform_for_tile_priority(transform_for_tile_priority),
      offscreen_pre_raster(offscreen_pre_raster),
      is_layer(is_layer) {}

ChildFrame::~ChildFrame() {
}

void ChildFrame::WaitOnFutureIfNeeded() {
  if (!frame_future)
    return;

  TRACE_EVENT0("android_webview", "GetFrame");
  DCHECK(!frame);
  auto frame_ptr = frame_future->GetFrame();
  if (frame_ptr) {
    layer_tree_frame_sink_id = frame_ptr->layer_tree_frame_sink_id;
    frame = std::move(frame_ptr->frame);
  }
  frame_future = nullptr;
}

}  // namespace android_webview
