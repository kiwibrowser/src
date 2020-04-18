// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_IMAGEBITMAP_IMAGE_BITMAP_RENDERING_CONTEXT_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_IMAGEBITMAP_IMAGE_BITMAP_RENDERING_CONTEXT_BASE_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context_factory.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/geometry/float_point.h"

namespace cc {
class Layer;
}

namespace blink {

class ImageBitmap;
class ImageLayerBridge;

class MODULES_EXPORT ImageBitmapRenderingContextBase
    : public CanvasRenderingContext {
 public:
  ImageBitmapRenderingContextBase(CanvasRenderingContextHost*,
                                  const CanvasContextCreationAttributesCore&);
  ~ImageBitmapRenderingContextBase() override;

  void Trace(blink::Visitor*) override;

  HTMLCanvasElement* canvas() {
    DCHECK(!Host() || !Host()->IsOffscreenCanvas());
    return static_cast<HTMLCanvasElement*>(Host());
  }

  void SetIsHidden(bool) override {}
  bool isContextLost() const override { return false; }
  void SetImage(ImageBitmap*);
  scoped_refptr<StaticBitmapImage> GetImage(AccelerationHint) const final;
  void SetUV(const FloatPoint& left_top, const FloatPoint& right_bottom);
  bool IsComposited() const final { return true; }
  bool IsAccelerated() const final;

  cc::Layer* CcLayer() const final;
  // TODO(junov): handle lost contexts when content is GPU-backed
  void LoseContext(LostContextMode) override {}

  void Stop() override;

  bool IsPaintable() const final;

 protected:
  Member<ImageLayerBridge> image_layer_bridge_;
};

}  // namespace blink

#endif
