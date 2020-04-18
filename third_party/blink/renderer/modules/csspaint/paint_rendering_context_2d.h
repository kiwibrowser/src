// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_CSSPAINT_PAINT_RENDERING_CONTEXT_2D_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_CSSPAINT_PAINT_RENDERING_CONTEXT_2D_H_

#include <memory>
#include "third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.h"
#include "third_party/blink/renderer/modules/csspaint/paint_rendering_context_2d_settings.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_record.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_recorder.h"

namespace blink {

class CanvasImageSource;
class Color;

class MODULES_EXPORT PaintRenderingContext2D : public ScriptWrappable,
                                               public BaseRenderingContext2D {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(PaintRenderingContext2D);
  WTF_MAKE_NONCOPYABLE(PaintRenderingContext2D);

 public:
  static PaintRenderingContext2D* Create(
      const IntSize& container_size,
      const CanvasColorParams& color_params,
      const PaintRenderingContext2DSettings& context_settings,
      float zoom) {
    return new PaintRenderingContext2D(container_size, color_params,
                                       context_settings, zoom);
  }

  void Trace(blink::Visitor* visitor) override {
    ScriptWrappable::Trace(visitor);
    BaseRenderingContext2D::Trace(visitor);
  }

  // PaintRenderingContext2D doesn't have any pixel readback so the origin
  // is always clean, and unable to taint it.
  bool OriginClean() const final { return true; }
  void SetOriginTainted() final {}
  bool WouldTaintOrigin(CanvasImageSource*, ExecutionContext*) final {
    return false;
  }

  int Width() const final;
  int Height() const final;

  bool ParseColorOrCurrentColor(Color&, const String& color_string) const final;

  PaintCanvas* DrawingCanvas() const final;
  PaintCanvas* ExistingDrawingCanvas() const final;
  void DisableDeferral(DisableDeferralReason) final {}

  void DidDraw(const SkIRect&) final;

  void setShadowBlur(double) final;

  bool StateHasFilter() final;
  sk_sp<PaintFilter> StateGetFilter() final;
  void SnapshotStateForFilter() final {}

  void ValidateStateStack() const final;

  bool HasAlpha() const final { return context_settings_.alpha(); }

  // PaintRenderingContext2D cannot lose it's context.
  bool isContextLost() const final { return false; }

  // PaintRenderingContext2D uses a recording canvas, so it should never
  // allocate a pixel buffer and is not accelerated.
  bool CanCreateCanvas2dResourceProvider() const final { return false; }
  bool IsAccelerated() const final { return false; }

  sk_sp<PaintRecord> GetRecord();

 protected:
  bool IsPaint2D() const override { return true; }
  void WillOverwriteCanvas() override;

 private:
  PaintRenderingContext2D(const IntSize& container_size,
                          const CanvasColorParams&,
                          const PaintRenderingContext2DSettings&,
                          float zoom);

  void InitializePaintRecorder();
  PaintCanvas* Canvas() const;

  std::unique_ptr<PaintRecorder> paint_recorder_;
  sk_sp<PaintRecord> previous_frame_;
  IntSize container_size_;
  const CanvasColorParams& color_params_;
  PaintRenderingContext2DSettings context_settings_;
  bool did_record_draw_commands_in_paint_recorder_;
  float effective_zoom_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_CSSPAINT_PAINT_RENDERING_CONTEXT_2D_H_
