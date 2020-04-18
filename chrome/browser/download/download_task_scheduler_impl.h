// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_TASK_SCHEDULER_IMPL_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_TASK_SCHEDULER_IMPL_H_

#include <map>

#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/download/public/background_service/task_scheduler.h"

namespace content {
class BrowserContext;
}  // namespace content

// A TaskScheduler implementation that doesn't do anything but posts the task
// after the specified delay.
// If Chrome is shut down, the implementation will not automatically restart it.
class DownloadTaskSchedulerImpl : public download::TaskScheduler {
 public:
  explicit DownloadTaskSchedulerImpl(content::BrowserContext* context);
  ~DownloadTaskSchedulerImpl() override;

  // TaskScheduler implementation.
  void ScheduleTask(download::DownloadTaskType task_type,
                    bool require_unmetered_network,
                    bool require_charging,
                    int optimal_battery_percentage,
                    long window_start_time_seconds,
                    long window_end_time_seconds) override;
  void CancelTask(download::DownloadTaskType task_type) override;

 private:
  void RunScheduledTask(download::DownloadTaskType task_type);
  void OnTaskFinished(bool reschedule);

  content::BrowserContext* context_;

  // Keeps track of scheduled tasks so that they can be cancelled.
  std::map<download::DownloadTaskType, base::CancelableClosure>
      scheduled_tasks_;

  base::WeakPtrFactory<DownloadTaskSchedulerImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadTaskSchedulerImpl);
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_TASK_SCHEDULER_IMPL_H_
