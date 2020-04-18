// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_lifetime_tracker.h"

#include "base/bind.h"
#include "base/stl_util.h"
#include "base/time/default_tick_clock.h"
#include "content/browser/service_worker/service_worker_metrics.h"

namespace content {

ServiceWorkerLifetimeTracker::ServiceWorkerLifetimeTracker()
    : ServiceWorkerLifetimeTracker(base::DefaultTickClock::GetInstance()) {}

ServiceWorkerLifetimeTracker::ServiceWorkerLifetimeTracker(
    const base::TickClock* tick_clock)
    : tick_clock_(tick_clock) {}

ServiceWorkerLifetimeTracker::~ServiceWorkerLifetimeTracker() = default;

void ServiceWorkerLifetimeTracker::StartTiming(int64_t embedded_worker_id) {
  DCHECK(!base::ContainsKey(running_workers_, embedded_worker_id));

  running_workers_[embedded_worker_id] = tick_clock_->NowTicks();
}

void ServiceWorkerLifetimeTracker::StopTiming(int64_t embedded_worker_id) {
  auto it = running_workers_.find(embedded_worker_id);
  if (it == running_workers_.end())
    return;
  ServiceWorkerMetrics::RecordRuntime(tick_clock_->NowTicks() - it->second);
  running_workers_.erase(it);
}

void ServiceWorkerLifetimeTracker::AbortTiming(int64_t embedded_worker_id) {
  auto it = running_workers_.find(embedded_worker_id);
  if (it == running_workers_.end())
    return;
  running_workers_.erase(it);
}

}  // namespace content
