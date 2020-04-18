// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_ANIMATED_ICON_H_
#define CHROME_BROWSER_UI_COCOA_ANIMATED_ICON_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/macros.h"
#include "base/time/time.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/vector_icon_types.h"

namespace gfx {

class LinearAnimation;

}  // namespace gfx

// This class hosts a vector icon that defines transitions. It can be in the
// start steady state, the end steady state, or transitioning in between.
class AnimatedIcon : public gfx::AnimationDelegate {
 public:
  AnimatedIcon(const gfx::VectorIcon& icon, NSView* host_view);
  ~AnimatedIcon() override;

  void set_color(SkColor color) { color_ = color; }

  // Animates the icon. Restart from the beginning if it's already running.
  // Virtual for testing.
  virtual void Animate();

  // Paints the icon on the current drawing context, centered in |frame|.
  // Requires a NSGraphicsContext to be present.
  void PaintIcon(NSRect frame);

  // Returns true if |animation_| is currently animating. Virtual for testing.
  virtual bool IsAnimating() const;

  // gfx::AnimationDelegate:
  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationProgressed(const gfx::Animation* animation) override;

 private:
  const gfx::VectorIcon& icon_;

  // The NSView object that's displaying the icon.
  NSView* const host_view_;

  // The length of the animation.
  const base::TimeDelta duration_;

  std::unique_ptr<gfx::LinearAnimation> animation_;

  SkColor color_;

  DISALLOW_COPY_AND_ASSIGN(AnimatedIcon);
};

#endif  // CHROME_BROWSER_UI_COCOA_ANIMATED_ICON_H_
