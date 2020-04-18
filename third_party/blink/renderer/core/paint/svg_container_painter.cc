// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/svg_container_painter.h"

#include "base/optional.h"
#include "third_party/blink/renderer/core/layout/layout_box_model_object.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_container.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_foreign_object.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_viewport_container.h"
#include "third_party/blink/renderer/core/layout/svg/svg_layout_support.h"
#include "third_party/blink/renderer/core/paint/float_clip_recorder.h"
#include "third_party/blink/renderer/core/paint/object_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/svg_foreign_object_painter.h"
#include "third_party/blink/renderer/core/paint/svg_paint_context.h"
#include "third_party/blink/renderer/core/svg/svg_svg_element.h"

namespace blink {

void SVGContainerPainter::Paint(const PaintInfo& paint_info) {
  // Spec: groups w/o children still may render filter content.
  if (!layout_svg_container_.FirstChild() &&
      !layout_svg_container_.SelfWillPaint())
    return;

  FloatRect bounding_box =
      layout_svg_container_.VisualRectInLocalSVGCoordinates();
  // LayoutSVGHiddenContainer's visual rect is always empty but we need to
  // paint its descendants.
  if (!layout_svg_container_.IsSVGHiddenContainer() &&
      !paint_info.GetCullRect().IntersectsCullRect(
          layout_svg_container_.LocalToSVGParentTransform(), bounding_box))
    return;

  // Spec: An empty viewBox on the <svg> element disables rendering.
  DCHECK(layout_svg_container_.GetElement());
  if (IsSVGSVGElement(*layout_svg_container_.GetElement()) &&
      ToSVGSVGElement(*layout_svg_container_.GetElement()).HasEmptyViewBox())
    return;

  PaintInfo paint_info_before_filtering(paint_info);
  paint_info_before_filtering.UpdateCullRect(
      layout_svg_container_.LocalToSVGParentTransform());
  SVGTransformContext transform_context(
      paint_info_before_filtering, layout_svg_container_,
      layout_svg_container_.LocalToSVGParentTransform());
  {
    base::Optional<FloatClipRecorder> clip_recorder;
    base::Optional<ScopedPaintChunkProperties> scoped_paint_chunk_properties;
    if (layout_svg_container_.IsSVGViewportContainer() &&
        SVGLayoutSupport::IsOverflowHidden(layout_svg_container_)) {
      if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
        const auto* fragment =
            paint_info.FragmentToPaint(layout_svg_container_);
        if (!fragment)
          return;
        const auto* properties = fragment->PaintProperties();
        // TODO(crbug.com/814815): The condition should be a DCHECK, but for now
        // we may paint the object for filters during PrePaint before the
        // properties are ready.
        if (properties && properties->OverflowClip()) {
          scoped_paint_chunk_properties.emplace(
              paint_info.context.GetPaintController(),
              properties->OverflowClip(), layout_svg_container_,
              paint_info.DisplayItemTypeForClipping());
        }
      } else {
        FloatRect viewport =
            layout_svg_container_.LocalToSVGParentTransform().Inverse().MapRect(
                ToLayoutSVGViewportContainer(layout_svg_container_).Viewport());
        clip_recorder.emplace(paint_info_before_filtering.context,
                              layout_svg_container_,
                              paint_info_before_filtering.phase, viewport);
      }
    }

    SVGPaintContext paint_context(layout_svg_container_,
                                  paint_info_before_filtering);
    bool continue_rendering = true;
    if (paint_context.GetPaintInfo().phase == PaintPhase::kForeground)
      continue_rendering = paint_context.ApplyClipMaskAndFilterIfNecessary();

    if (continue_rendering) {
      for (LayoutObject* child = layout_svg_container_.FirstChild(); child;
           child = child->NextSibling()) {
        if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
            child->IsSVGForeignObject()) {
          SVGForeignObjectPainter(ToLayoutSVGForeignObject(*child))
              .PaintLayer(paint_context.GetPaintInfo());
        } else {
          child->Paint(paint_context.GetPaintInfo(), IntPoint());
        }
      }
    }
  }

  if (paint_info_before_filtering.phase != PaintPhase::kForeground)
    return;

  if (layout_svg_container_.Style()->OutlineWidth() &&
      layout_svg_container_.Style()->Visibility() == EVisibility::kVisible) {
    PaintInfo outline_paint_info(paint_info_before_filtering);
    outline_paint_info.phase = PaintPhase::kSelfOutlineOnly;
    ObjectPainter(layout_svg_container_)
        .PaintOutline(outline_paint_info, LayoutPoint(bounding_box.Location()));
  }

  if (paint_info_before_filtering.IsPrinting())
    ObjectPainter(layout_svg_container_)
        .AddPDFURLRectIfNeeded(paint_info_before_filtering, LayoutPoint());
}

}  // namespace blink
