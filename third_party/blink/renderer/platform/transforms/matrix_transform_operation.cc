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

#include "third_party/blink/renderer/platform/transforms/matrix_transform_operation.h"

#include <algorithm>

namespace blink {

scoped_refptr<TransformOperation> MatrixTransformOperation::Blend(
    const TransformOperation* from,
    double progress,
    bool blend_to_identity) {
  if (from && !from->IsSameType(*this))
    return this;

  // convert the TransformOperations into matrices
  FloatSize size;
  TransformationMatrix from_t;
  TransformationMatrix to_t(a_, b_, c_, d_, e_, f_);
  if (from) {
    const MatrixTransformOperation* m =
        static_cast<const MatrixTransformOperation*>(from);
    from_t.SetMatrix(m->a_, m->b_, m->c_, m->d_, m->e_, m->f_);
  }

  if (blend_to_identity)
    std::swap(from_t, to_t);

  to_t.Blend(from_t, progress);
  return MatrixTransformOperation::Create(to_t.A(), to_t.B(), to_t.C(),
                                          to_t.D(), to_t.E(), to_t.F());
}

scoped_refptr<TransformOperation> MatrixTransformOperation::Zoom(
    double factor) {
  return Create(a_, b_, c_, d_, e_ * factor, f_ * factor);
}

}  // namespace blink
