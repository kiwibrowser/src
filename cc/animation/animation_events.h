// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_ANIMATION_EVENTS_H_
#define CC_ANIMATION_ANIMATION_EVENTS_H_

#include <memory>
#include <vector>

#include "cc/animation/animation_curve.h"
#include "cc/animation/animation_export.h"
#include "cc/animation/keyframe_model.h"
#include "cc/paint/filter_operations.h"
#include "cc/trees/element_id.h"
#include "cc/trees/mutator_host.h"
#include "ui/gfx/transform.h"

namespace cc {

struct CC_ANIMATION_EXPORT AnimationEvent {
  enum Type { STARTED, FINISHED, ABORTED, TAKEOVER };

  AnimationEvent(Type type,
                 ElementId element_id,
                 int group_id,
                 int target_property,
                 base::TimeTicks monotonic_time);

  AnimationEvent(const AnimationEvent& other);
  AnimationEvent& operator=(const AnimationEvent& other);

  ~AnimationEvent();

  Type type;
  ElementId element_id;
  int group_id;
  int target_property;
  base::TimeTicks monotonic_time;
  bool is_impl_only;
  float opacity;
  gfx::Transform transform;
  FilterOperations filters;

  // For continuing a scroll offset animation on the main thread.
  base::TimeTicks animation_start_time;
  std::unique_ptr<AnimationCurve> curve;
};

class CC_ANIMATION_EXPORT AnimationEvents : public MutatorEvents {
 public:
  AnimationEvents();

  // MutatorEvents implementation.
  ~AnimationEvents() override;
  bool IsEmpty() const override;

  std::vector<AnimationEvent> events_;
};

}  // namespace cc

#endif  // CC_ANIMATION_ANIMATION_EVENTS_H_
