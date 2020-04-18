/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) 2005, 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2009 Jeff Schiller <codedread@gmail.com>
 * Copyright (C) 2011 Renata Hodovan <reni@webkit.org>
 * Copyright (C) 2011 University of Szeged
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

#include "third_party/blink/renderer/core/layout/svg/layout_svg_shape.h"

#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/layout_analyzer.h"
#include "third_party/blink/renderer/core/layout/pointer_events_hit_rules.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_resource_paint_server.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_root.h"
#include "third_party/blink/renderer/core/layout/svg/svg_layout_support.h"
#include "third_party/blink/renderer/core/layout/svg/svg_resources.h"
#include "third_party/blink/renderer/core/layout/svg/svg_resources_cache.h"
#include "third_party/blink/renderer/core/paint/svg_shape_painter.h"
#include "third_party/blink/renderer/core/svg/svg_geometry_element.h"
#include "third_party/blink/renderer/core/svg/svg_length_context.h"
#include "third_party/blink/renderer/core/svg/svg_path_element.h"
#include "third_party/blink/renderer/platform/geometry/float_point.h"
#include "third_party/blink/renderer/platform/graphics/stroke_data.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"

namespace blink {

LayoutSVGShape::LayoutSVGShape(SVGGeometryElement* node)
    : LayoutSVGModelObject(node),
      // Default is false, the cached rects are empty from the beginning.
      needs_boundaries_update_(false),
      // Default is true, so we grab a Path object once from SVGGeometryElement.
      needs_shape_update_(true),
      // Default is true, so we grab a AffineTransform object once from
      // SVGGeometryElement.
      needs_transform_update_(true),
      // <line> elements have no joins and thus needn't care about miters.
      affected_by_miter_(!IsSVGLineElement(node)),
      // Default to false, since |needs_transform_update_| is true this will be
      // updated the first time transforms are updated.
      transform_uses_reference_box_(false) {}

LayoutSVGShape::~LayoutSVGShape() = default;

void LayoutSVGShape::StyleDidChange(StyleDifference diff,
                                    const ComputedStyle* old_style) {
  LayoutSVGModelObject::StyleDidChange(diff, old_style);
  SVGResources::UpdatePaints(*GetElement(), old_style, StyleRef());
}

void LayoutSVGShape::WillBeDestroyed() {
  SVGResources::ClearPaints(*GetElement(), Style());
  LayoutSVGModelObject::WillBeDestroyed();
}

void LayoutSVGShape::CreatePath() {
  if (!path_)
    path_ = std::make_unique<Path>();
  *path_ = ToSVGGeometryElement(GetElement())->AsPath();
}

float LayoutSVGShape::DashScaleFactor() const {
  if (!StyleRef().SvgStyle().StrokeDashArray()->size())
    return 1;
  return ToSVGGeometryElement(*GetElement()).PathLengthScaleFactor();
}

void LayoutSVGShape::UpdateShapeFromElement() {
  CreatePath();
  fill_bounding_box_ = CalculateObjectBoundingBox();

  if (HasNonScalingStroke()) {
    // NonScalingStrokeTransform may depend on LocalTransform which in turn may
    // depend on ObjectBoundingBox, thus we need to call them in this order.
    UpdateLocalTransform();
    UpdateNonScalingStrokeData();
  }

  stroke_bounding_box_ = CalculateStrokeBoundingBox();
}

namespace {

bool HasMiterJoinStyle(const SVGComputedStyle& svg_style) {
  return svg_style.JoinStyle() == kMiterJoin;
}
bool HasSquareCapStyle(const SVGComputedStyle& svg_style) {
  return svg_style.CapStyle() == kSquareCap;
}

}  // namespace

FloatRect LayoutSVGShape::ApproximateStrokeBoundingBox(
    const FloatRect& shape_bbox) const {
  FloatRect stroke_box = shape_bbox;

  // Implementation of
  // https://drafts.fxtf.org/css-masking/#compute-stroke-bounding-box
  // except that we ignore whether the stroke is none.

  const float stroke_width = StrokeWidth();
  if (stroke_width <= 0)
    return stroke_box;

  const SVGComputedStyle& svg_style = StyleRef().SvgStyle();
  float delta = stroke_width / 2;
  if (affected_by_miter_ && HasMiterJoinStyle(svg_style)) {
    const float miter = svg_style.StrokeMiterLimit();
    if (miter < M_SQRT2 && HasSquareCapStyle(svg_style))
      delta *= M_SQRT2;
    else
      delta *= std::max(miter, 1.0f);
  } else if (HasSquareCapStyle(svg_style)) {
    delta *= M_SQRT2;
  }

  stroke_box.Inflate(delta);
  return stroke_box;
}

FloatRect LayoutSVGShape::HitTestStrokeBoundingBox() const {
  if (Style()->SvgStyle().HasStroke())
    return stroke_bounding_box_;

  // Implementation of
  // https://drafts.fxtf.org/css-masking/#compute-stroke-bounding-box
  // for the <rect> / <ellipse> / <circle> case except that we ignore whether
  // the stroke is none.

  // TODO(fs): Fold this into ApproximateStrokeBoundingBox.
  FloatRect box = fill_bounding_box_;
  const float stroke_width = StrokeWidth();
  box.Inflate(stroke_width / 2);
  return box;
}

bool LayoutSVGShape::ShapeDependentStrokeContains(const FloatPoint& point) {
  // In case the subclass didn't create path during UpdateShapeFromElement()
  // for optimization but still calls this method.
  if (!HasPath())
    CreatePath();

  StrokeData stroke_data;
  SVGLayoutSupport::ApplyStrokeStyleToStrokeData(stroke_data, StyleRef(), *this,
                                                 DashScaleFactor());

  if (HasNonScalingStroke()) {
    // The reason is similar to the above code about HasPath().
    if (!rare_data_)
      UpdateNonScalingStrokeData();
    return NonScalingStrokePath().StrokeContains(
        NonScalingStrokeTransform().MapPoint(point), stroke_data);
  }

  return path_->StrokeContains(point, stroke_data);
}

bool LayoutSVGShape::ShapeDependentFillContains(
    const FloatPoint& point,
    const WindRule fill_rule) const {
  return GetPath().Contains(point, fill_rule);
}

bool LayoutSVGShape::FillContains(const FloatPoint& point,
                                  bool requires_fill,
                                  const WindRule fill_rule) {
  if (!fill_bounding_box_.Contains(point))
    return false;

  if (requires_fill && !SVGPaintServer::ExistsForLayoutObject(*this, StyleRef(),
                                                              kApplyToFillMode))
    return false;

  return ShapeDependentFillContains(point, fill_rule);
}

bool LayoutSVGShape::StrokeContains(const FloatPoint& point,
                                    bool requires_stroke) {
  // "A zero value causes no stroke to be painted."
  if (StyleRef().SvgStyle().StrokeWidth().IsZero())
    return false;

  if (requires_stroke) {
    if (!StrokeBoundingBox().Contains(point))
      return false;

    if (!SVGPaintServer::ExistsForLayoutObject(*this, StyleRef(),
                                               kApplyToStrokeMode))
      return false;
  } else {
    if (!HitTestStrokeBoundingBox().Contains(point))
      return false;
  }

  return ShapeDependentStrokeContains(point);
}

static inline bool TransformOriginIsFixed(const ComputedStyle& style) {
  // If the transform box is view-box and the transform origin is absolute, then
  // is does not depend on the reference box. For fill-box, the origin will
  // always move with the bounding box.
  return style.TransformBox() == ETransformBox::kViewBox &&
         style.TransformOriginX().GetType() == kFixed &&
         style.TransformOriginY().GetType() == kFixed;
}

static inline bool TransformDependsOnReferenceBox(const ComputedStyle& style) {
  // We're passing kExcludeMotionPath here because we're checking that
  // explicitly later.
  if (!TransformOriginIsFixed(style) &&
      style.RequireTransformOrigin(ComputedStyle::kIncludeTransformOrigin,
                                   ComputedStyle::kExcludeMotionPath))
    return true;
  if (style.Transform().DependsOnBoxSize())
    return true;
  if (style.Translate() && style.Translate()->DependsOnBoxSize())
    return true;
  if (style.HasOffset())
    return true;
  return false;
}

bool LayoutSVGShape::UpdateLocalTransform() {
  SVGGraphicsElement* graphics_element = ToSVGGraphicsElement(GetElement());
  if (graphics_element->HasTransform(SVGElement::kIncludeMotionTransform)) {
    local_transform_.SetTransform(graphics_element->CalculateTransform(
        SVGElement::kIncludeMotionTransform));
    return TransformDependsOnReferenceBox(StyleRef());
  }
  local_transform_ = AffineTransform();
  return false;
}

void LayoutSVGShape::UpdateLayout() {
  LayoutAnalyzer::Scope analyzer(*this);

  // Invalidate all resources of this client if our layout changed.
  if (EverHadLayout() && SelfNeedsLayout())
    SVGResourcesCache::ClientLayoutChanged(*this);

  bool update_parent_boundaries = false;
  bool bbox_changed = false;
  // UpdateShapeFromElement() also updates the object & stroke bounds - which
  // feeds into the visual rect - so we need to call it for both the
  // shape-update and the bounds-update flag.
  // We also need to update stroke bounds if HasNonScalingStroke() because the
  // shape may be affected by ancestor transforms.
  if (needs_shape_update_ || needs_boundaries_update_ ||
      HasNonScalingStroke()) {
    FloatRect old_object_bounding_box = ObjectBoundingBox();
    UpdateShapeFromElement();
    if (old_object_bounding_box != ObjectBoundingBox()) {
      GetElement()->SetNeedsResizeObserverUpdate();
      SetShouldDoFullPaintInvalidation();
      bbox_changed = true;
    }
    needs_shape_update_ = false;

    local_visual_rect_ = StrokeBoundingBox();
    SVGLayoutSupport::AdjustVisualRectWithResources(*this, local_visual_rect_);
    needs_boundaries_update_ = false;

    update_parent_boundaries = true;
  }

  // If the transform is relative to the reference box, check relevant
  // conditions to see if we need to recompute the transform.
  if (!needs_transform_update_ && transform_uses_reference_box_) {
    switch (StyleRef().TransformBox()) {
      case ETransformBox::kViewBox:
        needs_transform_update_ =
            SVGLayoutSupport::LayoutSizeOfNearestViewportChanged(this);
        break;
      case ETransformBox::kFillBox:
        needs_transform_update_ = bbox_changed;
        break;
    }
    if (needs_transform_update_)
      SetNeedsPaintPropertyUpdate();
  }

  if (needs_transform_update_) {
    transform_uses_reference_box_ = UpdateLocalTransform();
    needs_transform_update_ = false;
    update_parent_boundaries = true;
  }

  // If our bounds changed, notify the parents.
  if (update_parent_boundaries)
    LayoutSVGModelObject::SetNeedsBoundariesUpdate();

  DCHECK(!needs_shape_update_);
  DCHECK(!needs_boundaries_update_);
  DCHECK(!needs_transform_update_);
  ClearNeedsLayout();
}

void LayoutSVGShape::UpdateNonScalingStrokeData() {
  DCHECK(HasNonScalingStroke());

  // Compute the CTM to the SVG root. This should probably be the CTM all the
  // way to the "canvas" of the page ("host" coordinate system), but with our
  // current approach of applying/painting non-scaling-stroke, that can break in
  // unpleasant ways (see crbug.com/747708 for an example.) Maybe it would be
  // better to apply this effect during rasterization?
  const LayoutSVGRoot* svg_root = SVGLayoutSupport::FindTreeRootObject(this);
  AffineTransform t;
  t.Scale(1 / StyleRef().EffectiveZoom())
      .Multiply(LocalToAncestorTransform(svg_root).ToAffineTransform());
  // Width of non-scaling stroke is independent of translation, so zero it out
  // here.
  t.SetE(0);
  t.SetF(0);

  auto& rare_data = EnsureRareData();
  if (rare_data.non_scaling_stroke_transform_ != t) {
    SetShouldDoFullPaintInvalidation(PaintInvalidationReason::kStyle);
    rare_data.non_scaling_stroke_transform_ = t;
  }

  rare_data.non_scaling_stroke_path_ = *path_;
  rare_data.non_scaling_stroke_path_.Transform(t);
}

void LayoutSVGShape::Paint(const PaintInfo& paint_info,
                           const LayoutPoint&) const {
  SVGShapePainter(*this).Paint(paint_info);
}

// This method is called from inside paintOutline() since we call paintOutline()
// while transformed to our coord system, return local coords
void LayoutSVGShape::AddOutlineRects(Vector<LayoutRect>& rects,
                                     const LayoutPoint&,
                                     IncludeBlockVisualOverflowOrNot) const {
  rects.push_back(LayoutRect(VisualRectInLocalSVGCoordinates()));
}

bool LayoutSVGShape::NodeAtFloatPoint(HitTestResult& result,
                                      const FloatPoint& point_in_parent,
                                      HitTestAction hit_test_action) {
  // We only draw in the foreground phase, so we only hit-test then.
  if (hit_test_action != kHitTestForeground)
    return false;

  FloatPoint local_point;
  if (!SVGLayoutSupport::TransformToUserSpaceAndCheckClipping(
          *this, LocalToSVGParentTransform(), point_in_parent, local_point))
    return false;

  PointerEventsHitRules hit_rules(
      PointerEventsHitRules::SVG_GEOMETRY_HITTESTING,
      result.GetHitTestRequest(), Style()->PointerEvents());
  if (NodeAtFloatPointInternal(result.GetHitTestRequest(), local_point,
                               hit_rules)) {
    const LayoutPoint& local_layout_point = LayoutPoint(local_point);
    UpdateHitTestResult(result, local_layout_point);
    if (result.AddNodeToListBasedTestResult(GetElement(), local_layout_point) ==
        kStopHitTesting)
      return true;
  }

  return false;
}

bool LayoutSVGShape::NodeAtFloatPointInternal(const HitTestRequest& request,
                                              const FloatPoint& local_point,
                                              PointerEventsHitRules hit_rules) {
  const ComputedStyle& style = StyleRef();
  if (hit_rules.require_visible && style.Visibility() != EVisibility::kVisible)
    return false;
  if (hit_rules.can_hit_bounding_box &&
      ObjectBoundingBox().Contains(local_point))
    return true;
  const SVGComputedStyle& svg_style = style.SvgStyle();
  if (hit_rules.can_hit_stroke &&
      (svg_style.HasStroke() || !hit_rules.require_stroke) &&
      StrokeContains(local_point, hit_rules.require_stroke))
    return true;
  WindRule fill_rule = svg_style.FillRule();
  if (request.SvgClipContent())
    fill_rule = svg_style.ClipRule();
  if (hit_rules.can_hit_fill &&
      (svg_style.HasFill() || !hit_rules.require_fill) &&
      FillContains(local_point, hit_rules.require_fill, fill_rule))
    return true;
  return false;
}

FloatRect LayoutSVGShape::CalculateObjectBoundingBox() const {
  return GetPath().BoundingRect();
}

FloatRect LayoutSVGShape::CalculateStrokeBoundingBox() const {
  DCHECK(path_);
  FloatRect stroke_bounding_box = fill_bounding_box_;

  if (Style()->SvgStyle().HasStroke()) {
    StrokeData stroke_data;
    SVGLayoutSupport::ApplyStrokeStyleToStrokeData(stroke_data, StyleRef(),
                                                   *this, DashScaleFactor());
    if (HasNonScalingStroke()) {
      const auto& non_scaling_transform = NonScalingStrokeTransform();
      if (non_scaling_transform.IsInvertible()) {
        const auto& non_scaling_stroke = NonScalingStrokePath();
        FloatRect stroke_bounding_rect =
            non_scaling_stroke.StrokeBoundingRect(stroke_data);
        stroke_bounding_rect =
            non_scaling_transform.Inverse().MapRect(stroke_bounding_rect);
        stroke_bounding_box.Unite(stroke_bounding_rect);
      }
    } else {
      stroke_bounding_box = ApproximateStrokeBoundingBox(stroke_bounding_box);
    }
  }

  return stroke_bounding_box;
}

float LayoutSVGShape::StrokeWidth() const {
  SVGLengthContext length_context(GetElement());
  return length_context.ValueForLength(Style()->SvgStyle().StrokeWidth());
}

LayoutSVGShapeRareData& LayoutSVGShape::EnsureRareData() const {
  if (!rare_data_)
    rare_data_ = std::make_unique<LayoutSVGShapeRareData>();
  return *rare_data_.get();
}

float LayoutSVGShape::VisualRectOutsetForRasterEffects() const {
  // Account for raster expansions due to SVG stroke hairline raster effects.
  if (StyleRef().SvgStyle().HasVisibleStroke()) {
    float outset = 0.5f;
    if (StyleRef().SvgStyle().CapStyle() != kButtCap)
      outset += 0.5f;
    return outset;
  }
  return 0;
}

}  // namespace blink
