// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/image_shadow_animator.h"

#include <stddef.h>

#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/image/image_skia_operations.h"

using gfx::Tween;

namespace {

// Tweens between two ShadowValues.
gfx::ShadowValue LinearShadowValueBetween(double progress,
                                          const gfx::ShadowValue& start,
                                          const gfx::ShadowValue& end) {
  gfx::Vector2d offset(
      Tween::LinearIntValueBetween(progress, start.x(), end.x()),
      Tween::LinearIntValueBetween(progress, start.y(), end.y()));

  // The blur must be clamped to even values in order to avoid offsets being
  // incorrect when there are uneven margins.
  int blur =
      Tween::LinearIntValueBetween(progress, start.blur() / 2, end.blur() / 2) *
      2;

  return gfx::ShadowValue(
      offset, blur,
      Tween::ColorValueBetween(progress, start.color(), end.color()));
}

}  // namespace

namespace app_list {

ImageShadowAnimator::ImageShadowAnimator(Delegate* delegate)
    : delegate_(delegate), animation_(this) {
}

ImageShadowAnimator::~ImageShadowAnimator() {
}

void ImageShadowAnimator::SetOriginalImage(const gfx::ImageSkia& image) {
  original_image_ = image;
  animation_.Reset();
  UpdateShadowImageForProgress(0);
}

void ImageShadowAnimator::SetStartAndEndShadows(
    const gfx::ShadowValues& start_shadow,
    const gfx::ShadowValues& end_shadow) {
  start_shadow_ = start_shadow;
  end_shadow_ = end_shadow;
}

void ImageShadowAnimator::AnimationProgressed(const gfx::Animation* animation) {
  UpdateShadowImageForProgress(animation->GetCurrentValue());
}

gfx::ShadowValues ImageShadowAnimator::GetShadowValuesForProgress(
    double progress) const {
  DCHECK_EQ(start_shadow_.size(), end_shadow_.size())
      << "Only equally sized ShadowValues are supported";

  gfx::ShadowValues shadows;
  for (size_t i = 0; i < start_shadow_.size(); ++i) {
    shadows.push_back(
        LinearShadowValueBetween(progress, start_shadow_[i], end_shadow_[i]));
  }
  return shadows;
}

void ImageShadowAnimator::UpdateShadowImageForProgress(double progress) {
  shadow_image_ = gfx::ImageSkiaOperations::CreateImageWithDropShadow(
      original_image_, GetShadowValuesForProgress(progress));

  if (delegate_)
    delegate_->ImageShadowAnimationProgressed(this);
}

}  // namespace app_list
