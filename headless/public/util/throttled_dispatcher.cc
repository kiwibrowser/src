// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/throttled_dispatcher.h"

#include <utility>

#include "base/bind.h"
#include "base/synchronization/lock.h"
#include "headless/public/util/managed_dispatch_url_request_job.h"

namespace headless {

ThrottledDispatcher::ThrottledDispatcher(
    scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner)
    : ExpeditedDispatcher(std::move(io_thread_task_runner)),
      requests_paused_(false) {}

ThrottledDispatcher::~ThrottledDispatcher() = default;

void ThrottledDispatcher::PauseRequests() {
  base::AutoLock lock(lock_);
  requests_paused_ = true;
}

void ThrottledDispatcher::ResumeRequests() {
  base::AutoLock lock(lock_);
  requests_paused_ = false;
  for (ManagedDispatchURLRequestJob* job : paused_jobs_) {
    io_thread_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&ManagedDispatchURLRequestJob::OnHeadersComplete,
                       job->GetWeakPtr()));
  }
  paused_jobs_.clear();
}

void ThrottledDispatcher::DataReady(ManagedDispatchURLRequestJob* job) {
  base::AutoLock lock(lock_);
  if (requests_paused_) {
    paused_jobs_.push_back(job);
  } else {
    io_thread_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&ManagedDispatchURLRequestJob::OnHeadersComplete,
                       job->GetWeakPtr()));
  }
}

void ThrottledDispatcher::JobDeleted(ManagedDispatchURLRequestJob* job) {
  base::AutoLock lock(lock_);
  paused_jobs_.erase(std::remove(paused_jobs_.begin(), paused_jobs_.end(), job),
                     paused_jobs_.end());
}

}  // namespace headless
