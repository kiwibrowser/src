/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/page/page_overlay.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "cc/layers/layer.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/frame/web_frame_widget_base.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/scrolling/scrolling_coordinator.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer_client.h"
#include "third_party/blink/renderer/platform/scroll/main_thread_scrolling_reason.h"

namespace blink {

std::unique_ptr<PageOverlay> PageOverlay::Create(
    WebLocalFrameImpl* frame_impl,
    std::unique_ptr<PageOverlay::Delegate> delegate) {
  return base::WrapUnique(new PageOverlay(frame_impl, std::move(delegate)));
}

PageOverlay::PageOverlay(WebLocalFrameImpl* frame_impl,
                         std::unique_ptr<PageOverlay::Delegate> delegate)
    : frame_impl_(frame_impl), delegate_(std::move(delegate)) {}

PageOverlay::~PageOverlay() {
  if (!layer_)
    return;
  layer_->RemoveFromParent();
  layer_ = nullptr;
}

void PageOverlay::Update() {
  if (!frame_impl_->LocalRootFrameWidget()->IsAcceleratedCompositingActive())
    return;

  LocalFrame* frame = frame_impl_->GetFrame();
  if (!frame)
    return;

  if (!layer_) {
    layer_ = GraphicsLayer::Create(*this);
    layer_->SetDrawsContent(true);

    // This is required for contents of overlay to stay in sync with the page
    // while scrolling.
    cc::Layer* cc_layer = layer_->CcLayer();
    cc_layer->AddMainThreadScrollingReasons(
        MainThreadScrollingReason::kPageOverlay);
    if (frame->IsMainFrame()) {
      frame->GetPage()->GetVisualViewport().ContainerLayer()->AddChild(
          layer_.get());
    } else {
      frame_impl_->LocalRootFrameWidget()->RootGraphicsLayer()->AddChild(
          layer_.get());
    }

    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      layer_->SetLayerState(PropertyTreeState(PropertyTreeState::Root()),
                            IntPoint());
    }
  }

  IntSize size = frame->GetPage()->GetVisualViewport().Size();
  if (size != layer_->Size())
    layer_->SetSize(size);

  if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    layer_->SetNeedsDisplay();
}

LayoutRect PageOverlay::VisualRect() const {
  DCHECK(layer_.get());
  return LayoutRect(FloatPoint(), layer_->Size());
}

IntRect PageOverlay::ComputeInterestRect(const GraphicsLayer* graphics_layer,
                                         const IntRect&) const {
  return IntRect(IntPoint(), ExpandedIntSize(layer_->Size()));
}

void PageOverlay::PaintContents(const GraphicsLayer* graphics_layer,
                                GraphicsContext& gc,
                                GraphicsLayerPaintingPhase phase,
                                const IntRect& interest_rect) const {
  DCHECK(layer_);
  delegate_->PaintPageOverlay(*this, gc, interest_rect.Size());
}

String PageOverlay::DebugName(const GraphicsLayer*) const {
  return "WebViewImpl Page Overlay Content Layer";
}

}  // namespace blink
