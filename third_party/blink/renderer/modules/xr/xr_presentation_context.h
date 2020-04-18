// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_PRESENTATION_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_PRESENTATION_CONTEXT_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context_factory.h"
#include "third_party/blink/renderer/modules/canvas/imagebitmap/image_bitmap_rendering_context_base.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

class MODULES_EXPORT XRPresentationContext final
    : public ImageBitmapRenderingContextBase {
  DEFINE_WRAPPERTYPEINFO();

 public:
  class Factory : public CanvasRenderingContextFactory {
    WTF_MAKE_NONCOPYABLE(Factory);

   public:
    Factory() {}
    ~Factory() override {}

    CanvasRenderingContext* Create(
        CanvasRenderingContextHost*,
        const CanvasContextCreationAttributesCore&) override;
    CanvasRenderingContext::ContextType GetContextType() const override {
      return CanvasRenderingContext::kContextXRPresent;
    }
  };

  // CanvasRenderingContext implementation
  ContextType GetContextType() const override {
    return CanvasRenderingContext::kContextXRPresent;
  }
  void SetCanvasGetContextResult(RenderingContext&) final;

  ~XRPresentationContext() override;

 private:
  XRPresentationContext(CanvasRenderingContextHost*,
                        const CanvasContextCreationAttributesCore&);
};

DEFINE_TYPE_CASTS(XRPresentationContext,
                  CanvasRenderingContext,
                  context,
                  context->GetContextType() ==
                      CanvasRenderingContext::kContextXRPresent,
                  context.GetContextType() ==
                      CanvasRenderingContext::kContextXRPresent);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_PRESENTATION_CONTEXT_H_
