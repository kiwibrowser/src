// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIACAPTUREFROMELEMENT_ON_REQUEST_CANVAS_DRAW_LISTENER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIACAPTUREFROMELEMENT_ON_REQUEST_CANVAS_DRAW_LISTENER_H_

#include <memory>
#include "base/memory/weak_ptr.h"
#include "third_party/blink/public/platform/web_canvas_capture_handler.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_draw_listener.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace blink {

class OnRequestCanvasDrawListener final
    : public GarbageCollectedFinalized<OnRequestCanvasDrawListener>,
      public CanvasDrawListener {
  USING_GARBAGE_COLLECTED_MIXIN(OnRequestCanvasDrawListener);

 public:
  ~OnRequestCanvasDrawListener() override;
  static OnRequestCanvasDrawListener* Create(
      std::unique_ptr<WebCanvasCaptureHandler>);
  void SendNewFrame(
      sk_sp<SkImage>,
      base::WeakPtr<WebGraphicsContext3DProviderWrapper>) override;

  void Trace(blink::Visitor* visitor) override {}

 private:
  OnRequestCanvasDrawListener(std::unique_ptr<WebCanvasCaptureHandler>);
};

}  // namespace blink

#endif
