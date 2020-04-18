// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_job_controller.h"

#include <utility>

#include "content/browser/background_fetch/background_fetch_request_manager.h"
#include "content/public/browser/browser_thread.h"

namespace content {

BackgroundFetchJobController::BackgroundFetchJobController(
    BackgroundFetchDelegateProxy* delegate_proxy,
    const BackgroundFetchRegistrationId& registration_id,
    const BackgroundFetchOptions& options,
    const SkBitmap& icon,
    const BackgroundFetchRegistration& registration,
    BackgroundFetchRequestManager* request_manager,
    ProgressCallback progress_callback,
    BackgroundFetchScheduler::FinishedCallback finished_callback)
    : BackgroundFetchScheduler::Controller(registration_id,
                                           std::move(finished_callback)),
      options_(options),
      icon_(icon),
      complete_requests_downloaded_bytes_cache_(registration.downloaded),
      request_manager_(request_manager),
      delegate_proxy_(delegate_proxy),
      progress_callback_(std::move(progress_callback)),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void BackgroundFetchJobController::InitializeRequestStatus(
    int completed_downloads,
    int total_downloads,
    const std::vector<std::string>& outstanding_guids) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Don't allow double initialization.
  DCHECK_GT(total_downloads, 0);
  DCHECK_EQ(total_downloads_, 0);

  completed_downloads_ = completed_downloads;
  total_downloads_ = total_downloads;

  // TODO(nator): Update this when we support uploads.
  int total_downloads_size = options_.download_total;

  auto fetch_description = std::make_unique<BackgroundFetchDescription>(
      registration_id().unique_id(), options_.title, registration_id().origin(),
      icon_, completed_downloads, total_downloads,
      complete_requests_downloaded_bytes_cache_, total_downloads_size,
      outstanding_guids);

  delegate_proxy_->CreateDownloadJob(GetWeakPtr(),
                                     std::move(fetch_description));
}

BackgroundFetchJobController::~BackgroundFetchJobController() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

bool BackgroundFetchJobController::HasMoreRequests() {
  return completed_downloads_ < total_downloads_;
}

void BackgroundFetchJobController::StartRequest(
    scoped_refptr<BackgroundFetchRequestInfo> request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK_LT(completed_downloads_, total_downloads_);
  DCHECK(request);

  active_request_download_bytes_[request->download_guid()] = 0;

  delegate_proxy_->StartRequest(registration_id().unique_id(),
                                registration_id().origin(), request);
}

void BackgroundFetchJobController::DidStartRequest(
    const scoped_refptr<BackgroundFetchRequestInfo>& request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // TODO(delphick): Either add CORS check here or remove this function and do
  // the CORS check in BackgroundFetchDelegateImpl (since
  // download::Client::OnDownloadStarted returns a value that can abort the
  // download).
}

void BackgroundFetchJobController::DidUpdateRequest(
    const scoped_refptr<BackgroundFetchRequestInfo>& request,
    uint64_t bytes_downloaded) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  const std::string& download_guid = request->download_guid();
  if (active_request_download_bytes_[download_guid] == bytes_downloaded)
    return;

  active_request_download_bytes_[download_guid] = bytes_downloaded;

  progress_callback_.Run(registration_id().unique_id(), options_.download_total,
                         complete_requests_downloaded_bytes_cache_ +
                             GetInProgressDownloadedBytes());
}

void BackgroundFetchJobController::DidCompleteRequest(
    const scoped_refptr<BackgroundFetchRequestInfo>& request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  active_request_download_bytes_.erase(request->download_guid());
  complete_requests_downloaded_bytes_cache_ += request->GetFileSize();
  ++completed_downloads_;

  request_manager_->MarkRequestAsComplete(registration_id(), request.get());
}

void BackgroundFetchJobController::UpdateUI(const std::string& title) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  delegate_proxy_->UpdateUI(registration_id().unique_id(), title);
}

uint64_t BackgroundFetchJobController::GetInProgressDownloadedBytes() {
  uint64_t sum = 0;
  for (const auto& entry : active_request_download_bytes_)
    sum += entry.second;
  return sum;
}

void BackgroundFetchJobController::Abort(
    BackgroundFetchReasonToAbort reason_to_abort) {
  delegate_proxy_->Abort(registration_id().unique_id());

  std::vector<std::string> aborted_guids;
  for (const auto& pair : active_request_download_bytes_)
    aborted_guids.push_back(pair.first);
  request_manager_->OnJobAborted(registration_id(), std::move(aborted_guids));
  if (reason_to_abort != BackgroundFetchReasonToAbort::ABORTED_BY_DEVELOPER) {
    // Don't call Finish() here, so that we don't mark data for deletion while
    // there are active fetches.
    // Once the controller finishes processing, this function will be called
    // again. (BackgroundFetchScheduler's finished_callback_ will call
    // BackgroundFetchJobController::Abort() with |cancel_download| set to
    // true.)
    Finish(reason_to_abort);
  }
}

}  // namespace content
