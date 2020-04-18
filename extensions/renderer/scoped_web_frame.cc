// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/scoped_web_frame.h"

#include "third_party/blink/public/mojom/page/page_visibility_state.mojom.h"
#include "third_party/blink/public/web/web_heap.h"

namespace extensions {

ScopedWebFrame::ScopedWebFrame()
    : view_(blink::WebView::Create(/* client = */ nullptr,
                                   blink::mojom::PageVisibilityState::kVisible,
                                   /* opener = */ nullptr)),
      frame_(blink::WebLocalFrame::CreateMainFrame(view_,
                                                   &frame_client_,
                                                   nullptr,
                                                   nullptr)) {}

ScopedWebFrame::~ScopedWebFrame() {
  view_->Close();
  blink::WebHeap::CollectAllGarbageForTesting();
}

}  // namespace extensions
