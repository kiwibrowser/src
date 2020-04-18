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

#include "third_party/blink/renderer/platform/transforms/scale_transform_operation.h"

#include "third_party/blink/renderer/platform/animation/animation_utilities.h"

namespace blink {

scoped_refptr<TransformOperation> ScaleTransformOperation::Blend(
    const TransformOperation* from,
    double progress,
    bool blend_to_identity) {
  if (from && !from->CanBlendWith(*this))
    return this;

  if (blend_to_identity)
    return ScaleTransformOperation::Create(
        blink::Blend(x_, 1.0, progress), blink::Blend(y_, 1.0, progress),
        blink::Blend(z_, 1.0, progress), type_);

  const ScaleTransformOperation* from_op =
      static_cast<const ScaleTransformOperation*>(from);
  double from_x = from_op ? from_op->x_ : 1.0;
  double from_y = from_op ? from_op->y_ : 1.0;
  double from_z = from_op ? from_op->z_ : 1.0;
  return ScaleTransformOperation::Create(
      blink::Blend(from_x, x_, progress), blink::Blend(from_y, y_, progress),
      blink::Blend(from_z, z_, progress), type_);
}

bool ScaleTransformOperation::CanBlendWith(
    const TransformOperation& other) const {
  return other.GetType() == kScaleX || other.GetType() == kScaleY ||
         other.GetType() == kScaleZ || other.GetType() == kScale3D ||
         other.GetType() == kScale;
}

}  // namespace blink
