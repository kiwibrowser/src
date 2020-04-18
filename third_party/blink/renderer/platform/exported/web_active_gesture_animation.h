/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_EXPORTED_WEB_ACTIVE_GESTURE_ANIMATION_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_EXPORTED_WEB_ACTIVE_GESTURE_ANIMATION_H_

#include <memory>
#include "base/time/time.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class WebGestureCurve;
class WebGestureCurveTarget;

// Implements a gesture animation (fling scroll, etc.) using a curve with a
// generic interface to define the animation parameters as a function of time,
// and applies the animation to a target, again via a generic interface. It is
// assumed that animate() is called on a more-or-less regular basis by the
// owner.
class PLATFORM_EXPORT WebActiveGestureAnimation {
  USING_FAST_MALLOC(WebActiveGestureAnimation);
  WTF_MAKE_NONCOPYABLE(WebActiveGestureAnimation);

 public:
  static std::unique_ptr<WebActiveGestureAnimation> CreateWithTimeOffset(
      std::unique_ptr<WebGestureCurve>,
      WebGestureCurveTarget*,
      base::TimeTicks start_time);
  ~WebActiveGestureAnimation();

  bool Animate(base::TimeTicks);

 private:
  // Assumes a valid WebGestureCurveTarget that outlives the animation.
  WebActiveGestureAnimation(std::unique_ptr<WebGestureCurve>,
                            WebGestureCurveTarget*,
                            base::TimeTicks start_time);

  base::TimeTicks start_time_;
  std::unique_ptr<WebGestureCurve> curve_;
  WebGestureCurveTarget* target_;
};

}  // namespace blink

#endif
