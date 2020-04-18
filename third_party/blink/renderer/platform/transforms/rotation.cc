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

#include "third_party/blink/renderer/platform/transforms/rotation.h"

#include "third_party/blink/renderer/platform/animation/animation_utilities.h"
#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"

namespace blink {

namespace {

const double kAngleEpsilon = 1e-4;

Rotation ExtractFromMatrix(const TransformationMatrix& matrix,
                           const Rotation& fallback_value) {
  TransformationMatrix::DecomposedType decomp;
  if (!matrix.Decompose(decomp))
    return fallback_value;
  double x = -decomp.quaternion_x;
  double y = -decomp.quaternion_y;
  double z = -decomp.quaternion_z;
  double length = std::sqrt(x * x + y * y + z * z);
  double angle = 0;
  if (length > 0.00001) {
    x /= length;
    y /= length;
    z /= length;
    angle = rad2deg(std::acos(decomp.quaternion_w) * 2);
  } else {
    x = 0;
    y = 0;
    z = 1;
  }
  return Rotation(FloatPoint3D(x, y, z), angle);
}

}  // namespace

bool Rotation::GetCommonAxis(const Rotation& a,
                             const Rotation& b,
                             FloatPoint3D& result_axis,
                             double& result_angle_a,
                             double& result_angle_b) {
  result_axis = FloatPoint3D(0, 0, 1);
  result_angle_a = 0;
  result_angle_b = 0;

  bool is_zero_a = a.axis.IsZero() || fabs(a.angle) < kAngleEpsilon;
  bool is_zero_b = b.axis.IsZero() || fabs(b.angle) < kAngleEpsilon;

  if (is_zero_a && is_zero_b)
    return true;

  if (is_zero_a) {
    result_axis = b.axis;
    result_angle_b = b.angle;
    return true;
  }

  if (is_zero_b) {
    result_axis = a.axis;
    result_angle_a = a.angle;
    return true;
  }

  double dot = a.axis.Dot(b.axis);
  if (dot < 0)
    return false;

  double a_squared = a.axis.LengthSquared();
  double b_squared = b.axis.LengthSquared();
  double error = std::abs(1 - (dot * dot) / (a_squared * b_squared));
  if (error > kAngleEpsilon)
    return false;

  result_axis = a.axis;
  result_angle_a = a.angle;
  result_angle_b = b.angle;
  return true;
}

Rotation Rotation::Slerp(const Rotation& from,
                         const Rotation& to,
                         double progress) {
  double from_angle;
  double to_angle;
  FloatPoint3D axis;
  if (GetCommonAxis(from, to, axis, from_angle, to_angle))
    return Rotation(axis, blink::Blend(from_angle, to_angle, progress));

  TransformationMatrix from_matrix;
  TransformationMatrix to_matrix;
  from_matrix.Rotate3d(from);
  to_matrix.Rotate3d(to);
  to_matrix.Blend(from_matrix, progress);
  return ExtractFromMatrix(to_matrix, progress < 0.5 ? from : to);
}

Rotation Rotation::Add(const Rotation& a, const Rotation& b) {
  double angle_a;
  double angle_b;
  FloatPoint3D axis;
  if (GetCommonAxis(a, b, axis, angle_a, angle_b))
    return Rotation(axis, angle_a + angle_b);

  TransformationMatrix matrix;
  matrix.Rotate3d(a);
  matrix.Rotate3d(b);
  return ExtractFromMatrix(matrix, b);
}

}  // namespace blink
