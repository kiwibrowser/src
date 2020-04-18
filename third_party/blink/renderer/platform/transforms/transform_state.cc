/*
 * Copyright (C) 2011 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/transforms/transform_state.h"

namespace blink {

TransformState& TransformState::operator=(const TransformState& other) {
  accumulated_offset_ = other.accumulated_offset_;
  map_point_ = other.map_point_;
  map_quad_ = other.map_quad_;
  if (map_point_)
    last_planar_point_ = other.last_planar_point_;
  if (map_quad_)
    last_planar_quad_ = other.last_planar_quad_;
  accumulating_transform_ = other.accumulating_transform_;
  force_accumulating_transform_ = other.force_accumulating_transform_;
  direction_ = other.direction_;

  accumulated_transform_.reset();

  if (other.accumulated_transform_)
    accumulated_transform_ =
        TransformationMatrix::Create(*other.accumulated_transform_);

  return *this;
}

void TransformState::TranslateTransform(const LayoutSize& offset) {
  if (direction_ == kApplyTransformDirection) {
    accumulated_transform_->PostTranslate(offset.Width().ToDouble(),
                                          offset.Height().ToDouble());
  } else {
    accumulated_transform_->Translate(offset.Width().ToDouble(),
                                      offset.Height().ToDouble());
  }
}

void TransformState::TranslateMappedCoordinates(const LayoutSize& offset) {
  FloatSize adjusted_offset((direction_ == kApplyTransformDirection) ? offset
                                                                     : -offset);
  if (map_point_)
    last_planar_point_.Move(adjusted_offset);
  if (map_quad_)
    last_planar_quad_.Move(adjusted_offset);
}

void TransformState::Move(const LayoutSize& offset,
                          TransformAccumulation accumulate) {
  if (force_accumulating_transform_)
    accumulate = kAccumulateTransform;

  if (accumulate == kFlattenTransform || !accumulated_transform_) {
    accumulated_offset_ += offset;
  } else {
    ApplyAccumulatedOffset();
    if (accumulating_transform_ && accumulated_transform_) {
      // If we're accumulating into an existing transform, apply the
      // translation.
      TranslateTransform(offset);
    } else {
      // Just move the point and/or quad.
      TranslateMappedCoordinates(offset);
    }
  }
  accumulating_transform_ = accumulate == kAccumulateTransform;
}

void TransformState::ApplyAccumulatedOffset() {
  LayoutSize offset = accumulated_offset_;
  accumulated_offset_ = LayoutSize();
  if (!offset.IsZero()) {
    if (accumulated_transform_) {
      TranslateTransform(offset);
      Flatten();
    } else {
      TranslateMappedCoordinates(offset);
    }
  }
}

// FIXME: We transform AffineTransform to TransformationMatrix. This is rather
// inefficient.
void TransformState::ApplyTransform(
    const AffineTransform& transform_from_container,
    TransformAccumulation accumulate,
    bool* was_clamped) {
  ApplyTransform(transform_from_container.ToTransformationMatrix(), accumulate,
                 was_clamped);
}

void TransformState::ApplyTransform(
    const TransformationMatrix& transform_from_container,
    TransformAccumulation accumulate,
    bool* was_clamped) {
  if (was_clamped)
    *was_clamped = false;

  if (transform_from_container.IsIntegerTranslation()) {
    Move(LayoutSize(LayoutUnit(transform_from_container.E()),
                    LayoutUnit(transform_from_container.F())),
         accumulate);
    return;
  }

  ApplyAccumulatedOffset();

  // If we have an accumulated transform from last time, multiply in this
  // transform
  if (accumulated_transform_) {
    if (direction_ == kApplyTransformDirection)
      accumulated_transform_ = TransformationMatrix::Create(
          transform_from_container * *accumulated_transform_);
    else
      accumulated_transform_->Multiply(transform_from_container);
  } else if (accumulate == kAccumulateTransform) {
    // Make one if we started to accumulate
    accumulated_transform_ =
        TransformationMatrix::Create(transform_from_container);
  }

  if (accumulate == kFlattenTransform) {
    if (force_accumulating_transform_) {
      accumulated_transform_->FlattenTo2d();
    } else {
      const TransformationMatrix* final_transform =
          accumulated_transform_ ? accumulated_transform_.get()
                                 : &transform_from_container;
      FlattenWithTransform(*final_transform, was_clamped);
    }
  }
  accumulating_transform_ =
      accumulate == kAccumulateTransform || force_accumulating_transform_;
}

void TransformState::Flatten(bool* was_clamped) {
  DCHECK(!force_accumulating_transform_);
  if (was_clamped)
    *was_clamped = false;

  ApplyAccumulatedOffset();

  if (!accumulated_transform_) {
    accumulating_transform_ = false;
    return;
  }

  FlattenWithTransform(*accumulated_transform_, was_clamped);
}

FloatPoint TransformState::MappedPoint(bool* was_clamped) const {
  if (was_clamped)
    *was_clamped = false;

  FloatPoint point = last_planar_point_;
  point.Move((direction_ == kApplyTransformDirection) ? accumulated_offset_
                                                      : -accumulated_offset_);
  if (!accumulated_transform_)
    return point;

  if (direction_ == kApplyTransformDirection)
    return accumulated_transform_->MapPoint(point);

  return accumulated_transform_->Inverse().ProjectPoint(point, was_clamped);
}

FloatQuad TransformState::MappedQuad(bool* was_clamped) const {
  if (was_clamped)
    *was_clamped = false;

  FloatQuad quad = last_planar_quad_;
  quad.Move(FloatSize((direction_ == kApplyTransformDirection)
                          ? accumulated_offset_
                          : -accumulated_offset_));
  if (!accumulated_transform_)
    return quad;

  if (direction_ == kApplyTransformDirection)
    return accumulated_transform_->MapQuad(quad);

  return accumulated_transform_->Inverse().ProjectQuad(quad, was_clamped);
}

const TransformationMatrix& TransformState::AccumulatedTransform() const {
  DCHECK(force_accumulating_transform_);
  DCHECK(accumulating_transform_);
  return *accumulated_transform_;
}

void TransformState::FlattenWithTransform(const TransformationMatrix& t,
                                          bool* was_clamped) {
  if (direction_ == kApplyTransformDirection) {
    if (map_point_)
      last_planar_point_ = t.MapPoint(last_planar_point_);
    if (map_quad_)
      last_planar_quad_ = t.MapQuad(last_planar_quad_);
  } else {
    TransformationMatrix inverse_transform = t.Inverse();
    if (map_point_)
      last_planar_point_ = inverse_transform.ProjectPoint(last_planar_point_);
    if (map_quad_)
      last_planar_quad_ =
          inverse_transform.ProjectQuad(last_planar_quad_, was_clamped);
  }

  // We could throw away m_accumulatedTransform if we wanted to here, but that
  // would cause thrash when traversing hierarchies with alternating
  // preserve-3d and flat elements.
  if (accumulated_transform_)
    accumulated_transform_->MakeIdentity();
  accumulating_transform_ = false;
}

}  // namespace blink
