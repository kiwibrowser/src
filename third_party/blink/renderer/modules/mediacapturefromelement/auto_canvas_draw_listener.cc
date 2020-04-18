// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/mediacapturefromelement/auto_canvas_draw_listener.h"

#include <memory>

namespace blink {

AutoCanvasDrawListener::AutoCanvasDrawListener(
    std::unique_ptr<WebCanvasCaptureHandler> handler)
    : CanvasDrawListener(std::move(handler)) {}

// static
AutoCanvasDrawListener* AutoCanvasDrawListener::Create(
    std::unique_ptr<WebCanvasCaptureHandler> handler) {
  return new AutoCanvasDrawListener(std::move(handler));
}

}  // namespace blink
