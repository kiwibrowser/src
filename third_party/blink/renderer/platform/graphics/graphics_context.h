/*
 * Copyright (C) 2003, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008-2009 Torch Mobile, Inc.
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_GRAPHICS_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_GRAPHICS_CONTEXT_H_

#include <memory>
#include "third_party/blink/renderer/platform/fonts/font.h"
#include "third_party/blink/renderer/platform/graphics/dash_array.h"
#include "third_party/blink/renderer/platform/graphics/draw_looper_builder.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context_state.h"
#include "third_party/blink/renderer/platform/graphics/high_contrast_image_classifier.h"
#include "third_party/blink/renderer/platform/graphics/high_contrast_settings.h"
#include "third_party/blink/renderer/platform/graphics/image_orientation.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_filter.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_record.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_recorder.h"
#include "third_party/blink/renderer/platform/graphics/skia/skia_utils.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/skia/include/core/SkClipOp.h"
#include "third_party/skia/include/core/SkMetaData.h"
#include "third_party/skia/include/core/SkRefCnt.h"

class SkPath;
class SkRRect;
struct SkRect;

namespace blink {

class FloatRect;
class FloatRoundedRect;
class KURL;
class PaintController;
class Path;
struct TextRunPaintInfo;

class PLATFORM_EXPORT GraphicsContext {
  WTF_MAKE_NONCOPYABLE(GraphicsContext);
  USING_FAST_MALLOC(GraphicsContext);

 public:
  enum DisabledMode {
    kNothingDisabled = 0,  // Run as normal.
    kFullyDisabled = 1     // Do absolutely minimal work to remove the cost of
                           // the context from performance tests.
  };

  explicit GraphicsContext(PaintController&,
                           DisabledMode = kNothingDisabled,
                           SkMetaData* = nullptr);

  ~GraphicsContext();

  PaintCanvas* Canvas() { return canvas_; }
  const PaintCanvas* Canvas() const { return canvas_; }

  PaintController& GetPaintController() { return paint_controller_; }
  const PaintController& GetPaintController() const {
    return paint_controller_;
  }

  bool ContextDisabled() const { return disabled_state_; }

  const HighContrastSettings& high_contrast_settings() {
    return high_contrast_settings_;
  }

  // ---------- State management methods -----------------
  void Save();
  void Restore();

#if DCHECK_IS_ON()
  unsigned SaveCount() const;
#endif

  void SetHighContrast(const HighContrastSettings&);

  float StrokeThickness() const {
    return ImmutableState()->GetStrokeData().Thickness();
  }
  void SetStrokeThickness(float thickness) {
    MutableState()->SetStrokeThickness(thickness);
  }

  StrokeStyle GetStrokeStyle() const {
    return ImmutableState()->GetStrokeData().Style();
  }
  void SetStrokeStyle(StrokeStyle style) {
    MutableState()->SetStrokeStyle(style);
  }

  Color StrokeColor() const { return ImmutableState()->StrokeColor(); }
  void SetStrokeColor(const Color& color) {
    MutableState()->SetStrokeColor(color);
  }

  void SetLineCap(LineCap cap) { MutableState()->SetLineCap(cap); }
  void SetLineDash(const DashArray& dashes, float dash_offset) {
    MutableState()->SetLineDash(dashes, dash_offset);
  }
  void SetLineJoin(LineJoin join) { MutableState()->SetLineJoin(join); }
  void SetMiterLimit(float limit) { MutableState()->SetMiterLimit(limit); }

  Color FillColor() const { return ImmutableState()->FillColor(); }
  void SetFillColor(const Color& color) { MutableState()->SetFillColor(color); }

  void SetShouldAntialias(bool antialias) {
    MutableState()->SetShouldAntialias(antialias);
  }
  bool ShouldAntialias() const { return ImmutableState()->ShouldAntialias(); }

  void SetTextDrawingMode(TextDrawingModeFlags mode) {
    MutableState()->SetTextDrawingMode(mode);
  }
  TextDrawingModeFlags TextDrawingMode() const {
    return ImmutableState()->TextDrawingMode();
  }

  void SetImageInterpolationQuality(InterpolationQuality quality) {
    MutableState()->SetInterpolationQuality(quality);
  }
  InterpolationQuality ImageInterpolationQuality() const {
    return ImmutableState()->GetInterpolationQuality();
  }

  // Specify the device scale factor which may change the way document markers
  // and fonts are rendered.
  void SetDeviceScaleFactor(float factor) { device_scale_factor_ = factor; }
  float DeviceScaleFactor() const { return device_scale_factor_; }

  // Returns if the context is a printing context instead of a display
  // context. Bitmap shouldn't be resampled when printing to keep the best
  // possible quality.
  bool Printing() const { return printing_; }
  void SetPrinting(bool printing) { printing_ = printing; }

  SkColorFilter* GetColorFilter() const;
  void SetColorFilter(ColorFilter);
  // ---------- End state management methods -----------------

  // DrawRect() fills and always strokes using a 1-pixel stroke inset from
  // the rect borders (of the pre-set stroke color).
  void DrawRect(const IntRect&);

  // DrawLine() only operates on horizontal or vertical lines and uses the
  // current stroke settings.
  void DrawLine(const IntPoint&, const IntPoint&);

  void FillPath(const Path&);

  // The length parameter is only used when the path has a dashed or dotted
  // stroke style, with the default dash/dot path effect. If a non-zero length
  // is provided the number of dashes/dots on a dashed/dotted
  // line will be adjusted to start and end that length with a dash/dot.
  // The dash_thickness parameter is only used when drawing dashed borders,
  // where the stroke thickness has been set for corner miters but we want the
  // dash length set from the border width.
  void StrokePath(const Path&,
                  const int length = 0,
                  const int dash_thickness = 0);

  void FillEllipse(const FloatRect&);
  void StrokeEllipse(const FloatRect&);

  void FillRect(const FloatRect&);
  void FillRect(const FloatRect&,
                const Color&,
                SkBlendMode = SkBlendMode::kSrcOver);
  void FillRoundedRect(const FloatRoundedRect&, const Color&);
  void FillDRRect(const FloatRoundedRect&,
                  const FloatRoundedRect&,
                  const Color&);

  void StrokeRect(const FloatRect&, float line_width);

  void DrawRecord(sk_sp<const PaintRecord>);
  void CompositeRecord(sk_sp<PaintRecord>,
                       const FloatRect& dest,
                       const FloatRect& src,
                       SkBlendMode);

  void DrawImage(Image*,
                 Image::ImageDecodingMode,
                 const FloatRect& dest_rect,
                 const FloatRect* src_rect = nullptr,
                 SkBlendMode = SkBlendMode::kSrcOver,
                 RespectImageOrientationEnum = kDoNotRespectImageOrientation);
  void DrawImageRRect(
      Image*,
      Image::ImageDecodingMode,
      const FloatRoundedRect& dest,
      const FloatRect& src_rect,
      SkBlendMode = SkBlendMode::kSrcOver,
      RespectImageOrientationEnum = kDoNotRespectImageOrientation);
  void DrawTiledImage(Image*,
                      const FloatRect& dest_rect,
                      const FloatPoint& src_point,
                      const FloatSize& tile_size,
                      SkBlendMode = SkBlendMode::kSrcOver,
                      const FloatSize& repeat_spacing = FloatSize());
  void DrawTiledImage(Image*,
                      const FloatRect& dest_rect,
                      const FloatRect& src_rect,
                      const FloatSize& tile_scale_factor,
                      Image::TileRule h_rule = Image::kStretchTile,
                      Image::TileRule v_rule = Image::kStretchTile,
                      SkBlendMode = SkBlendMode::kSrcOver);

  // These methods write to the canvas.
  // Also drawLine(const IntPoint& point1, const IntPoint& point2) and
  // fillRoundedRect().
  void DrawOval(const SkRect&, const PaintFlags&);
  void DrawPath(const SkPath&, const PaintFlags&);
  void DrawRect(const SkRect&, const PaintFlags&);
  void DrawRRect(const SkRRect&, const PaintFlags&);

  void Clip(const IntRect& rect) { ClipRect(rect); }
  void Clip(const FloatRect& rect) { ClipRect(rect); }
  void ClipRoundedRect(const FloatRoundedRect&,
                       SkClipOp = SkClipOp::kIntersect,
                       AntiAliasingMode = kAntiAliased);
  void ClipOut(const IntRect& rect) {
    ClipRect(rect, kNotAntiAliased, SkClipOp::kDifference);
  }
  void ClipOut(const FloatRect& rect) {
    ClipRect(rect, kNotAntiAliased, SkClipOp::kDifference);
  }
  void ClipOut(const Path&);
  void ClipOutRoundedRect(const FloatRoundedRect&);
  void ClipPath(const SkPath&,
                AntiAliasingMode = kNotAntiAliased,
                SkClipOp = SkClipOp::kIntersect);
  void ClipRect(const SkRect&,
                AntiAliasingMode = kNotAntiAliased,
                SkClipOp = SkClipOp::kIntersect);

  void DrawText(const Font&, const TextRunPaintInfo&, const FloatPoint&);
  void DrawText(const Font&, const NGTextFragmentPaintInfo&, const FloatPoint&);

  void DrawText(const Font&,
                const TextRunPaintInfo&,
                const FloatPoint&,
                const PaintFlags&);
  void DrawText(const Font&,
                const NGTextFragmentPaintInfo&,
                const FloatPoint&,
                const PaintFlags&);

  void DrawEmphasisMarks(const Font&,
                         const TextRunPaintInfo&,
                         const AtomicString& mark,
                         const FloatPoint&);
  void DrawEmphasisMarks(const Font&,
                         const NGTextFragmentPaintInfo&,
                         const AtomicString& mark,
                         const FloatPoint&);

  void DrawBidiText(
      const Font&,
      const TextRunPaintInfo&,
      const FloatPoint&,
      Font::CustomFontNotReadyAction = Font::kDoNotPaintIfFontNotReady);
  void DrawHighlightForText(const Font&,
                            const TextRun&,
                            const FloatPoint&,
                            int h,
                            const Color& background_color,
                            int from = 0,
                            int to = -1);

  void DrawLineForText(const FloatPoint&, float width);

  // beginLayer()/endLayer() behave like save()/restore() for CTM and clip
  // states. Apply SkBlendMode when the layer is composited on the backdrop
  // (i.e. endLayer()).
  void BeginLayer(float opacity = 1.0f,
                  SkBlendMode = SkBlendMode::kSrcOver,
                  const FloatRect* = nullptr,
                  ColorFilter = kColorFilterNone,
                  sk_sp<PaintFilter> = nullptr);
  void EndLayer();

  // Instead of being dispatched to the active canvas, draw commands following
  // beginRecording() are stored in a display list that can be replayed at a
  // later time. Pass in the bounding rectangle for the content in the list.
  void BeginRecording(const FloatRect&);

  // Returns a record with any recorded draw commands since the prerequisite
  // call to beginRecording().  The record is guaranteed to be non-null (but
  // not necessarily non-empty), even when the context is disabled.
  sk_sp<PaintRecord> EndRecording();

  void SetShadow(const FloatSize& offset,
                 float blur,
                 const Color&,
                 DrawLooperBuilder::ShadowTransformMode =
                     DrawLooperBuilder::kShadowRespectsTransforms,
                 DrawLooperBuilder::ShadowAlphaMode =
                     DrawLooperBuilder::kShadowRespectsAlpha,
                 ShadowMode = kDrawShadowAndForeground);

  void SetDrawLooper(sk_sp<SkDrawLooper>);

  void DrawFocusRing(const Vector<IntRect>&,
                     float width,
                     int offset,
                     const Color&);
  void DrawFocusRing(const Path&, float width, int offset, const Color&);

  enum Edge {
    kNoEdge = 0,
    kTopEdge = 1 << 1,
    kRightEdge = 1 << 2,
    kBottomEdge = 1 << 3,
    kLeftEdge = 1 << 4
  };
  typedef unsigned Edges;
  void DrawInnerShadow(const FloatRoundedRect&,
                       const Color& shadow_color,
                       const FloatSize& shadow_offset,
                       float shadow_blur,
                       float shadow_spread,
                       Edges clipped_edges = kNoEdge);

  const PaintFlags& FillFlags() const { return ImmutableState()->FillFlags(); }
  // If the length of the path to be stroked is known, pass it in for correct
  // dash or dot placement. Border painting uses a stroke thickness determined
  // by the corner miters. Set the dash_thickness to a non-zero number for
  // cases where dashes should be based on a different thickness.
  const PaintFlags& StrokeFlags(const int length = 0,
                                const int dash_thickness = 0) const {
    return ImmutableState()->StrokeFlags(length, dash_thickness);
  }

  // ---------- Transformation methods -----------------
  void ConcatCTM(const AffineTransform&);

  void Scale(float x, float y);
  void Rotate(float angle_in_radians);
  void Translate(float x, float y);
  // ---------- End transformation methods -----------------

  SkFilterQuality ComputeFilterQuality(Image*,
                                       const FloatRect& dest,
                                       const FloatRect& src) const;

  // Sets target URL of a clickable area.
  void SetURLForRect(const KURL&, const IntRect&);

  // Sets the destination of a clickable area of a URL fragment (in a URL
  // pointing to the same web page). When the area is clicked, the page should
  // be scrolled to the location set by setURLDestinationLocation() for the
  // destination whose name is |name|.
  void SetURLFragmentForRect(const String& name, const IntRect&);

  // Sets location of a URL destination (a.k.a. anchor) in the page.
  void SetURLDestinationLocation(const String& name, const IntPoint&);

  static void AdjustLineToPixelBoundaries(FloatPoint& p1,
                                          FloatPoint& p2,
                                          float stroke_width);

  static int FocusRingOutsetExtent(int offset, int width);

#if DCHECK_IS_ON()
  void SetInDrawingRecorder(bool);
#endif

  static sk_sp<SkColorFilter> WebCoreColorFilterToSkiaColorFilter(ColorFilter);

 private:
  const GraphicsContextState* ImmutableState() const { return paint_state_; }

  GraphicsContextState* MutableState() {
    RealizePaintSave();
    return paint_state_;
  }

  template <typename TextPaintInfo>
  void DrawTextInternal(const Font&,
                        const TextPaintInfo&,
                        const FloatPoint&,
                        const PaintFlags&);

  template <typename TextPaintInfo>
  void DrawTextInternal(const Font&, const TextPaintInfo&, const FloatPoint&);

  template <typename TextPaintInfo>
  void DrawEmphasisMarksInternal(const Font&,
                                 const TextPaintInfo&,
                                 const AtomicString& mark,
                                 const FloatPoint&);

  template <typename DrawTextFunc>
  void DrawTextPasses(const DrawTextFunc&);

  void SaveLayer(const SkRect* bounds, const PaintFlags*);
  void RestoreLayer();

  // Helpers for drawing a focus ring (drawFocusRing)
  void DrawFocusRingPath(const SkPath&, const Color&, float width);
  void DrawFocusRingRect(const SkRect&, const Color&, float width);

  // SkCanvas wrappers.
  void ClipRRect(const SkRRect&,
                 AntiAliasingMode = kNotAntiAliased,
                 SkClipOp = SkClipOp::kIntersect);
  void Concat(const SkMatrix&);

  // Apply deferred paint state saves
  void RealizePaintSave() {
    if (ContextDisabled())
      return;

    if (paint_state_->SaveCount()) {
      paint_state_->DecrementSaveCount();
      ++paint_state_index_;
      if (paint_state_stack_.size() == paint_state_index_) {
        paint_state_stack_.push_back(
            GraphicsContextState::CreateAndCopy(*paint_state_));
        paint_state_ = paint_state_stack_[paint_state_index_].get();
      } else {
        GraphicsContextState* prior_paint_state = paint_state_;
        paint_state_ = paint_state_stack_[paint_state_index_].get();
        paint_state_->Copy(*prior_paint_state);
      }
    }
  }

  void FillRectWithRoundedHole(const FloatRect&,
                               const FloatRoundedRect& rounded_hole_rect,
                               const Color&);

  const SkMetaData& MetaData() const { return meta_data_; }

  bool ShouldApplyHighContrastFilterToImage(Image&);
  Color ApplyHighContrastFilter(const Color& input) const;
  PaintFlags ApplyHighContrastFilter(const PaintFlags* input) const;

  // null indicates painting is contextDisabled. Never delete this object.
  PaintCanvas* canvas_;

  PaintController& paint_controller_;

  // Paint states stack. The state controls the appearance of drawn content, so
  // this stack enables local drawing state changes with save()/restore() calls.
  // We do not delete from this stack to avoid memory churn.
  Vector<std::unique_ptr<GraphicsContextState>> paint_state_stack_;

  // Current index on the stack. May not be the last thing on the stack.
  unsigned paint_state_index_;

  // Raw pointer to the current state.
  GraphicsContextState* paint_state_;

  PaintRecorder paint_recorder_;

  SkMetaData meta_data_;

#if DCHECK_IS_ON()
  int layer_count_;
  bool disable_destruction_checks_;
  bool in_drawing_recorder_;
#endif

  const DisabledMode disabled_state_;

  float device_scale_factor_;

  HighContrastSettings high_contrast_settings_;
  sk_sp<SkColorFilter> high_contrast_filter_;
  HighContrastImageClassifier high_contrast_image_classifier_;

  unsigned printing_ : 1;
  unsigned has_meta_data_ : 1;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_GRAPHICS_CONTEXT_H_
