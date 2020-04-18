/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 *
 */

#include "third_party/blink/renderer/platform/transforms/rotate_transform_operation.h"

#include "third_party/blink/renderer/platform/animation/animation_utilities.h"

namespace blink {

bool RotateTransformOperation::operator==(
    const TransformOperation& other) const {
  if (!IsSameType(other))
    return false;
  const Rotation& other_rotation = ToRotateTransformOperation(other).rotation_;
  return rotation_.axis == other_rotation.axis &&
         rotation_.angle == other_rotation.angle;
}

bool RotateTransformOperation::GetCommonAxis(const RotateTransformOperation* a,
                                             const RotateTransformOperation* b,
                                             FloatPoint3D& result_axis,
                                             double& result_angle_a,
                                             double& result_angle_b) {
  return Rotation::GetCommonAxis(a ? a->rotation_ : Rotation(),
                                 b ? b->rotation_ : Rotation(), result_axis,
                                 result_angle_a, result_angle_b);
}

scoped_refptr<TransformOperation> RotateTransformOperation::Blend(
    const TransformOperation* from,
    double progress,
    bool blend_to_identity) {
  if (from && !from->IsSameType(*this))
    return this;

  if (blend_to_identity)
    return RotateTransformOperation::Create(
        Rotation(Axis(), Angle() * (1 - progress)), type_);

  // Optimize for single axis rotation
  if (!from)
    return RotateTransformOperation::Create(
        Rotation(Axis(), Angle() * progress), type_);

  const RotateTransformOperation& from_rotate =
      ToRotateTransformOperation(*from);
  if (GetType() == kRotate3D) {
    return RotateTransformOperation::Create(
        Rotation::Slerp(from_rotate.rotation_, rotation_, progress), kRotate3D);
  }

  DCHECK(Axis() == from_rotate.Axis());
  return RotateTransformOperation::Create(
      Rotation(Axis(), blink::Blend(from_rotate.Angle(), Angle(), progress)),
      type_);
}

bool RotateTransformOperation::CanBlendWith(
    const TransformOperation& other) const {
  return other.IsSameType(*this);
}

RotateAroundOriginTransformOperation::RotateAroundOriginTransformOperation(
    double angle,
    double origin_x,
    double origin_y)
    : RotateTransformOperation(Rotation(FloatPoint3D(0, 0, 1), angle),
                               kRotateAroundOrigin),
      origin_x_(origin_x),
      origin_y_(origin_y) {}

void RotateAroundOriginTransformOperation::Apply(
    TransformationMatrix& transform,
    const FloatSize& box_size) const {
  transform.Translate(origin_x_, origin_y_);
  RotateTransformOperation::Apply(transform, box_size);
  transform.Translate(-origin_x_, -origin_y_);
}

bool RotateAroundOriginTransformOperation::operator==(
    const TransformOperation& other) const {
  if (!IsSameType(other))
    return false;
  const RotateAroundOriginTransformOperation& other_rotate =
      ToRotateAroundOriginTransformOperation(other);
  const Rotation& other_rotation = other_rotate.rotation_;
  return rotation_.axis == other_rotation.axis &&
         rotation_.angle == other_rotation.angle &&
         origin_x_ == other_rotate.origin_x_ &&
         origin_y_ == other_rotate.origin_y_;
}

scoped_refptr<TransformOperation> RotateAroundOriginTransformOperation::Blend(
    const TransformOperation* from,
    double progress,
    bool blend_to_identity) {
  if (from && !from->IsSameType(*this))
    return this;
  if (blend_to_identity) {
    return RotateAroundOriginTransformOperation::Create(
        Angle() * (1 - progress), origin_x_, origin_y_);
  }
  if (!from) {
    return RotateAroundOriginTransformOperation::Create(Angle() * progress,
                                                        origin_x_, origin_y_);
  }
  const RotateAroundOriginTransformOperation& from_rotate =
      ToRotateAroundOriginTransformOperation(*from);
  return RotateAroundOriginTransformOperation::Create(
      blink::Blend(from_rotate.Angle(), Angle(), progress),
      blink::Blend(from_rotate.origin_x_, origin_x_, progress),
      blink::Blend(from_rotate.origin_y_, origin_y_, progress));
}

scoped_refptr<TransformOperation> RotateAroundOriginTransformOperation::Zoom(
    double factor) {
  return Create(Angle(), origin_x_ * factor, origin_y_ * factor);
}

}  // namespace blink
