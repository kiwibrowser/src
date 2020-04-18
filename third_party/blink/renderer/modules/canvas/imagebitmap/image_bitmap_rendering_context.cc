// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/canvas/imagebitmap/image_bitmap_rendering_context.h"

#include "third_party/blink/renderer/bindings/modules/v8/rendering_context.h"
#include "third_party/blink/renderer/core/imagebitmap/image_bitmap.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"

namespace blink {

ImageBitmapRenderingContext::ImageBitmapRenderingContext(
    CanvasRenderingContextHost* host,
    const CanvasContextCreationAttributesCore& attrs)
    : ImageBitmapRenderingContextBase(host, attrs) {}

ImageBitmapRenderingContext::~ImageBitmapRenderingContext() = default;

void ImageBitmapRenderingContext::SetCanvasGetContextResult(
    RenderingContext& result) {
  result.SetImageBitmapRenderingContext(this);
}

void ImageBitmapRenderingContext::transferFromImageBitmap(
    ImageBitmap* image_bitmap,
    ExceptionState& exception_state) {
  if (image_bitmap && image_bitmap->IsNeutered()) {
    exception_state.ThrowDOMException(
        kInvalidStateError, "The input ImageBitmap has been detached");
    return;
  }

  SetImage(image_bitmap);
}

CanvasRenderingContext* ImageBitmapRenderingContext::Factory::Create(
    CanvasRenderingContextHost* host,
    const CanvasContextCreationAttributesCore& attrs) {
  return new ImageBitmapRenderingContext(host, attrs);
}

}  // namespace blink
