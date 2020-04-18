// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/about_resource_loader.h"

#include <memory>
#include <vector>

#include "base/threading/thread_task_runner_handle.h"
#include "components/drive/job_scheduler.h"

namespace drive {
namespace internal {

AboutResourceLoader::AboutResourceLoader(JobScheduler* scheduler)
    : scheduler_(scheduler),
      current_update_task_id_(-1),
      weak_ptr_factory_(this) {}

AboutResourceLoader::~AboutResourceLoader() = default;

void AboutResourceLoader::GetAboutResource(
    const google_apis::AboutResourceCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  // If the latest UpdateAboutResource task is still running. Wait for it,
  if (pending_callbacks_.count(current_update_task_id_)) {
    pending_callbacks_[current_update_task_id_].emplace_back(callback);
    return;
  }

  if (cached_about_resource_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(callback, google_apis::HTTP_NO_CONTENT,
                                  std::make_unique<google_apis::AboutResource>(
                                      *cached_about_resource_)));
  } else {
    UpdateAboutResource(callback);
  }
}

void AboutResourceLoader::UpdateAboutResource(
    const google_apis::AboutResourceCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  ++current_update_task_id_;
  pending_callbacks_[current_update_task_id_].emplace_back(callback);

  scheduler_->GetAboutResource(base::BindRepeating(
      &AboutResourceLoader::UpdateAboutResourceAfterGetAbout,
      weak_ptr_factory_.GetWeakPtr(), current_update_task_id_));
}

void AboutResourceLoader::UpdateAboutResourceAfterGetAbout(
    int task_id,
    google_apis::DriveApiErrorCode status,
    std::unique_ptr<google_apis::AboutResource> about_resource) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  FileError error = GDataToFileError(status);

  const std::vector<google_apis::AboutResourceCallback> callbacks =
      pending_callbacks_[task_id];
  pending_callbacks_.erase(task_id);

  if (error != FILE_ERROR_OK) {
    for (auto& callback : callbacks)
      callback.Run(status, nullptr);
    return;
  }

  // Updates the cache when the resource is successfully obtained.
  if (cached_about_resource_ && cached_about_resource_->largest_change_id() >
                                    about_resource->largest_change_id()) {
    LOG(WARNING) << "Local cached about resource is fresher than server, "
                 << "local = " << cached_about_resource_->largest_change_id()
                 << ", server = " << about_resource->largest_change_id();
  }
  cached_about_resource_ =
      std::make_unique<google_apis::AboutResource>(*about_resource);

  for (auto& callback : callbacks) {
    callback.Run(status,
                 std::make_unique<google_apis::AboutResource>(*about_resource));
  }
}

}  // namespace internal
}  // namespace drive
