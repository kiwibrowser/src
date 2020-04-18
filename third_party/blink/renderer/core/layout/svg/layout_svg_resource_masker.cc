/*
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "third_party/blink/renderer/core/layout/svg/layout_svg_resource_masker.h"

#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/layout/svg/svg_layout_support.h"
#include "third_party/blink/renderer/core/paint/svg_paint_context.h"
#include "third_party/blink/renderer/core/svg/svg_element.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_record.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_record_builder.h"
#include "third_party/blink/renderer/platform/transforms/affine_transform.h"

namespace blink {

LayoutSVGResourceMasker::LayoutSVGResourceMasker(SVGMaskElement* node)
    : LayoutSVGResourceContainer(node) {}

LayoutSVGResourceMasker::~LayoutSVGResourceMasker() = default;

void LayoutSVGResourceMasker::RemoveAllClientsFromCache(
    bool mark_for_invalidation) {
  cached_paint_record_.reset();
  mask_content_boundaries_ = FloatRect();
  MarkAllClientsForInvalidation(
      mark_for_invalidation ? SVGResourceClient::kLayoutInvalidation |
                                  SVGResourceClient::kBoundariesInvalidation
                            : SVGResourceClient::kParentOnlyInvalidation);
}

sk_sp<const PaintRecord> LayoutSVGResourceMasker::CreatePaintRecord(
    AffineTransform& content_transformation,
    const FloatRect& target_bounding_box,
    GraphicsContext& context) {
  SVGUnitTypes::SVGUnitType content_units = ToSVGMaskElement(GetElement())
                                                ->maskContentUnits()
                                                ->CurrentValue()
                                                ->EnumValue();
  if (content_units == SVGUnitTypes::kSvgUnitTypeObjectboundingbox) {
    content_transformation.Translate(target_bounding_box.X(),
                                     target_bounding_box.Y());
    content_transformation.ScaleNonUniform(target_bounding_box.Width(),
                                           target_bounding_box.Height());
  }

  if (cached_paint_record_)
    return cached_paint_record_;

  SubtreeContentTransformScope content_transform_scope(content_transformation);
  PaintRecordBuilder builder(nullptr, &context);

  ColorFilter mask_content_filter =
      Style()->SvgStyle().ColorInterpolation() == CI_LINEARRGB
          ? kColorFilterSRGBToLinearRGB
          : kColorFilterNone;
  builder.Context().SetColorFilter(mask_content_filter);

  for (const SVGElement& child_element :
       Traversal<SVGElement>::ChildrenOf(*GetElement())) {
    const LayoutObject* layout_object = child_element.GetLayoutObject();
    if (!layout_object ||
        layout_object->StyleRef().Display() == EDisplay::kNone)
      continue;
    SVGPaintContext::PaintResourceSubtree(builder.Context(), layout_object);
  }

  cached_paint_record_ = builder.EndRecording();
  return cached_paint_record_;
}

void LayoutSVGResourceMasker::CalculateMaskContentVisualRect() {
  for (const SVGElement& child_element :
       Traversal<SVGElement>::ChildrenOf(*GetElement())) {
    const LayoutObject* layout_object = child_element.GetLayoutObject();
    if (!layout_object ||
        layout_object->StyleRef().Display() == EDisplay::kNone)
      continue;
    mask_content_boundaries_.Unite(
        layout_object->LocalToSVGParentTransform().MapRect(
            layout_object->VisualRectInLocalSVGCoordinates()));
  }
}

FloatRect LayoutSVGResourceMasker::ResourceBoundingBox(
    const LayoutObject* object) {
  SVGMaskElement* mask_element = ToSVGMaskElement(GetElement());
  DCHECK(mask_element);

  FloatRect object_bounding_box = object->ObjectBoundingBox();
  FloatRect mask_boundaries =
      SVGLengthContext::ResolveRectangle<SVGMaskElement>(
          mask_element, mask_element->maskUnits()->CurrentValue()->EnumValue(),
          object_bounding_box);

  // Resource was not layouted yet. Give back clipping rect of the mask.
  if (SelfNeedsLayout())
    return mask_boundaries;

  if (mask_content_boundaries_.IsEmpty())
    CalculateMaskContentVisualRect();

  FloatRect mask_rect = mask_content_boundaries_;
  if (mask_element->maskContentUnits()->CurrentValue()->Value() ==
      SVGUnitTypes::kSvgUnitTypeObjectboundingbox) {
    AffineTransform transform;
    transform.Translate(object_bounding_box.X(), object_bounding_box.Y());
    transform.ScaleNonUniform(object_bounding_box.Width(),
                              object_bounding_box.Height());
    mask_rect = transform.MapRect(mask_rect);
  }

  mask_rect.Intersect(mask_boundaries);
  return mask_rect;
}

}  // namespace blink
