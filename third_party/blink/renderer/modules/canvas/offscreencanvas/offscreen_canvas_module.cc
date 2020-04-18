// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/canvas/offscreencanvas/offscreen_canvas_module.h"

#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/offscreencanvas/offscreen_canvas.h"
#include "third_party/blink/renderer/modules/canvas/htmlcanvas/canvas_context_creation_attributes_helpers.h"
#include "third_party/blink/renderer/modules/canvas/htmlcanvas/canvas_context_creation_attributes_module.h"
#include "third_party/blink/renderer/modules/canvas/offscreencanvas2d/offscreen_canvas_rendering_context_2d.h"

namespace blink {

void OffscreenCanvasModule::getContext(
    ExecutionContext* execution_context,
    OffscreenCanvas& offscreen_canvas,
    const String& id,
    const CanvasContextCreationAttributesModule& attributes,
    ExceptionState& exception_state,
    OffscreenRenderingContext& result) {
  if (offscreen_canvas.IsNeutered()) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "OffscreenCanvas object is detached");
    return;
  }

  // OffscreenCanvas cannot be transferred after getContext, so this execution
  // context will always be the right one from here on.
  CanvasRenderingContext* context = offscreen_canvas.GetCanvasRenderingContext(
      execution_context, id, ToCanvasContextCreationAttributes(attributes));
  if (context)
    context->SetOffscreenCanvasGetContextResult(result);
}

}  // namespace blink
