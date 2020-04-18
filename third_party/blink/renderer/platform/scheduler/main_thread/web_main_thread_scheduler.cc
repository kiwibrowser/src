// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/scheduler/web_main_thread_scheduler.h"

#include <memory>
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial.h"
#include "base/time/default_tick_clock.h"
#include "base/trace_event/trace_event.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/util/tracing_helper.h"

namespace blink {
namespace scheduler {

WebMainThreadScheduler::WebMainThreadScheduler() = default;

WebMainThreadScheduler::~WebMainThreadScheduler() = default;

WebMainThreadScheduler::RAILModeObserver::~RAILModeObserver() = default;

// static
std::unique_ptr<WebMainThreadScheduler> WebMainThreadScheduler::Create(
    base::Optional<base::Time> initial_virtual_time) {
  // Ensure categories appear as an option in chrome://tracing.
  WarmupTracingCategories();
  // Workers might be short-lived, so placing warmup here.
  TRACE_EVENT_WARMUP_CATEGORY(TRACE_DISABLED_BY_DEFAULT("worker.scheduler"));

  std::unique_ptr<MainThreadSchedulerImpl> scheduler(
      new MainThreadSchedulerImpl(
          base::sequence_manager::TaskQueueManager::TakeOverCurrentThread(),
          initial_virtual_time));
  return base::WrapUnique<WebMainThreadScheduler>(scheduler.release());
}

// static
const char* WebMainThreadScheduler::InputEventStateToString(
    InputEventState input_event_state) {
  switch (input_event_state) {
    case InputEventState::EVENT_CONSUMED_BY_COMPOSITOR:
      return "event_consumed_by_compositor";
    case InputEventState::EVENT_FORWARDED_TO_MAIN_THREAD:
      return "event_forwarded_to_main_thread";
    default:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace scheduler
}  // namespace blink
