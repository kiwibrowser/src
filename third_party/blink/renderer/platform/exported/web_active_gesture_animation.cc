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

#include "third_party/blink/renderer/platform/exported/web_active_gesture_animation.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/public/platform/web_gesture_curve.h"
#include "third_party/blink/public/platform/web_gesture_curve_target.h"

namespace blink {

std::unique_ptr<WebActiveGestureAnimation>
WebActiveGestureAnimation::CreateWithTimeOffset(
    std::unique_ptr<WebGestureCurve> curve,
    WebGestureCurveTarget* target,
    base::TimeTicks start_time) {
  return base::WrapUnique(
      new WebActiveGestureAnimation(std::move(curve), target, start_time));
}

WebActiveGestureAnimation::~WebActiveGestureAnimation() = default;

WebActiveGestureAnimation::WebActiveGestureAnimation(
    std::unique_ptr<WebGestureCurve> curve,
    WebGestureCurveTarget* target,
    base::TimeTicks start_time)
    : start_time_(start_time), curve_(std::move(curve)), target_(target) {}

bool WebActiveGestureAnimation::Animate(base::TimeTicks time) {
  // All WebGestureCurves assume zero-based time, so we subtract
  // the animation start time before passing to the curve.
  // TODO(dcheng): WebGestureCurve should be using base::TimeDelta to represent
  // this.
  return curve_->AdvanceAndApplyToTarget((time - start_time_).InSecondsF(),
                                         target_);
}

}  // namespace blink
