// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/worklet_animation.h"

#include "cc/animation/scroll_timeline.h"

namespace cc {

WorkletAnimation::WorkletAnimation(
    int id,
    const std::string& name,
    std::unique_ptr<ScrollTimeline> scroll_timeline,
    bool is_controlling_instance)
    : SingleKeyframeEffectAnimation(id),
      name_(name),
      scroll_timeline_(std::move(scroll_timeline)),
      last_current_time_(base::nullopt),
      is_impl_instance_(is_controlling_instance) {}

WorkletAnimation::~WorkletAnimation() = default;

scoped_refptr<WorkletAnimation> WorkletAnimation::Create(
    int id,
    const std::string& name,
    std::unique_ptr<ScrollTimeline> scroll_timeline) {
  return WrapRefCounted(
      new WorkletAnimation(id, name, std::move(scroll_timeline), false));
}

scoped_refptr<Animation> WorkletAnimation::CreateImplInstance() const {
  std::unique_ptr<ScrollTimeline> impl_timeline;
  if (scroll_timeline_)
    impl_timeline = scroll_timeline_->CreateImplInstance();

  return WrapRefCounted(
      new WorkletAnimation(id(), name(), std::move(impl_timeline), true));
}

void WorkletAnimation::SetLocalTime(base::TimeDelta local_time) {
  local_time_ = local_time;
  SetNeedsPushProperties();
}

void WorkletAnimation::Tick(base::TimeTicks monotonic_time) {
  // Do not tick worklet animations on main thread. This should be removed if we
  // skip ticking all animations on main thread in http://crbug.com/762717.
  if (!is_impl_instance_)
    return;
  // As the output of a WorkletAnimation is driven by a script-provided local
  // time, we don't want the underlying effect to participate in the normal
  // animations lifecycle. To avoid this we pause the underlying keyframe effect
  // at the local time obtained from the user script - essentially turning each
  // call to |WorkletAnimation::Tick| into a seek in the effect.
  keyframe_effect()->Pause(local_time_);
  keyframe_effect()->Tick(monotonic_time);
}

// TODO(crbug.com/780151): The current time returned should be an offset against
// the animation's start time and based on the playback rate, not just the
// timeline time directly.
double WorkletAnimation::CurrentTime(base::TimeTicks monotonic_time,
                                     const ScrollTree& scroll_tree) {
  if (scroll_timeline_) {
    return scroll_timeline_->CurrentTime(scroll_tree);
  }

  // TODO(crbug.com/783333): Support DocumentTimeline's originTime concept.
  return (monotonic_time - base::TimeTicks()).InMillisecondsF();
}

bool WorkletAnimation::NeedsUpdate(base::TimeTicks monotonic_time,
                                   const ScrollTree& scroll_tree) {
  double current_time = CurrentTime(monotonic_time, scroll_tree);
  bool needs_update = last_current_time_ != current_time;
  last_current_time_ = current_time;
  return needs_update;
}

bool WorkletAnimation::IsWorkletAnimation() const {
  return true;
}

}  // namespace cc
