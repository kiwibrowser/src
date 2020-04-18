// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/transform_operations.h"

#include <stddef.h>

#include <algorithm>

#include "cc/base/math_util.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/geometry/angle_conversions.h"
#include "ui/gfx/geometry/box_f.h"
#include "ui/gfx/geometry/vector3d_f.h"
#include "ui/gfx/transform_util.h"

namespace cc {

TransformOperations::TransformOperations()
    : decomposed_transform_dirty_(true) {
}

TransformOperations::TransformOperations(const TransformOperations& other) {
  operations_ = other.operations_;
  decomposed_transform_dirty_ = other.decomposed_transform_dirty_;
  if (!decomposed_transform_dirty_) {
    decomposed_transform_.reset(
        new gfx::DecomposedTransform(*other.decomposed_transform_.get()));
  }
}

TransformOperations::~TransformOperations() = default;

TransformOperations& TransformOperations::operator=(
    const TransformOperations& other) {
  operations_ = other.operations_;
  decomposed_transform_dirty_ = other.decomposed_transform_dirty_;
  if (!decomposed_transform_dirty_) {
    decomposed_transform_.reset(
        new gfx::DecomposedTransform(*other.decomposed_transform_.get()));
  }
  return *this;
}

gfx::Transform TransformOperations::Apply() const {
  gfx::Transform to_return;
  for (auto& operation : operations_)
    to_return.PreconcatTransform(operation.matrix);
  return to_return;
}

TransformOperations TransformOperations::Blend(const TransformOperations& from,
                                               SkMScalar progress) const {
  TransformOperations to_return;
  if (!BlendInternal(from, progress, &to_return)) {
    // If the matrices cannot be blended, fallback to discrete animation logic.
    // See https://drafts.csswg.org/css-transforms/#matrix-interpolation
    to_return = progress < 0.5 ? from : *this;
  }
  return to_return;
}

bool TransformOperations::BlendedBoundsForBox(const gfx::BoxF& box,
                                              const TransformOperations& from,
                                              SkMScalar min_progress,
                                              SkMScalar max_progress,
                                              gfx::BoxF* bounds) const {
  *bounds = box;

  bool from_identity = from.IsIdentity();
  bool to_identity = IsIdentity();
  if (from_identity && to_identity)
    return true;

  if (!MatchesTypes(from))
    return false;

  size_t num_operations = std::max(from_identity ? 0 : from.operations_.size(),
                                   to_identity ? 0 : operations_.size());

  // Because we are squashing all of the matrices together when applying
  // them to the animation, we must apply them in reverse order when
  // not squashing them.
  for (size_t i = 0; i < num_operations; ++i) {
    size_t operation_index = num_operations - 1 - i;
    gfx::BoxF bounds_for_operation;
    const TransformOperation* from_op =
        from_identity ? nullptr : &from.operations_[operation_index];
    const TransformOperation* to_op =
        to_identity ? nullptr : &operations_[operation_index];
    if (!TransformOperation::BlendedBoundsForBox(*bounds, from_op, to_op,
                                                 min_progress, max_progress,
                                                 &bounds_for_operation)) {
      return false;
    }
    *bounds = bounds_for_operation;
  }

  return true;
}

bool TransformOperations::PreservesAxisAlignment() const {
  for (auto& operation : operations_) {
    switch (operation.type) {
      case TransformOperation::TRANSFORM_OPERATION_IDENTITY:
      case TransformOperation::TRANSFORM_OPERATION_TRANSLATE:
      case TransformOperation::TRANSFORM_OPERATION_SCALE:
        continue;
      case TransformOperation::TRANSFORM_OPERATION_MATRIX:
        if (!operation.matrix.IsIdentity() &&
            !operation.matrix.IsScaleOrTranslation())
          return false;
        continue;
      case TransformOperation::TRANSFORM_OPERATION_ROTATE:
      case TransformOperation::TRANSFORM_OPERATION_SKEW:
      case TransformOperation::TRANSFORM_OPERATION_PERSPECTIVE:
        return false;
    }
  }
  return true;
}

bool TransformOperations::IsTranslation() const {
  for (auto& operation : operations_) {
    switch (operation.type) {
      case TransformOperation::TRANSFORM_OPERATION_IDENTITY:
      case TransformOperation::TRANSFORM_OPERATION_TRANSLATE:
        continue;
      case TransformOperation::TRANSFORM_OPERATION_MATRIX:
        if (!operation.matrix.IsIdentityOrTranslation())
          return false;
        continue;
      case TransformOperation::TRANSFORM_OPERATION_ROTATE:
      case TransformOperation::TRANSFORM_OPERATION_SCALE:
      case TransformOperation::TRANSFORM_OPERATION_SKEW:
      case TransformOperation::TRANSFORM_OPERATION_PERSPECTIVE:
        return false;
    }
  }
  return true;
}

static SkMScalar TanDegrees(double degrees) {
  return SkDoubleToMScalar(std::tan(gfx::DegToRad(degrees)));
}

bool TransformOperations::ScaleComponent(SkMScalar* scale) const {
  SkMScalar operations_scale = 1.f;
  for (auto& operation : operations_) {
    switch (operation.type) {
      case TransformOperation::TRANSFORM_OPERATION_IDENTITY:
      case TransformOperation::TRANSFORM_OPERATION_TRANSLATE:
      case TransformOperation::TRANSFORM_OPERATION_ROTATE:
        continue;
      case TransformOperation::TRANSFORM_OPERATION_MATRIX: {
        if (operation.matrix.HasPerspective())
          return false;
        gfx::Vector2dF scale_components =
            MathUtil::ComputeTransform2dScaleComponents(operation.matrix, 1.f);
        operations_scale *=
            std::max(scale_components.x(), scale_components.y());
        break;
      }
      case TransformOperation::TRANSFORM_OPERATION_SKEW: {
        SkMScalar x_component = TanDegrees(operation.skew.x);
        SkMScalar y_component = TanDegrees(operation.skew.y);
        SkMScalar x_scale = std::sqrt(x_component * x_component + 1);
        SkMScalar y_scale = std::sqrt(y_component * y_component + 1);
        operations_scale *= std::max(x_scale, y_scale);
        break;
      }
      case TransformOperation::TRANSFORM_OPERATION_PERSPECTIVE:
        return false;
      case TransformOperation::TRANSFORM_OPERATION_SCALE:
        operations_scale *= std::max(
            std::abs(operation.scale.x),
            std::max(std::abs(operation.scale.y), std::abs(operation.scale.z)));
    }
  }
  *scale = operations_scale;
  return true;
}

bool TransformOperations::MatchesTypes(const TransformOperations& other) const {
  if (operations_.size() == 0 || other.operations_.size() == 0)
    return true;

  if (operations_.size() != other.operations_.size())
    return false;

  for (size_t i = 0; i < operations_.size(); ++i) {
    if (operations_[i].type != other.operations_[i].type)
      return false;
  }

  return true;
}

bool TransformOperations::CanBlendWith(
    const TransformOperations& other) const {
  TransformOperations dummy;
  return BlendInternal(other, 0.5, &dummy);
}

void TransformOperations::AppendTranslate(SkMScalar x,
                                          SkMScalar y,
                                          SkMScalar z) {
  TransformOperation to_add;
  to_add.matrix.Translate3d(x, y, z);
  to_add.type = TransformOperation::TRANSFORM_OPERATION_TRANSLATE;
  to_add.translate.x = x;
  to_add.translate.y = y;
  to_add.translate.z = z;
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendRotate(SkMScalar x,
                                       SkMScalar y,
                                       SkMScalar z,
                                       SkMScalar degrees) {
  TransformOperation to_add;
  to_add.type = TransformOperation::TRANSFORM_OPERATION_ROTATE;
  to_add.rotate.axis.x = x;
  to_add.rotate.axis.y = y;
  to_add.rotate.axis.z = z;
  to_add.rotate.angle = degrees;
  to_add.Bake();
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendScale(SkMScalar x, SkMScalar y, SkMScalar z) {
  TransformOperation to_add;
  to_add.type = TransformOperation::TRANSFORM_OPERATION_SCALE;
  to_add.scale.x = x;
  to_add.scale.y = y;
  to_add.scale.z = z;
  to_add.Bake();
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendSkew(SkMScalar x, SkMScalar y) {
  TransformOperation to_add;
  to_add.type = TransformOperation::TRANSFORM_OPERATION_SKEW;
  to_add.skew.x = x;
  to_add.skew.y = y;
  to_add.Bake();
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendPerspective(SkMScalar depth) {
  TransformOperation to_add;
  to_add.type = TransformOperation::TRANSFORM_OPERATION_PERSPECTIVE;
  to_add.perspective_depth = depth;
  to_add.Bake();
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendMatrix(const gfx::Transform& matrix) {
  TransformOperation to_add;
  to_add.matrix = matrix;
  to_add.type = TransformOperation::TRANSFORM_OPERATION_MATRIX;
  operations_.push_back(to_add);
  decomposed_transform_dirty_ = true;
}

void TransformOperations::AppendIdentity() {
  operations_.push_back(TransformOperation());
}

void TransformOperations::Append(const TransformOperation& operation) {
  operations_.push_back(operation);
}

bool TransformOperations::IsIdentity() const {
  for (auto& operation : operations_) {
    if (!operation.IsIdentity())
      return false;
  }
  return true;
}

bool TransformOperations::ApproximatelyEqual(const TransformOperations& other,
                                             SkMScalar tolerance) const {
  if (size() != other.size())
    return false;
  for (size_t i = 0; i < operations_.size(); ++i) {
    if (!operations_[i].ApproximatelyEqual(other.operations_[i], tolerance))
      return false;
  }
  return true;
}

bool TransformOperations::BlendInternal(const TransformOperations& from,
                                        SkMScalar progress,
                                        TransformOperations* result) const {
  bool from_identity = from.IsIdentity();
  bool to_identity = IsIdentity();
  if (from_identity && to_identity)
    return true;

  if (MatchesTypes(from)) {
    size_t num_operations =
        std::max(from_identity ? 0 : from.operations_.size(),
                 to_identity ? 0 : operations_.size());
    for (size_t i = 0; i < num_operations; ++i) {
      TransformOperation blended;
      if (!TransformOperation::BlendTransformOperations(
              from_identity ? nullptr : &from.operations_[i],
              to_identity ? nullptr : &operations_[i], progress, &blended)) {
        return false;
      }
      result->Append(blended);
    }
    return true;
  }

  if (!ComputeDecomposedTransform() || !from.ComputeDecomposedTransform())
    return false;

  gfx::DecomposedTransform to_return;
  to_return = gfx::BlendDecomposedTransforms(*decomposed_transform_.get(),
                                             *from.decomposed_transform_.get(),
                                             progress);

  result->AppendMatrix(ComposeTransform(to_return));
  return true;
}

bool TransformOperations::ComputeDecomposedTransform() const {
  if (decomposed_transform_dirty_) {
    if (!decomposed_transform_)
      decomposed_transform_.reset(new gfx::DecomposedTransform());
    gfx::Transform transform = Apply();
    if (!gfx::DecomposeTransform(decomposed_transform_.get(), transform))
      return false;
    decomposed_transform_dirty_ = false;
  }
  return true;
}

}  // namespace cc
