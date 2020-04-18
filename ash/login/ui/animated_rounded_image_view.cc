// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/animated_rounded_image_view.h"

#include "skia/ext/image_operations.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/skia_util.h"

namespace ash {

AnimatedRoundedImageView::AnimatedRoundedImageView(const gfx::Size& size,
                                                   int corner_radius)
    : image_size_(size), corner_radius_(corner_radius) {}

AnimatedRoundedImageView::~AnimatedRoundedImageView() = default;

void AnimatedRoundedImageView::SetAnimation(const AnimationFrames& animation) {
  frames_.clear();
  frames_.reserve(animation.size());
  for (AnimationFrame frame : animation) {
    // Try to get the best image quality for the animation.
    frame.image = gfx::ImageSkiaOperations::CreateResizedImage(
        frame.image, skia::ImageOperations::RESIZE_BEST, image_size_);
    DCHECK(frame.image.bitmap()->isImmutable());
    frames_.emplace_back(frame);
  }

  StartOrStopAnimation();
}

void AnimatedRoundedImageView::SetImage(const gfx::ImageSkia& image) {
  AnimationFrame frame;
  frame.image = image;
  SetAnimation({frame});
}

void AnimatedRoundedImageView::SetAnimationEnabled(bool enabled) {
  should_animate_ = enabled;
  StartOrStopAnimation();
}

gfx::Size AnimatedRoundedImageView::CalculatePreferredSize() const {
  return gfx::Size(image_size_.width() + GetInsets().width(),
                   image_size_.height() + GetInsets().height());
}

void AnimatedRoundedImageView::OnPaint(gfx::Canvas* canvas) {
  if (frames_.empty())
    return;

  View::OnPaint(canvas);
  gfx::Rect image_bounds(GetContentsBounds());
  image_bounds.ClampToCenteredSize(GetPreferredSize());
  const SkScalar kRadius[8] = {
      SkIntToScalar(corner_radius_), SkIntToScalar(corner_radius_),
      SkIntToScalar(corner_radius_), SkIntToScalar(corner_radius_),
      SkIntToScalar(corner_radius_), SkIntToScalar(corner_radius_),
      SkIntToScalar(corner_radius_), SkIntToScalar(corner_radius_)};
  SkPath path;
  path.addRoundRect(gfx::RectToSkRect(image_bounds), kRadius);
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  canvas->DrawImageInPath(frames_[active_frame_].image, image_bounds.x(),
                          image_bounds.y(), path, flags);
}

void AnimatedRoundedImageView::StartOrStopAnimation() {
  // If animation is disabled or if there are less than 2 frames, show a static
  // image.
  if (!should_animate_ || frames_.size() < 2) {
    active_frame_ = 0;
    update_frame_timer_.Stop();
    SchedulePaint();
    return;
  }

  // Start animation.
  active_frame_ = -1;
  UpdateAnimationFrame();
}

void AnimatedRoundedImageView::UpdateAnimationFrame() {
  DCHECK(!frames_.empty());

  // Note: |active_frame_| may be invalid.
  active_frame_ = (active_frame_ + 1) % frames_.size();
  SchedulePaint();

  // Schedule next frame update.
  update_frame_timer_.Start(
      FROM_HERE, frames_[active_frame_].duration,
      base::BindRepeating(&AnimatedRoundedImageView::UpdateAnimationFrame,
                          base::Unretained(this)));
}

}  // namespace ash
