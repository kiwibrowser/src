// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_LIFETIME_TRACKER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_LIFETIME_TRACKER_H_

#include <map>
#include <memory>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/time/tick_clock.h"
#include "base/timer/timer.h"
#include "content/common/content_export.h"

namespace content {

// ServiceWorkerLifetimeTracker tracks how long service workers run, for UMA
// purposes.
class CONTENT_EXPORT ServiceWorkerLifetimeTracker {
 public:
  ServiceWorkerLifetimeTracker();
  explicit ServiceWorkerLifetimeTracker(const base::TickClock* tick_clock);
  virtual ~ServiceWorkerLifetimeTracker();

  // Called when the worker started running.
  void StartTiming(int64_t embedded_worker_id);
  // Called when the worker stopped running.
  void StopTiming(int64_t embedded_worker_id);
  // Called when DevTools was attached to the worker. Forgets the outstanding
  // start timing.
  void AbortTiming(int64_t embedded_worker_id);

 private:
  friend class ServiceWorkerLifetimeTrackerTest;

  void RecordHistograms();

  const base::TickClock* tick_clock_;
  std::map<int64_t /* embedded_worker_id */, base::TimeTicks /* start_time */>
      running_workers_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerLifetimeTracker);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_LIFETIME_TRACKER_H_
