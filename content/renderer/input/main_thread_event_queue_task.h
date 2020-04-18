// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_MAIN_THREAD_EVENT_QUEUE_TASK_H_
#define CONTENT_RENDERER_INPUT_MAIN_THREAD_EVENT_QUEUE_TASK_H_

#include "content/public/common/input_event_ack_state.h"
#include "third_party/blink/public/platform/scheduler/web_main_thread_scheduler.h"
#include "third_party/blink/public/platform/web_input_event.h"

namespace content {

class MainThreadEventQueue;

// A work item to execute from the main thread event queue.
// The MainThreadEventQueue supports 2 types of tasks (subclasses):
// 1) Closures
// 2) WebInputEvent
class MainThreadEventQueueTask {
 public:
  virtual ~MainThreadEventQueueTask() {}

  enum class FilterResult {
    // The passed in event was coalesced into this event. Don't queue
    // the new event.
    CoalescedEvent,

    // Stop invoking FilterNewEvent on any other events in the queue.
    StopIterating,

    // Keep invoking FilterNewEvent on the next older event in the queue.
    KeepIterating,
  };

  // Filter a new event that is about to be queued. Acceptable actions
  // are to coalesce event, stop iterating or keep iterating.
  // Iteration of the list begins at the end of the queue (newest to oldest).
  virtual FilterResult FilterNewEvent(MainThreadEventQueueTask*) = 0;
  virtual bool IsWebInputEvent() const = 0;
  virtual void Dispatch(MainThreadEventQueue*) = 0;
};

}  // namespace content

#endif  // CONTENT_RENDERER_INPUT_MAIN_THREAD_EVENT_QUEUE_TASK_H_
