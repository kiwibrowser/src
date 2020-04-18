// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/file_manager/job_event_router.h"

#include <cmath>

#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chromeos/file_manager/app_id.h"
#include "chrome/browser/profiles/profile.h"
#include "components/drive/file_system_core_util.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;
namespace file_manager_private = extensions::api::file_manager_private;

namespace file_manager {

namespace {

// Utility function to check if |job_info| is a file uploading job.
bool IsUploadJob(const drive::JobType& type) {
  return (type == drive::TYPE_UPLOAD_NEW_FILE ||
          type == drive::TYPE_UPLOAD_EXISTING_FILE);
}

}  // namespace

JobEventRouter::JobEventRouter(const base::TimeDelta& event_delay)
    : event_delay_(event_delay),
      num_completed_bytes_(0),
      num_total_bytes_(0),
      weak_factory_(this) {
}

JobEventRouter::~JobEventRouter() {
}

void JobEventRouter::OnJobAdded(const drive::JobInfo& job_info) {
  OnJobUpdated(job_info);
}

void JobEventRouter::OnJobUpdated(const drive::JobInfo& job_info) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!drive::IsActiveFileTransferJobInfo(job_info))
    return;

  // Add new job info.
  UpdateBytes(job_info);
  drive_jobs_[job_info.job_id] = std::make_unique<drive::JobInfo>(job_info);

  ScheduleDriveFileTransferEvent(
      job_info, file_manager_private::TRANSFER_STATE_IN_PROGRESS,
      false /* immediate */);
}

void JobEventRouter::OnJobDone(const drive::JobInfo& job_info,
                               drive::FileError error) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!drive::IsActiveFileTransferJobInfo(job_info))
    return;

  const file_manager_private::TransferState state =
      error == drive::FILE_ERROR_OK
          ? file_manager_private::TRANSFER_STATE_COMPLETED
          : file_manager_private::TRANSFER_STATE_FAILED;

  drive::JobInfo completed_job = job_info;
  completed_job.num_completed_bytes = completed_job.num_total_bytes;
  UpdateBytes(completed_job);

  ScheduleDriveFileTransferEvent(job_info, state, true /* immediate */);

  // Forget about the job.
  drive_jobs_.erase(job_info.job_id);
  if (drive_jobs_.empty()) {
    num_completed_bytes_ = 0L;
    num_total_bytes_ = 0L;
  }
}

void JobEventRouter::UpdateBytes(const drive::JobInfo& job_info) {
  int64_t last_completed_bytes = 0;
  int64_t last_total_bytes = 0;
  if (drive_jobs_.count(job_info.job_id)) {
    last_completed_bytes = drive_jobs_[job_info.job_id]->num_completed_bytes;
    last_total_bytes = drive_jobs_[job_info.job_id]->num_total_bytes;
  }
  num_completed_bytes_ += job_info.num_completed_bytes - last_completed_bytes;
  num_total_bytes_ += job_info.num_total_bytes - last_total_bytes;
}

void JobEventRouter::ScheduleDriveFileTransferEvent(
    const drive::JobInfo& job_info,
    file_manager_private::TransferState state,
    bool immediate) {
  const bool no_pending_task = !pending_job_info_;

  pending_job_info_.reset(new drive::JobInfo(job_info));
  pending_state_ = state;

  if (immediate) {
    SendDriveFileTransferEvent();
  } else if (no_pending_task) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&JobEventRouter::SendDriveFileTransferEvent,
                       weak_factory_.GetWeakPtr()),
        event_delay_);
  }
}

void JobEventRouter::SendDriveFileTransferEvent() {
  if (!pending_job_info_)
    return;

  const std::set<std::string>& extension_ids =
      GetFileTransfersUpdateEventListenerExtensionIds();

  for (const auto extension_id : extension_ids) {
    DispatchFileTransfersUpdateEventToExtension(
        extension_id, *pending_job_info_, pending_state_, drive_jobs_.size(),
        num_completed_bytes_, num_total_bytes_);
  }

  pending_job_info_.reset();
}

void JobEventRouter::DispatchFileTransfersUpdateEventToExtension(
    const std::string& extension_id,
    const drive::JobInfo& job_info,
    const file_manager_private::TransferState& state,
    const int64_t num_total_jobs,
    const int64_t num_completed_bytes,
    const int64_t num_total_bytes) {
  file_manager_private::FileTransferStatus status;

  const GURL url =
      ConvertDrivePathToFileSystemUrl(job_info.file_path, extension_id);
  status.file_url = url.spec();
  status.transfer_state = state;
  status.transfer_type = IsUploadJob(job_info.job_type)
                             ? file_manager_private::TRANSFER_TYPE_UPLOAD
                             : file_manager_private::TRANSFER_TYPE_DOWNLOAD;
  // JavaScript does not have 64-bit integers. Instead we use double, which
  // is in IEEE 754 formant and accurate up to 52-bits in JS, and in practice
  // in C++. Larger values are rounded.
  status.num_total_jobs = num_total_jobs;
  status.processed = num_completed_bytes;
  status.total = num_total_bytes;

  DispatchEventToExtension(
      extension_id,
      extensions::events::FILE_MANAGER_PRIVATE_ON_FILE_TRANSFERS_UPDATED,
      file_manager_private::OnFileTransfersUpdated::kEventName,
      file_manager_private::OnFileTransfersUpdated::Create(status));
}

}  // namespace file_manager
