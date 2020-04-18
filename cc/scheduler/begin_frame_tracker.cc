// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/scheduler/begin_frame_tracker.h"

namespace cc {

BeginFrameTracker::BeginFrameTracker(const base::Location& location)
    : location_(location),
      location_string_(location.ToString()),
      current_finished_at_(base::TimeTicks() +
                           base::TimeDelta::FromMicroseconds(-1)) {}

BeginFrameTracker::~BeginFrameTracker() = default;

void BeginFrameTracker::Start(viz::BeginFrameArgs new_args) {
  // Trace the frame time being passed between BeginFrameTrackers.
  TRACE_EVENT_FLOW_STEP0(
      TRACE_DISABLED_BY_DEFAULT("cc.debug.scheduler.frames"), "BeginFrameArgs",
      new_args.frame_time.since_origin().InMicroseconds(), location_string_);

  // Trace this specific begin frame tracker Start/Finish times.
  TRACE_EVENT_COPY_ASYNC_BEGIN2(
      TRACE_DISABLED_BY_DEFAULT("cc.debug.scheduler.frames"),
      location_string_.c_str(),
      new_args.frame_time.since_origin().InMicroseconds(), "new args",
      new_args.AsValue(), "current args", current_args_.AsValue());

  // Check the new viz::BeginFrameArgs are valid and monotonically increasing.
  DCHECK(new_args.IsValid());
  DCHECK_LE(current_args_.frame_time, new_args.frame_time);

  DCHECK(HasFinished())
      << "Tried to start a new frame before finishing an existing frame.";
  current_updated_at_ = base::TimeTicks::Now();
  current_args_ = new_args;
  current_finished_at_ = base::TimeTicks();

  // TODO(mithro): Add UMA tracking of delta between current_updated_at_ time
  // and the new_args.frame_time argument. This will give us how long after a
  // viz::BeginFrameArgs message was created before we started processing it.
}

const viz::BeginFrameArgs& BeginFrameTracker::Current() const {
  DCHECK(!HasFinished())
      << "Tried to use viz::BeginFrameArgs after marking the frame finished.";
  DCHECK(current_args_.IsValid())
      << "Tried to use viz::BeginFrameArgs before starting a frame!";
  return current_args_;
}

void BeginFrameTracker::Finish() {
  DCHECK(!HasFinished()) << "Tried to finish an already finished frame";
  current_finished_at_ = base::TimeTicks::Now();
  TRACE_EVENT_COPY_ASYNC_END0(
      TRACE_DISABLED_BY_DEFAULT("cc.debug.scheduler.frames"),
      location_string_.c_str(),
      current_args_.frame_time.since_origin().InMicroseconds());
}

const viz::BeginFrameArgs& BeginFrameTracker::Last() const {
  DCHECK(current_args_.IsValid())
      << "Tried to use last viz::BeginFrameArgs before starting a frame!";
  DCHECK(HasFinished())
      << "Tried to use last viz::BeginFrameArgs before the frame is finished.";
  return current_args_;
}

base::TimeDelta BeginFrameTracker::Interval() const {
  base::TimeDelta interval = current_args_.interval;
  // Normal interval will be ~16ms, 200Hz (5ms) screens are the fastest
  // easily available so anything less than that is likely an error.
  if (interval < base::TimeDelta::FromMilliseconds(1)) {
    interval = viz::BeginFrameArgs::DefaultInterval();
  }
  return interval;
}

void BeginFrameTracker::AsValueInto(
    base::TimeTicks now,
    base::trace_event::TracedValue* state) const {
  state->SetDouble("updated_at_ms",
                   current_updated_at_.since_origin().InMillisecondsF());
  state->SetDouble("finished_at_ms",
                   current_finished_at_.since_origin().InMillisecondsF());
  if (HasFinished()) {
    state->SetString("state", "FINISHED");
    state->BeginDictionary("current_args_");
  } else {
    state->SetString("state", "USING");
    state->BeginDictionary("last_args_");
  }
  current_args_.AsValueInto(state);
  state->EndDictionary();

  base::TimeTicks frame_time = current_args_.frame_time;
  base::TimeTicks deadline = current_args_.deadline;
  base::TimeDelta interval = current_args_.interval;
  state->BeginDictionary("major_timestamps_in_ms");
  state->SetDouble("0_interval", interval.InMillisecondsF());
  state->SetDouble("1_now_to_deadline", (deadline - now).InMillisecondsF());
  state->SetDouble("2_frame_time_to_now", (now - frame_time).InMillisecondsF());
  state->SetDouble("3_frame_time_to_deadline",
                   (deadline - frame_time).InMillisecondsF());
  state->SetDouble("4_now", now.since_origin().InMillisecondsF());
  state->SetDouble("5_frame_time", frame_time.since_origin().InMillisecondsF());
  state->SetDouble("6_deadline", deadline.since_origin().InMillisecondsF());
  state->EndDictionary();
}

const viz::BeginFrameArgs& BeginFrameTracker::DangerousMethodCurrentOrLast()
    const {
  if (!HasFinished()) {
    return Current();
  } else {
    return Last();
  }
}

}  // namespace cc
