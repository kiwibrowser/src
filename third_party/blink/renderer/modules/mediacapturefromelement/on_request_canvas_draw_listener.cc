// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/mediacapturefromelement/on_request_canvas_draw_listener.h"

#include "third_party/skia/include/core/SkImage.h"

namespace blink {

OnRequestCanvasDrawListener::OnRequestCanvasDrawListener(
    std::unique_ptr<WebCanvasCaptureHandler> handler)
    : CanvasDrawListener(std::move(handler)) {}

OnRequestCanvasDrawListener::~OnRequestCanvasDrawListener() = default;

// static
OnRequestCanvasDrawListener* OnRequestCanvasDrawListener::Create(
    std::unique_ptr<WebCanvasCaptureHandler> handler) {
  return new OnRequestCanvasDrawListener(std::move(handler));
}

void OnRequestCanvasDrawListener::SendNewFrame(
    sk_sp<SkImage> image,
    base::WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider) {
  frame_capture_requested_ = false;
  CanvasDrawListener::SendNewFrame(image, context_provider);
}

}  // namespace blink
