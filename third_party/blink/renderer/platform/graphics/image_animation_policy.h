// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_IMAGE_ANIMATION_POLICY_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_IMAGE_ANIMATION_POLICY_H_

namespace blink {

// ImageAnimationPolicy is used for controlling image animation
// when image frame is rendered for animation

enum ImageAnimationPolicy {
  // Animate the image (the default).
  kImageAnimationPolicyAllowed,
  // Animate image just once.
  kImageAnimationPolicyAnimateOnce,
  // Show the first frame and do not animate.
  kImageAnimationPolicyNoAnimation
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_IMAGE_ANIMATION_POLICY_H_
