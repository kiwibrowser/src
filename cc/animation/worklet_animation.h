// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_WORKLET_ANIMATION_H_
#define CC_ANIMATION_WORKLET_ANIMATION_H_

#include "base/optional.h"
#include "base/time/time.h"
#include "cc/animation/animation_export.h"
#include "cc/animation/keyframe_effect.h"
#include "cc/animation/single_keyframe_effect_animation.h"

namespace cc {

class ScrollTimeline;

// A WorkletAnimation is an animation that allows its animation
// timing to be controlled by an animator instance that is running in a
// AnimationWorkletGlobalScope.
class CC_ANIMATION_EXPORT WorkletAnimation final
    : public SingleKeyframeEffectAnimation {
 public:
  WorkletAnimation(int id,
                   const std::string& name,
                   std::unique_ptr<ScrollTimeline> scroll_timeline,
                   bool is_controlling_instance);
  static scoped_refptr<WorkletAnimation> Create(
      int id,
      const std::string& name,
      std::unique_ptr<ScrollTimeline> scroll_timeline);
  scoped_refptr<Animation> CreateImplInstance() const override;

  const std::string& name() const { return name_; }
  const ScrollTimeline* scroll_timeline() const {
    return scroll_timeline_.get();
  }

  void SetLocalTime(base::TimeDelta local_time);
  bool IsWorkletAnimation() const override;

  void Tick(base::TimeTicks monotonic_time) override;

  // Returns the current time to be passed into the underlying AnimationWorklet.
  // The current time is based on the timeline associated with the animation.
  double CurrentTime(base::TimeTicks monotonic_time,
                     const ScrollTree& scroll_tree);

  // Returns true if the worklet animation needs to be updated which happens iff
  // its current time is going to be different from last time given these input.
  bool NeedsUpdate(base::TimeTicks monotonic_time,
                   const ScrollTree& scroll_tree);

 private:
  ~WorkletAnimation() override;

  std::string name_;

  // The ScrollTimeline associated with the underlying animation. If null, the
  // animation is based on a DocumentTimeline.
  //
  // TODO(crbug.com/780148): A WorkletAnimation should own an AnimationTimeline
  // which must exist but can either be a DocumentTimeline, ScrollTimeline, or
  // some other future implementation.
  std::unique_ptr<ScrollTimeline> scroll_timeline_;

  base::TimeDelta local_time_;

  base::Optional<double> last_current_time_;

  bool is_impl_instance_;
};

}  // namespace cc

#endif  // CC_ANIMATION_WORKLET_ANIMATION_H_
