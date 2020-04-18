// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/main_thread_event_queue_task_list.h"

namespace content {

MainThreadEventQueueTaskList::MainThreadEventQueueTaskList() {}

MainThreadEventQueueTaskList::~MainThreadEventQueueTaskList() {}

void MainThreadEventQueueTaskList::Queue(
    std::unique_ptr<MainThreadEventQueueTask> event) {
  for (auto last_event_iter = queue_.rbegin(); last_event_iter != queue_.rend();
       ++last_event_iter) {
    switch ((*last_event_iter)->FilterNewEvent(event.get())) {
      case MainThreadEventQueueTask::FilterResult::CoalescedEvent:
        return;
      case MainThreadEventQueueTask::FilterResult::StopIterating:
        break;
      case MainThreadEventQueueTask::FilterResult::KeepIterating:
        continue;
    }
    break;
  }
  queue_.emplace_back(std::move(event));
}

std::unique_ptr<MainThreadEventQueueTask> MainThreadEventQueueTaskList::Pop() {
  std::unique_ptr<MainThreadEventQueueTask> result;
  if (!queue_.empty()) {
    result.reset(queue_.front().release());
    queue_.pop_front();
  }
  return result;
}

}  // namespace
