/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_ANIMATION_CLOCK_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_ANIMATION_CLOCK_H_

#include <limits>

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

// Maintains a stationary clock time during script execution.  Tries to track
// the glass time (the moment photons leave the screen) of the current animation
// frame.
class CORE_EXPORT AnimationClock {
  DISALLOW_NEW();

 public:
  using TimeTicksFunction = base::TimeTicks (*)();
  explicit AnimationClock(
      TimeTicksFunction monotonically_increasing_time = WTF::CurrentTimeTicks)
      : monotonically_increasing_time_(monotonically_increasing_time),
        time_(),
        task_for_which_time_was_calculated_(
            std::numeric_limits<unsigned>::max()) {}

  void UpdateTime(base::TimeTicks time);
  double CurrentTime();
  void ResetTimeForTesting(base::TimeTicks time = base::TimeTicks());
  void DisableSyntheticTimeForTesting() {
    monotonically_increasing_time_ = nullptr;
  }

  // notifyTaskStart should be called right before the main message loop starts
  // to run the next task from the message queue.
  static void NotifyTaskStart() { ++currently_running_task_; }

 private:
  TimeTicksFunction monotonically_increasing_time_;
  base::TimeTicks time_;
  unsigned task_for_which_time_was_calculated_;
  static unsigned currently_running_task_;
  DISALLOW_COPY_AND_ASSIGN(AnimationClock);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_ANIMATION_CLOCK_H_
