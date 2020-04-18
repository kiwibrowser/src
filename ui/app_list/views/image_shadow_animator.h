// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_IMAGE_SHADOW_ANIMATOR_H_
#define UI_APP_LIST_VIEWS_IMAGE_SHADOW_ANIMATOR_H_

#include "base/macros.h"
#include "ui/app_list/app_list_export.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/shadow_value.h"

namespace app_list {

namespace test {
class ImageShadowAnimatorTest;
}  // namespace test

// A helper class that animates an image's shadow between two shadow values.
class APP_LIST_EXPORT ImageShadowAnimator : public gfx::AnimationDelegate {
 public:
  class Delegate {
   public:
    // Called when |shadow_image_| is updated.
    virtual void ImageShadowAnimationProgressed(
        ImageShadowAnimator* animator) = 0;
  };

  explicit ImageShadowAnimator(Delegate* delegate);
  ~ImageShadowAnimator() override;

  // Sets the image that will have its shadow animated. Synchronously calls the
  // Delegate's ImageShadowAnimationProgressed() method.
  void SetOriginalImage(const gfx::ImageSkia& image);

  // Sets the shadow values to animate between.
  void SetStartAndEndShadows(const gfx::ShadowValues& start_shadow,
                             const gfx::ShadowValues& end_shadow);

  gfx::SlideAnimation* animation() { return &animation_; }

  const gfx::ImageSkia& shadow_image() const { return shadow_image_; }

  // Overridden from gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;

 private:
  friend test::ImageShadowAnimatorTest;

  // Returns shadow values for a tween between |start_shadow_| and |end_shadow_|
  // at |progress|.
  gfx::ShadowValues GetShadowValuesForProgress(double progress) const;

  // Updates |shadow_image_| and notifies the delegate.
  void UpdateShadowImageForProgress(double progress);

  Delegate* const delegate_;

  // The image to paint a drop shadow for.
  gfx::ImageSkia original_image_;

  // The image with a drop shadow painted. This stays current with the progress
  // of |animation_|.
  gfx::ImageSkia shadow_image_;

  // The animation that controls the progress of the shadow tween.
  gfx::SlideAnimation animation_;

  // The shadow value to tween from.
  gfx::ShadowValues start_shadow_;

  // The shadow value to tween to.
  gfx::ShadowValues end_shadow_;

  DISALLOW_COPY_AND_ASSIGN(ImageShadowAnimator);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_IMAGE_SHADOW_ANIMATOR_H_
