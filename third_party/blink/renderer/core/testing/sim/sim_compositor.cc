// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/testing/sim/sim_compositor.h"

#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/cull_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_record_builder.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

SimCompositor::SimCompositor()
    : needs_begin_frame_(false),
      defer_commits_(true),
      has_selection_(false),
      web_view_(nullptr) {
  LocalFrameView::SetInitialTracksPaintInvalidationsForTesting(true);
}

SimCompositor::~SimCompositor() {
  LocalFrameView::SetInitialTracksPaintInvalidationsForTesting(false);
}

void SimCompositor::SetWebView(WebViewImpl& web_view) {
  web_view_ = &web_view;
}

void SimCompositor::SetNeedsBeginFrame() {
  needs_begin_frame_ = true;
}

void SimCompositor::SetDeferCommits(bool defer_commits) {
  defer_commits_ = defer_commits;
}

void SimCompositor::RegisterSelection(const WebSelection&) {
  has_selection_ = true;
}

void SimCompositor::ClearSelection() {
  has_selection_ = false;
}

SimCanvas::Commands SimCompositor::BeginFrame(double time_delta_in_seconds) {
  DCHECK(web_view_);
  DCHECK(!defer_commits_);
  DCHECK(needs_begin_frame_);
  DCHECK_GT(time_delta_in_seconds, 0);
  needs_begin_frame_ = false;

  last_frame_time_ += base::TimeDelta::FromSecondsD(time_delta_in_seconds);

  web_view_->BeginFrame(last_frame_time_);
  web_view_->UpdateAllLifecyclePhases();

  return PaintFrame();
}

SimCanvas::Commands SimCompositor::PaintFrame() {
  DCHECK(web_view_);

  auto* frame = web_view_->MainFrameImpl()->GetFrame();
  DocumentLifecycle::AllowThrottlingScope throttling_scope(
      frame->GetDocument()->Lifecycle());
  PaintRecordBuilder builder;
  auto infinite_rect = LayoutRect::InfiniteIntRect();
  frame->View()->Paint(builder.Context(), kGlobalPaintFlattenCompositingLayers,
                       CullRect(infinite_rect));

  SimCanvas canvas(infinite_rect.Width(), infinite_rect.Height());
  builder.EndRecording()->Playback(&canvas);
  return canvas.GetCommands();
}

}  // namespace blink
