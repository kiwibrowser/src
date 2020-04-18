// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/idleness_detector.h"

#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/instrumentation/resource_coordinator/frame_resource_coordinator.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"

namespace blink {

constexpr TimeDelta IdlenessDetector::kNetworkQuietWindow;
constexpr TimeDelta IdlenessDetector::kNetworkQuietWatchdog;

void IdlenessDetector::Shutdown() {
  Stop();
  local_frame_ = nullptr;
}

void IdlenessDetector::WillCommitLoad() {
  in_network_2_quiet_period_ = false;
  in_network_0_quiet_period_ = false;
  network_2_quiet_ = TimeTicks();
  network_0_quiet_ = TimeTicks();
  network_2_quiet_start_time_ = TimeTicks();
  network_0_quiet_start_time_ = TimeTicks();
}

void IdlenessDetector::DomContentLoadedEventFired() {
  if (!local_frame_)
    return;

  if (!task_observer_added_) {
    Platform::Current()->CurrentThread()->AddTaskTimeObserver(this);
    task_observer_added_ = true;
  }

  in_network_2_quiet_period_ = true;
  in_network_0_quiet_period_ = true;
  network_2_quiet_ = TimeTicks();
  network_0_quiet_ = TimeTicks();

  if (::resource_coordinator::IsPageAlmostIdleSignalEnabled()) {
    if (auto* frame_resource_coordinator =
            local_frame_->GetFrameResourceCoordinator()) {
      frame_resource_coordinator->SetNetworkAlmostIdle(false);
    }
  }
  OnDidLoadResource();
}

void IdlenessDetector::OnWillSendRequest(ResourceFetcher* fetcher) {
  // If |fetcher| is not the current fetcher of the Document, then that means
  // it's a new navigation, bail out in this case since it shouldn't affect the
  // current idleness of the local frame.
  if (!local_frame_ || fetcher != local_frame_->GetDocument()->Fetcher())
    return;

  // When OnWillSendRequest is called, the new loader hasn't been added to the
  // fetcher, thus we need to add 1 as the total request count.
  int request_count = fetcher->ActiveRequestCount() + 1;
  // If we are above the allowed number of active requests, reset timers.
  if (in_network_2_quiet_period_ && request_count > 2)
    network_2_quiet_ = TimeTicks();
  if (in_network_0_quiet_period_ && request_count > 0)
    network_0_quiet_ = TimeTicks();
}

// This function is called when the number of active connections is decreased.
// Note that the number of active connections doesn't decrease monotonically.
void IdlenessDetector::OnDidLoadResource() {
  if (!local_frame_)
    return;

  // Document finishes parsing after DomContentLoadedEventEnd is fired,
  // check the status in order to avoid false signals.
  if (!local_frame_->GetDocument()->HasFinishedParsing())
    return;

  // If we already reported quiet time, bail out.
  if (!in_network_0_quiet_period_ && !in_network_2_quiet_period_)
    return;

  int request_count =
      local_frame_->GetDocument()->Fetcher()->ActiveRequestCount();
  // If we did not achieve either 0 or 2 active connections, bail out.
  if (request_count > 2)
    return;

  TimeTicks timestamp = CurrentTimeTicks();
  // Arriving at =2 updates the quiet_2 base timestamp.
  // Arriving at <2 sets the quiet_2 base timestamp only if
  // it was not already set.
  if (request_count == 2 && in_network_2_quiet_period_) {
    network_2_quiet_ = timestamp;
    network_2_quiet_start_time_ = timestamp;
  } else if (request_count < 2 && in_network_2_quiet_period_ &&
             network_2_quiet_.is_null()) {
    network_2_quiet_ = timestamp;
    network_2_quiet_start_time_ = timestamp;
  }

  if (request_count == 0 && in_network_0_quiet_period_) {
    network_0_quiet_ = timestamp;
    network_0_quiet_start_time_ = timestamp;
  }

  if (!network_quiet_timer_.IsActive()) {
    network_quiet_timer_.StartOneShot(kNetworkQuietWatchdog, FROM_HERE);
  }
}

TimeTicks IdlenessDetector::GetNetworkAlmostIdleTime() {
  return network_2_quiet_start_time_;
}

TimeTicks IdlenessDetector::GetNetworkIdleTime() {
  return network_0_quiet_start_time_;
}

void IdlenessDetector::WillProcessTask(double start_time_seconds) {
  // If we have idle time and we are kNetworkQuietWindow seconds past it, emit
  // idle signals.
  TimeTicks start_time = TimeTicksFromSeconds(start_time_seconds);
  DocumentLoader* loader = local_frame_->Loader().GetDocumentLoader();
  if (in_network_2_quiet_period_ && !network_2_quiet_.is_null() &&
      start_time - network_2_quiet_ > kNetworkQuietWindow) {
    probe::lifecycleEvent(local_frame_, loader, "networkAlmostIdle",
                          TimeTicksInSeconds(network_2_quiet_start_time_));
    if (::resource_coordinator::IsPageAlmostIdleSignalEnabled()) {
      if (auto* frame_resource_coordinator =
              local_frame_->GetFrameResourceCoordinator()) {
        frame_resource_coordinator->SetNetworkAlmostIdle(true);
      }
    }
    local_frame_->GetDocument()->Fetcher()->OnNetworkQuiet();
    in_network_2_quiet_period_ = false;
    network_2_quiet_ = TimeTicks();
  }

  if (in_network_0_quiet_period_ && !network_0_quiet_.is_null() &&
      start_time - network_0_quiet_ > kNetworkQuietWindow) {
    probe::lifecycleEvent(local_frame_, loader, "networkIdle",
                          TimeTicksInSeconds(network_0_quiet_start_time_));
    in_network_0_quiet_period_ = false;
    network_0_quiet_ = TimeTicks();
  }

  if (!in_network_0_quiet_period_ && !in_network_2_quiet_period_)
    Stop();
}

void IdlenessDetector::DidProcessTask(double start_time_seconds,
                                      double end_time_seconds) {
  TimeTicks start_time = TimeTicksFromSeconds(start_time_seconds);
  TimeTicks end_time = TimeTicksFromSeconds(end_time_seconds);

  // Shift idle timestamps with the duration of the task, we were not idle.
  if (in_network_2_quiet_period_ && !network_2_quiet_.is_null())
    network_2_quiet_ += end_time - start_time;
  if (in_network_0_quiet_period_ && !network_0_quiet_.is_null())
    network_0_quiet_ += end_time - start_time;
}

IdlenessDetector::IdlenessDetector(LocalFrame* local_frame)
    : local_frame_(local_frame),
      task_observer_added_(false),
      network_quiet_timer_(
          local_frame->GetTaskRunner(TaskType::kInternalLoading),
          this,
          &IdlenessDetector::NetworkQuietTimerFired) {}

void IdlenessDetector::Stop() {
  network_quiet_timer_.Stop();
  if (!task_observer_added_)
    return;
  Platform::Current()->CurrentThread()->RemoveTaskTimeObserver(this);
  task_observer_added_ = false;
}

void IdlenessDetector::NetworkQuietTimerFired(TimerBase*) {
  // TODO(lpy) Reduce the number of timers.
  if ((in_network_0_quiet_period_ && !network_0_quiet_.is_null()) ||
      (in_network_2_quiet_period_ && !network_2_quiet_.is_null())) {
    network_quiet_timer_.StartOneShot(kNetworkQuietWatchdog, FROM_HERE);
  }
}

void IdlenessDetector::Trace(blink::Visitor* visitor) {
  visitor->Trace(local_frame_);
}

}  // namespace blink
