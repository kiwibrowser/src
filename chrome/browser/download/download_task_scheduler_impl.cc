// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/download_task_scheduler_impl.h"

#include "base/bind.h"
#include "base/cancelable_callback.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/download/download_service_factory.h"
#include "components/download/public/background_service/download_service.h"
#include "content/public/browser/browser_context.h"

DownloadTaskSchedulerImpl::DownloadTaskSchedulerImpl(
    content::BrowserContext* context)
    : context_(context), weak_factory_(this) {}

DownloadTaskSchedulerImpl::~DownloadTaskSchedulerImpl() = default;

void DownloadTaskSchedulerImpl::ScheduleTask(
    download::DownloadTaskType task_type,
    bool require_unmetered_network,
    bool require_charging,
    int optimal_battery_percentage,
    long window_start_time_seconds,
    long window_end_time_seconds) {
  // We only rely on this for cleanup tasks. Since this doesn't restart Chrome,
  // for download tasks it doesn't do much and we handle them outside of task
  // scheduler.
  if (task_type != download::DownloadTaskType::CLEANUP_TASK)
    return;

  scheduled_tasks_[task_type].Reset(
      base::Bind(&DownloadTaskSchedulerImpl::RunScheduledTask,
                 weak_factory_.GetWeakPtr(), task_type));
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, scheduled_tasks_[task_type].callback(),
      base::TimeDelta::FromSeconds(window_start_time_seconds));
}

void DownloadTaskSchedulerImpl::CancelTask(
    download::DownloadTaskType task_type) {
  scheduled_tasks_[task_type].Cancel();
}

void DownloadTaskSchedulerImpl::RunScheduledTask(
    download::DownloadTaskType task_type) {
  download::DownloadService* download_service =
      DownloadServiceFactory::GetForBrowserContext(context_);
  download_service->OnStartScheduledTask(
      task_type, base::Bind(&DownloadTaskSchedulerImpl::OnTaskFinished,
                            weak_factory_.GetWeakPtr()));
}

void DownloadTaskSchedulerImpl::OnTaskFinished(bool reschedule) {
  // TODO(shaktisahu): Cache the original scheduling params and re-post task in
  // case it needs reschedule.
}
