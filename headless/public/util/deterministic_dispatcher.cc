// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/deterministic_dispatcher.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "headless/public/util/managed_dispatch_url_request_job.h"
#include "headless/public/util/navigation_request.h"

namespace headless {

DeterministicDispatcher::DeterministicDispatcher(
    scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner)
    : io_thread_task_runner_(std::move(io_thread_task_runner)),
      dispatch_pending_(false),
      navigation_in_progress_(false),
      weak_ptr_factory_(this) {}

DeterministicDispatcher::~DeterministicDispatcher() = default;

void DeterministicDispatcher::JobCreated(ManagedDispatchURLRequestJob* job) {
  base::AutoLock lock(lock_);
  pending_requests_.emplace_back(job);
}

void DeterministicDispatcher::JobKilled(ManagedDispatchURLRequestJob* job) {
  base::AutoLock lock(lock_);
  for (auto it = pending_requests_.begin(); it != pending_requests_.end();
       it++) {
    if (it->url_request == job) {
      pending_requests_.erase(it);
      break;
    }
  }
  ready_status_map_.erase(job);
  // We rely on JobDeleted getting called to call MaybeDispatchJobLocked.
}

void DeterministicDispatcher::JobFailed(ManagedDispatchURLRequestJob* job,
                                        net::Error error) {
  base::AutoLock lock(lock_);
  ready_status_map_[job] = error;
  MaybeDispatchJobLocked();
}

void DeterministicDispatcher::DataReady(ManagedDispatchURLRequestJob* job) {
  base::AutoLock lock(lock_);
  ready_status_map_[job] = net::OK;
  MaybeDispatchJobLocked();
}

void DeterministicDispatcher::JobDeleted(ManagedDispatchURLRequestJob* job) {
  base::AutoLock lock(lock_);
  MaybeDispatchJobLocked();
}

void DeterministicDispatcher::NavigationRequested(
    std::unique_ptr<NavigationRequest> navigation) {
  base::AutoLock lock(lock_);
  pending_requests_.emplace_back(std::move(navigation));

  MaybeDispatchJobLocked();
}

void DeterministicDispatcher::MaybeDispatchJobLocked() {
  if (dispatch_pending_ || navigation_in_progress_)
    return;

  if (ready_status_map_.empty()) {
    if (pending_requests_.empty())
      return;  // Nothing to do.

    // Don't post a task if the first job is a url request (which isn't ready
    // yet).
    if (pending_requests_.front().url_request)
      return;
  }

  dispatch_pending_ = true;
  io_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&DeterministicDispatcher::MaybeDispatchJobOnIOThreadTask,
                     weak_ptr_factory_.GetWeakPtr()));
}

void DeterministicDispatcher::MaybeDispatchJobOnIOThreadTask() {
  Request request;
  net::Error job_status = net::ERR_FAILED;

  {
    base::AutoLock lock(lock_);
    dispatch_pending_ = false;
    // If the job got deleted, |pending_requests_| may be empty.
    if (pending_requests_.empty())
      return;

    // Bail out if we're waiting for a navigation to complete.
    if (navigation_in_progress_)
      return;

    request = std::move(pending_requests_.front());
    if (request.url_request) {
      StatusMap::const_iterator it =
          ready_status_map_.find(request.url_request);
      // Bail out if the oldest job is not be ready for dispatch yet.
      if (it == ready_status_map_.end())
        return;

      job_status = it->second;
      ready_status_map_.erase(it);
    } else {
      DCHECK(!navigation_in_progress_);
      navigation_in_progress_ = true;
    }
    pending_requests_.pop_front();
  }

  if (request.url_request) {
    if (job_status == net::OK) {
      request.url_request->OnHeadersComplete();
    } else {
      request.url_request->OnStartError(job_status);
    }
  } else {
    request.navigation_request->StartProcessing(
        base::Bind(&DeterministicDispatcher::NavigationDoneTask,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void DeterministicDispatcher::NavigationDoneTask() {
  {
    base::AutoLock lock(lock_);
    DCHECK(navigation_in_progress_);
    navigation_in_progress_ = false;
  }

  MaybeDispatchJobLocked();
}

DeterministicDispatcher::Request::Request() = default;
DeterministicDispatcher::Request::Request(Request&&) = default;
DeterministicDispatcher::Request::~Request() = default;

DeterministicDispatcher::Request::Request(
    ManagedDispatchURLRequestJob* url_request)
    : url_request(url_request) {}

DeterministicDispatcher::Request::Request(
    std::unique_ptr<NavigationRequest> navigation_request)
    : navigation_request(std::move(navigation_request)) {}

DeterministicDispatcher::Request& DeterministicDispatcher::Request::operator=(
    DeterministicDispatcher::Request&& other) = default;

}  // namespace headless
