/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2006 Apple Computer, Inc
 * Copyright (C) 2009 Google, Inc.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_SVG_LAYOUT_SVG_SHAPE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_SVG_LAYOUT_SVG_SHAPE_H_

#include <memory>
#include "base/macros.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_model_object.h"
#include "third_party/blink/renderer/core/layout/svg/svg_marker_data.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/transforms/affine_transform.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class FloatPoint;
class PointerEventsHitRules;
class SVGGeometryElement;

enum ShapeGeometryCodePath {
  kPathGeometry,
  kRectGeometryFastPath,
  kEllipseGeometryFastPath
};

struct LayoutSVGShapeRareData {
  USING_FAST_MALLOC(LayoutSVGShapeRareData);

 public:
  LayoutSVGShapeRareData() = default;
  Path non_scaling_stroke_path_;
  AffineTransform non_scaling_stroke_transform_;
  DISALLOW_COPY_AND_ASSIGN(LayoutSVGShapeRareData);
};

class LayoutSVGShape : public LayoutSVGModelObject {
 public:
  explicit LayoutSVGShape(SVGGeometryElement*);
  ~LayoutSVGShape() override;

  void SetNeedsShapeUpdate() { needs_shape_update_ = true; }
  void SetNeedsBoundariesUpdate() final { needs_boundaries_update_ = true; }
  void SetNeedsTransformUpdate() final { needs_transform_update_ = true; }

  bool NodeAtFloatPointInternal(const HitTestRequest&,
                                const FloatPoint&,
                                PointerEventsHitRules);

  Path& GetPath() const {
    DCHECK(path_);
    return *path_;
  }
  bool HasPath() const { return path_.get(); }
  float DashScaleFactor() const;

  // This method is sometimes (rarely) called with a null path and crashes. The
  // code managing the path enforces the necessary invariants to ensure a valid
  // path but somehow that fails. The assert and check for hasPath() are
  // intended to detect and prevent crashes.
  virtual bool IsShapeEmpty() const {
    DCHECK(path_);
    return !HasPath() || GetPath().IsEmpty();
  }

  bool HasNonScalingStroke() const {
    return Style()->SvgStyle().VectorEffect() == VE_NON_SCALING_STROKE;
  }
  const Path& NonScalingStrokePath() const {
    DCHECK(HasNonScalingStroke());
    DCHECK(rare_data_);
    return rare_data_->non_scaling_stroke_path_;
  }
  const AffineTransform& NonScalingStrokeTransform() const {
    DCHECK(HasNonScalingStroke());
    DCHECK(rare_data_);
    return rare_data_->non_scaling_stroke_transform_;
  }

  AffineTransform LocalSVGTransform() const final { return local_transform_; }

  virtual const Vector<MarkerPosition>* MarkerPositions() const {
    return nullptr;
  }

  float StrokeWidth() const;

  virtual ShapeGeometryCodePath GeometryCodePath() const {
    return kPathGeometry;
  }

  FloatRect ObjectBoundingBox() const final { return fill_bounding_box_; }

  const char* GetName() const override { return "LayoutSVGShape"; }

 protected:
  void StyleDidChange(StyleDifference, const ComputedStyle* old_style) override;
  void WillBeDestroyed() override;

  float VisualRectOutsetForRasterEffects() const override;

  void ClearPath() { path_.reset(); }
  void CreatePath();

  virtual void UpdateShapeFromElement();
  // Calculates an inclusive bounding box of this shape as if this shape has
  // a stroke. If this shape has a stroke, then m_strokeBoundingBox is returned;
  // otherwise, estimates a bounding box (not necessarily tight) that would
  // include this shape's stroke bounding box if it had a stroke.
  virtual FloatRect HitTestStrokeBoundingBox() const;
  virtual bool ShapeDependentStrokeContains(const FloatPoint&);
  virtual bool ShapeDependentFillContains(const FloatPoint&,
                                          const WindRule) const;

  // Compute an approximation of the bounding box that this stroke geometry
  // would generate when applied to a shape with the (tight-fitting) bounding
  // box |shape_bbox|.
  FloatRect ApproximateStrokeBoundingBox(const FloatRect& shape_bbox) const;

  FloatRect fill_bounding_box_;
  FloatRect stroke_bounding_box_;
  LayoutSVGShapeRareData& EnsureRareData() const;

 private:
  // Hit-detection separated for the fill and the stroke
  bool FillContains(const FloatPoint&,
                    bool requires_fill = true,
                    const WindRule fill_rule = RULE_NONZERO);
  bool StrokeContains(const FloatPoint&, bool requires_stroke = true);

  bool IsOfType(LayoutObjectType type) const override {
    return type == kLayoutObjectSVGShape ||
           LayoutSVGModelObject::IsOfType(type);
  }
  void UpdateLayout() final;
  void Paint(const PaintInfo&, const LayoutPoint&) const final;
  void AddOutlineRects(Vector<LayoutRect>&,
                       const LayoutPoint& additional_offset,
                       IncludeBlockVisualOverflowOrNot) const final;

  bool NodeAtFloatPoint(HitTestResult&,
                        const FloatPoint& point_in_parent,
                        HitTestAction) final;

  FloatRect StrokeBoundingBox() const final { return stroke_bounding_box_; }
  FloatRect CalculateObjectBoundingBox() const;
  FloatRect CalculateStrokeBoundingBox() const;
  void UpdateNonScalingStrokeData();
  bool UpdateLocalTransform();

 private:
  AffineTransform local_transform_;
  // TODO(fmalita): the Path is now cached in SVGPath; while this additional
  // cache is just a shallow copy, it certainly has a complexity/state
  // management cost (plus allocation & storage overhead) - so we should look
  // into removing it.
  std::unique_ptr<Path> path_;
  mutable std::unique_ptr<LayoutSVGShapeRareData> rare_data_;

  bool needs_boundaries_update_ : 1;
  bool needs_shape_update_ : 1;
  bool needs_transform_update_ : 1;
  bool affected_by_miter_ : 1;
  bool transform_uses_reference_box_ : 1;
};

DEFINE_LAYOUT_OBJECT_TYPE_CASTS(LayoutSVGShape, IsSVGShape());

}  // namespace blink

#endif
