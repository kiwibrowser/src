// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/start_page_token_loader.h"

#include "base/threading/thread_task_runner_handle.h"
#include "components/drive/job_scheduler.h"

using google_apis::StartPageToken;
using google_apis::StartPageTokenCallback;

namespace drive {
namespace internal {

StartPageTokenLoader::StartPageTokenLoader(const std::string& team_drive_id,
                                           JobScheduler* scheduler)
    : team_drive_id_(team_drive_id),
      scheduler_(scheduler),
      current_update_task_id_(-1),
      pending_update_callbacks_(),
      cached_start_page_token_(),
      weak_ptr_factory_(this) {}

StartPageTokenLoader::~StartPageTokenLoader() = default;

const StartPageToken* StartPageTokenLoader::cached_start_page_token() const {
  return cached_start_page_token_.get();
}

void StartPageTokenLoader::GetStartPageToken(
    const StartPageTokenCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  // If the update task is running, then we will wait for it.
  if (!pending_update_callbacks_[current_update_task_id_].empty()) {
    pending_update_callbacks_[current_update_task_id_].emplace_back(callback);
    return;
  }

  if (cached_start_page_token_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(callback, google_apis::HTTP_NO_CONTENT,
                                  std::make_unique<StartPageToken>(
                                      *cached_start_page_token_)));
  } else {
    UpdateStartPageToken(callback);
  }
}

void StartPageTokenLoader::UpdateStartPageToken(
    const StartPageTokenCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(callback);

  pending_update_callbacks_[++current_update_task_id_].emplace_back(callback);

  scheduler_->GetStartPageToken(
      team_drive_id_,
      base::BindRepeating(&StartPageTokenLoader::UpdateStartPageTokenAfterGet,
                          weak_ptr_factory_.GetWeakPtr(),
                          current_update_task_id_));
}

void StartPageTokenLoader::UpdateStartPageTokenAfterGet(
    int task_id,
    google_apis::DriveApiErrorCode status,
    std::unique_ptr<StartPageToken> start_page_token) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  FileError error = GDataToFileError(status);

  std::vector<StartPageTokenCallback> callbacks =
      pending_update_callbacks_[task_id];
  pending_update_callbacks_.erase(task_id);

  if (error != FILE_ERROR_OK) {
    for (auto& callback : callbacks) {
      callback.Run(status, nullptr);
    }
    return;
  }

  cached_start_page_token_ =
      std::make_unique<StartPageToken>(*start_page_token);

  for (auto& callback : callbacks) {
    callback.Run(status, std::make_unique<StartPageToken>(*start_page_token));
  }
}

}  // namespace internal
}  // namespace drive
