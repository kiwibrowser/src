// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/animation/animation_test_api.h"

#include "base/time/time.h"
#include "ui/gfx/animation/animation.h"

namespace gfx {

// static
std::unique_ptr<base::AutoReset<Animation::RichAnimationRenderMode>>
AnimationTestApi::SetRichAnimationRenderMode(
    Animation::RichAnimationRenderMode mode) {
  DCHECK(Animation::rich_animation_rendering_mode_ ==
         Animation::RichAnimationRenderMode::PLATFORM);
  return std::make_unique<base::AutoReset<Animation::RichAnimationRenderMode>>(
      &Animation::rich_animation_rendering_mode_, mode);
}

AnimationTestApi::AnimationTestApi(Animation* animation)
    : animation_(animation) {}

AnimationTestApi::~AnimationTestApi() {}

void AnimationTestApi::SetStartTime(base::TimeTicks ticks) {
  animation_->SetStartTime(ticks);
}

void AnimationTestApi::Step(base::TimeTicks ticks) {
  animation_->Step(ticks);
}

}  // namespace gfx
