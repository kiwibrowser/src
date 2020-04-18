// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_EMPTY_TASK_SCHEDULER_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_EMPTY_TASK_SCHEDULER_H_

#include "components/download/public/background_service/task_scheduler.h"

#include "base/macros.h"

namespace download {

// Task scheduler that does nothing for incognito mode.
class EmptyTaskScheduler : public TaskScheduler {
 public:
  EmptyTaskScheduler();
  ~EmptyTaskScheduler() override;

 private:
  // TaskScheduler implementation.
  void ScheduleTask(DownloadTaskType task_type,
                    bool require_unmetered_network,
                    bool require_charging,
                    int optimal_battery_percentage,
                    long window_start_time_seconds,
                    long window_end_time_seconds) override;
  void CancelTask(DownloadTaskType task_type) override;

  DISALLOW_COPY_AND_ASSIGN(EmptyTaskScheduler);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_EMPTY_TASK_SCHEDULER_H_
