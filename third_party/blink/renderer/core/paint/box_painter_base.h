// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BOX_PAINTER_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BOX_PAINTER_BASE_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/layout/background_bleed_avoidance.h"
#include "third_party/blink/renderer/core/style/style_image.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect_outsets.h"
#include "third_party/blink/renderer/platform/geometry/layout_size.h"
#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/skia/include/core/SkBlendMode.h"

namespace blink {

class BackgroundImageGeometry;
class ComputedStyle;
class DisplayItemClient;
class Document;
class FillLayer;
class FloatRoundedRect;
class ImageResourceObserver;
class LayoutRect;
class PaintLayer;
struct PaintInfo;

// Base class for box painting. Has no dependencies on the layout tree and thus
// provides functionality and definitions that can be shared between both legacy
// layout and LayoutNG.
class BoxPainterBase {
  STACK_ALLOCATED();

 public:
  BoxPainterBase(const DisplayItemClient& display_item,
                 const Document* document,
                 const ComputedStyle& style,
                 Node* node,
                 LayoutRectOutsets border,
                 LayoutRectOutsets padding,
                 const PaintLayer* paint_layer)
      : display_item_(display_item),
        document_(document),
        style_(style),
        node_(node),
        border_(border),
        padding_(padding),
        paint_layer_(paint_layer) {}

  void PaintFillLayers(const PaintInfo&,
                       const Color&,
                       const FillLayer&,
                       const LayoutRect&,
                       BackgroundImageGeometry&,
                       BackgroundBleedAvoidance = kBackgroundBleedNone,
                       SkBlendMode = SkBlendMode::kSrcOver);

  void PaintFillLayer(const PaintInfo&,
                      const Color&,
                      const FillLayer&,
                      const LayoutRect&,
                      BackgroundBleedAvoidance,
                      BackgroundImageGeometry&,
                      SkBlendMode = SkBlendMode::kSrcOver,
                      const LayoutSize flow_box_size = LayoutSize());

  void PaintMaskImages(const PaintInfo&,
                       const LayoutRect&,
                       const ImageResourceObserver&,
                       BackgroundImageGeometry&,
                       bool include_logical_left_edge,
                       bool include_logical_right_edge);

  static void PaintNormalBoxShadow(const PaintInfo&,
                                   const LayoutRect&,
                                   const ComputedStyle&,
                                   bool include_logical_left_edge = true,
                                   bool include_logical_right_edge = true);

  static void PaintInsetBoxShadowWithBorderRect(
      const PaintInfo&,
      const LayoutRect&,
      const ComputedStyle&,
      bool include_logical_left_edge = true,
      bool include_logical_right_edge = true);

  static void PaintInsetBoxShadowWithInnerRect(const PaintInfo&,
                                               const LayoutRect&,
                                               const ComputedStyle&);

  static void PaintBorder(const ImageResourceObserver&,
                          const Document&,
                          Node*,
                          const PaintInfo&,
                          const LayoutRect&,
                          const ComputedStyle&,
                          BackgroundBleedAvoidance = kBackgroundBleedNone,
                          bool include_logical_left_edge = true,
                          bool include_logical_right_edge = true);

  static bool ShouldForceWhiteBackgroundForPrintEconomy(const Document&,
                                                        const ComputedStyle&);

  typedef Vector<const FillLayer*, 8> FillLayerOcclusionOutputList;
  // Returns true if the result fill layers have non-associative blending or
  // compositing mode.  (i.e. The rendering will be different without creating
  // isolation group by context.saveLayer().) Note that the output list will be
  // in top-bottom order.
  bool CalculateFillLayerOcclusionCulling(
      FillLayerOcclusionOutputList& reversed_paint_list,
      const FillLayer&);

  struct FillLayerInfo {
    STACK_ALLOCATED();

   public:
    FillLayerInfo(const Document&,
                  const ComputedStyle&,
                  bool has_overflow_clip,
                  Color bg_color,
                  const FillLayer&,
                  BackgroundBleedAvoidance,
                  bool include_left_edge,
                  bool include_right_edge);

    // FillLayerInfo is a temporary, stack-allocated container which cannot
    // outlive the StyleImage.  This would normally be a raw pointer, if not for
    // the Oilpan tooling complaints.
    Member<StyleImage> image;
    Color color;

    bool include_left_edge;
    bool include_right_edge;
    bool is_bottom_layer;
    bool is_border_fill;
    bool is_clipped_with_local_scrolling;
    bool is_rounded_fill;
    bool should_paint_image;
    bool should_paint_color;
  };

 protected:
  FloatRoundedRect BackgroundRoundedRectAdjustedForBleedAvoidance(
      const LayoutRect& border_rect,
      const LayoutSize& flow_box_size,
      BackgroundBleedAvoidance,
      bool include_logical_left_edge,
      bool include_logical_right_edge) const;
  FloatRoundedRect RoundedBorderRectForClip(
      const FillLayerInfo&,
      const FillLayer&,
      const LayoutRect&,
      const LayoutSize& flow_box_size,
      BackgroundBleedAvoidance,
      LayoutRectOutsets border_padding_insets) const;

  void PaintFillLayerBackground(GraphicsContext&,
                                const FillLayerInfo&,
                                Image*,
                                SkBlendMode,
                                const BackgroundImageGeometry&,
                                LayoutRect scrolled_paint_rect);
  LayoutRectOutsets BorderOutsets(const FillLayerInfo&) const;
  LayoutRectOutsets PaddingOutsets(const FillLayerInfo&) const;

  void PaintFillLayerTextFillBox(GraphicsContext&,
                                 const FillLayerInfo&,
                                 Image*,
                                 SkBlendMode composite_op,
                                 const BackgroundImageGeometry&,
                                 const LayoutRect&,
                                 const LayoutRect& scrolled_paint_rect);
  virtual void PaintTextClipMask(GraphicsContext&,
                                 const IntRect& mask_rect,
                                 const LayoutPoint& paint_offset) = 0;
  virtual LayoutRect AdjustForScrolledContent(const PaintInfo&,
                                              const FillLayerInfo&,
                                              const LayoutRect&) = 0;
  virtual FillLayerInfo GetFillLayerInfo(const Color&,
                                         const FillLayer&,
                                         BackgroundBleedAvoidance) const = 0;
  virtual FloatRoundedRect GetBackgroundRoundedRect(
      const LayoutRect& border_rect,
      const LayoutSize& flow_box_size,
      bool include_logical_left_edge,
      bool include_logical_right_edge) const;

  static void PaintInsetBoxShadow(const PaintInfo&,
                                  const FloatRoundedRect&,
                                  const ComputedStyle&,
                                  bool include_logical_left_edge = true,
                                  bool include_logical_right_edge = true);

 private:
  const DisplayItemClient& display_item_;
  Member<const Document> document_;
  const ComputedStyle& style_;
  Member<Node> node_;
  LayoutRectOutsets border_;
  LayoutRectOutsets padding_;
  const PaintLayer* paint_layer_;
};

}  // namespace blink

#endif
