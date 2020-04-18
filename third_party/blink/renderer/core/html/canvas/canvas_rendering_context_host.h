// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_CANVAS_RENDERING_CONTEXT_HOST_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_CANVAS_RENDERING_CONTEXT_HOST_H_

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/events/event_dispatcher.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/graphics/canvas_resource_host.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace blink {

class CanvasRenderingContext;
class FontSelector;
class StaticBitmapImage;
class KURL;

class CORE_EXPORT CanvasRenderingContextHost : public CanvasResourceHost,
                                               public GarbageCollectedMixin {
 public:
  CanvasRenderingContextHost();

  virtual void DetachContext() = 0;

  virtual void DidDraw(const FloatRect& rect) = 0;
  virtual void DidDraw() = 0;

  virtual void FinalizeFrame() = 0;
  virtual void PushFrame(scoped_refptr<StaticBitmapImage> image,
                         const SkIRect& damage_rect) {
    NOTIMPLEMENTED();
  }
  virtual bool OriginClean() const = 0;
  virtual void SetOriginTainted() = 0;
  virtual const IntSize& Size() const = 0;
  virtual CanvasRenderingContext* RenderingContext() const = 0;

  virtual ExecutionContext* GetTopExecutionContext() const = 0;
  virtual DispatchEventResult HostDispatchEvent(Event*) = 0;
  virtual const KURL& GetExecutionContextUrl() const = 0;

  virtual void DiscardResourceProvider() = 0;

  // If WebGL1 is disabled by enterprise policy or command line switch.
  virtual bool IsWebGL1Enabled() const = 0;
  // If WebGL2 is disabled by enterprise policy or command line switch.
  virtual bool IsWebGL2Enabled() const = 0;
  // If WebGL is temporarily blocked because WebGL contexts were lost one or
  // more times, in particular, via the GL_ARB_robustness extension.
  virtual bool IsWebGLBlocked() const = 0;
  virtual void SetContextCreationWasBlocked() {}

  virtual FontSelector* GetFontSelector() = 0;

  // TODO(fserb): remove this.
  virtual bool IsOffscreenCanvas() const { return false; }

  virtual void Commit(scoped_refptr<StaticBitmapImage> bitmap_image,
                      const SkIRect& damage_rect) {
    NOTIMPLEMENTED();
  }

  bool IsPaintable() const;

  virtual void RegisterContextToDispatch(CanvasRenderingContext*) {}

  // Partial CanvasResourceHost implementation
  void RestoreCanvasMatrixClipStack(PaintCanvas*) const final;

 protected:
  ~CanvasRenderingContextHost() override {}

  scoped_refptr<StaticBitmapImage> CreateTransparentImage(const IntSize&) const;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CANVAS_CANVAS_RENDERING_CONTEXT_HOST_H_
