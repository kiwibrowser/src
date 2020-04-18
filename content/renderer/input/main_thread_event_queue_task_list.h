// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_MAIN_THREAD_EVENT_QUEUE_TASK_LIST_H_
#define CONTENT_RENDERER_INPUT_MAIN_THREAD_EVENT_QUEUE_TASK_LIST_H_

#include <memory>

#include "base/containers/circular_deque.h"
#include "content/renderer/input/main_thread_event_queue_task.h"

namespace content {

// The list of tasks that the main thread event queue will execute.
// This class supports coalescing upon queueing a task.
class MainThreadEventQueueTaskList {
 public:
  MainThreadEventQueueTaskList();
  ~MainThreadEventQueueTaskList();

  // Adds an event to the queue. The event may be coalesced with previously
  // queued events.
  void Queue(std::unique_ptr<MainThreadEventQueueTask> event);
  std::unique_ptr<MainThreadEventQueueTask> Pop();

  const std::unique_ptr<MainThreadEventQueueTask>& front() const {
    return queue_.front();
  }
  const std::unique_ptr<MainThreadEventQueueTask>& at(size_t pos) const {
    return queue_.at(pos);
  }

  bool empty() const { return queue_.empty(); }

  size_t size() const { return queue_.size(); }

 private:
  using EventQueue =
      base::circular_deque<std::unique_ptr<MainThreadEventQueueTask>>;
  EventQueue queue_;

  DISALLOW_COPY_AND_ASSIGN(MainThreadEventQueueTaskList);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INPUT_MAIN_THREAD_EVENT_QUEUE_TASK_LIST_H_
