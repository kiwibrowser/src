// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_ANIMATION_ANIMATION_CONTAINER_H_
#define UI_GFX_ANIMATION_ANIMATION_CONTAINER_H_

#include <utility>

#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "ui/gfx/animation/animation_export.h"

namespace gfx {

class AnimationContainerElement;
class AnimationContainerObserver;

// AnimationContainer is used by Animation to manage the underlying timer.
// Internally each Animation creates a single AnimationContainer. You can
// group a set of Animations into the same AnimationContainer by way of
// Animation::SetContainer. Grouping a set of Animations into the same
// AnimationContainer ensures they all update and start at the same time.
//
// AnimationContainer is ref counted. Each Animation contained within the
// AnimationContainer own it.
class ANIMATION_EXPORT AnimationContainer
    : public base::RefCounted<AnimationContainer> {
 public:
  AnimationContainer();

  // Invoked by Animation when it needs to start. Starts the timer if necessary.
  // NOTE: This is invoked by Animation for you, you shouldn't invoke this
  // directly.
  void Start(AnimationContainerElement* animation);

  // Invoked by Animation when it needs to stop. If there are no more animations
  // running the timer stops.
  // NOTE: This is invoked by Animation for you, you shouldn't invoke this
  // directly.
  void Stop(AnimationContainerElement* animation);

  void set_observer(AnimationContainerObserver* observer) {
    observer_ = observer;
  }

  // The time the last animation ran at.
  base::TimeTicks last_tick_time() const { return last_tick_time_; }

  // Are there any timers running?
  bool is_running() const { return !elements_.empty(); }

 private:
  friend class base::RefCounted<AnimationContainer>;

  // This set is usually quite small so a flat_set is the most obvious choice.
  // However, in extreme cases this can grow to 100s or even 1000s of elements.
  // Since this set is duplicated on every call to 'Run' and indexed very
  // frequently the cache locality of the vector is more important than the
  // costlier (but rarer) insertion. Profiling shows that flat_set continues to
  // perform best in these cases (up to 12x faster than std::set).
  typedef base::flat_set<AnimationContainerElement*> Elements;

  ~AnimationContainer();

  // Timer callback method.
  void Run();

  // Sets min_timer_interval_ and restarts the timer.
  void SetMinTimerInterval(base::TimeDelta delta);

  // Returns the min timer interval of all the timers, and the count of timers
  // at that interval.
  std::pair<base::TimeDelta, size_t> GetMinIntervalAndCount() const;

  // Represents one of two possible values:
  // . If only a single animation has been started and the timer hasn't yet
  //   fired this is the time the animation was added.
  // . The time the last animation ran at (::Run was invoked).
  base::TimeTicks last_tick_time_;

  // Set of elements (animations) being managed.
  Elements elements_;

  // Minimum interval the timers run at, plus the number of timers that have
  // been seen at that interval. The most common case is for all of the
  // animations to run at 60Hz, in which case all of the intervals are the same.
  // This acts as a cache of size 1, and when an animation stops and is removed
  // it means that the linear scan for the new minimum timer can almost always
  // be avoided.
  base::TimeDelta min_timer_interval_;
  size_t min_timer_interval_count_;

  base::RepeatingTimer timer_;

  AnimationContainerObserver* observer_;

  DISALLOW_COPY_AND_ASSIGN(AnimationContainer);
};

}  // namespace gfx

#endif  // UI_GFX_ANIMATION_ANIMATION_CONTAINER_H_
