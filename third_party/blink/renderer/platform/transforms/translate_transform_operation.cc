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

#include "third_party/blink/renderer/platform/transforms/translate_transform_operation.h"

#include "third_party/blink/renderer/platform/animation/animation_utilities.h"

namespace blink {

scoped_refptr<TransformOperation> TranslateTransformOperation::Blend(
    const TransformOperation* from,
    double progress,
    bool blend_to_identity) {
  if (from && !from->CanBlendWith(*this))
    return this;

  const Length zero_length(0, kFixed);
  if (blend_to_identity) {
    return TranslateTransformOperation::Create(
        zero_length.Blend(x_, progress, kValueRangeAll),
        zero_length.Blend(y_, progress, kValueRangeAll),
        blink::Blend(z_, 0., progress), type_);
  }

  const auto* from_op = ToTranslateTransformOperation(from);
  const Length& from_x = from_op ? from_op->x_ : zero_length;
  const Length& from_y = from_op ? from_op->y_ : zero_length;
  double from_z = from_op ? from_op->z_ : 0;
  return TranslateTransformOperation::Create(
      x_.Blend(from_x, progress, kValueRangeAll),
      y_.Blend(from_y, progress, kValueRangeAll),
      blink::Blend(from_z, z_, progress), type_);
}

bool TranslateTransformOperation::CanBlendWith(
    const TransformOperation& other) const {
  return other.GetType() == kTranslate || other.GetType() == kTranslateX ||
         other.GetType() == kTranslateY || other.GetType() == kTranslateZ ||
         other.GetType() == kTranslate3D;
}

scoped_refptr<TranslateTransformOperation>
TranslateTransformOperation::ZoomTranslate(double factor) {
  return Create(x_.Zoom(factor), y_.Zoom(factor), z_ * factor, type_);
}

}  // namespace blink
