// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_scheduler.h"

#include "base/guid.h"
#include "content/browser/background_fetch/background_fetch_job_controller.h"

namespace content {

BackgroundFetchScheduler::Controller::Controller(
    const BackgroundFetchRegistrationId& registration_id,
    FinishedCallback finished_callback)
    : registration_id_(registration_id),
      finished_callback_(std::move(finished_callback)) {
  DCHECK(finished_callback_);
}

BackgroundFetchScheduler::Controller::~Controller() = default;

void BackgroundFetchScheduler::Controller::Finish(
    BackgroundFetchReasonToAbort reason_to_abort) {
  DCHECK(reason_to_abort != BackgroundFetchReasonToAbort::NONE ||
         !HasMoreRequests());

  std::move(finished_callback_).Run(registration_id_, reason_to_abort);
}

BackgroundFetchScheduler::BackgroundFetchScheduler(
    BackgroundFetchScheduler::RequestProvider* request_provider)
    : request_provider_(request_provider) {}

BackgroundFetchScheduler::~BackgroundFetchScheduler() = default;

void BackgroundFetchScheduler::AddJobController(
    BackgroundFetchScheduler::Controller* controller) {
  controller_queue_.push_back(controller);

  while (!controller_queue_.empty() &&
         download_controller_map_.size() < max_concurrent_downloads_) {
    ScheduleDownload();
  }
}

void BackgroundFetchScheduler::ScheduleDownload() {
  DCHECK(download_controller_map_.size() < max_concurrent_downloads_);

  if (controller_queue_.empty())
    return;

  auto* controller = controller_queue_.front();
  controller_queue_.pop_front();

  request_provider_->PopNextRequest(
      controller->registration_id(),
      base::BindOnce(&BackgroundFetchScheduler::DidPopNextRequest,
                     base::Unretained(this), controller));
}

void BackgroundFetchScheduler::DidPopNextRequest(
    BackgroundFetchScheduler::Controller* controller,
    scoped_refptr<BackgroundFetchRequestInfo> request_info) {
  download_controller_map_[request_info->download_guid()] = controller;
  controller->StartRequest(request_info);
}

void BackgroundFetchScheduler::MarkRequestAsComplete(
    const BackgroundFetchRegistrationId& registration_id,
    scoped_refptr<BackgroundFetchRequestInfo> request) {
  DCHECK(download_controller_map_.count(request->download_guid()));
  auto* controller = download_controller_map_[request->download_guid()];
  download_controller_map_.erase(request->download_guid());

  request_provider_->MarkRequestAsComplete(
      controller->registration_id(), request.get(),
      base::BindOnce(&BackgroundFetchScheduler::DidMarkRequestAsComplete,
                     base::Unretained(this), controller));
}

void BackgroundFetchScheduler::DidMarkRequestAsComplete(
    BackgroundFetchScheduler::Controller* controller) {
  if (controller->HasMoreRequests())
    controller_queue_.push_back(controller);
  else
    controller->Finish(BackgroundFetchReasonToAbort::NONE);

  ScheduleDownload();
}

void BackgroundFetchScheduler::OnJobAborted(
    const BackgroundFetchRegistrationId& registration_id,
    std::vector<std::string> aborted_guids) {
  // For every active download that was aborted, remove it and schedule a new
  // download.
  for (const auto& guid : aborted_guids) {
    download_controller_map_.erase(guid);
    ScheduleDownload();
  }
}

}  // namespace content
