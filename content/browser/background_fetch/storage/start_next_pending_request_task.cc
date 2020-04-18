// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/storage/start_next_pending_request_task.h"

#include "base/guid.h"
#include "content/browser/background_fetch/storage/database_helpers.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"

namespace content {

namespace background_fetch {

StartNextPendingRequestTask::StartNextPendingRequestTask(
    BackgroundFetchDataManager* data_manager,
    int64_t service_worker_registration_id,
    std::unique_ptr<proto::BackgroundFetchMetadata> metadata,
    NextRequestCallback callback)
    : DatabaseTask(data_manager),
      service_worker_registration_id_(service_worker_registration_id),
      metadata_(std::move(metadata)),
      callback_(std::move(callback)),
      weak_factory_(this) {
  DCHECK(metadata_);
}

StartNextPendingRequestTask::~StartNextPendingRequestTask() = default;

void StartNextPendingRequestTask::Start() {
  GetPendingRequests();
}

void StartNextPendingRequestTask::GetPendingRequests() {
  service_worker_context()->GetRegistrationUserDataByKeyPrefix(
      service_worker_registration_id_,
      PendingRequestKeyPrefix(metadata_->registration().unique_id()),
      base::BindOnce(&StartNextPendingRequestTask::DidGetPendingRequests,
                     weak_factory_.GetWeakPtr()));
}

void StartNextPendingRequestTask::DidGetPendingRequests(
    const std::vector<std::string>& data,
    ServiceWorkerStatusCode status) {
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kNotFound:
    case DatabaseStatus::kFailed:
      // TODO(crbug.com/780025): Log failures to UMA.
      std::move(callback_).Run(nullptr /* request */);
      Finished();  // Destroys |this|.
      return;
    case DatabaseStatus::kOk:
      if (data.empty()) {
        // There are no pending requests.
        std::move(callback_).Run(nullptr /* request */);
        Finished();  // Destroys |this|.
        return;
      }
  }

  if (!pending_request_.ParseFromString(data.front())) {
    NOTREACHED() << "Database is corrupt";  // TODO(crbug.com/780027): Nuke it.
  }

  // Make sure there isn't already an Active Request.
  // This might happen if the browser is killed in-between writes.
  service_worker_context()->GetRegistrationUserData(
      service_worker_registration_id_,
      {ActiveRequestKey(pending_request_.unique_id(),
                        pending_request_.request_index())},
      base::BindOnce(&StartNextPendingRequestTask::DidFindActiveRequest,
                     weak_factory_.GetWeakPtr()));
}

void StartNextPendingRequestTask::DidFindActiveRequest(
    const std::vector<std::string>& data,
    ServiceWorkerStatusCode status) {
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kFailed:
      // TODO(crbug.com/780025): Log failures to UMA.
      std::move(callback_).Run(nullptr /* request */);
      Finished();  // Destroys |this|.
      return;
    case DatabaseStatus::kNotFound:
      CreateAndStoreActiveRequest();
      return;
    case DatabaseStatus::kOk:
      // We already stored the active request.
      if (!active_request_.ParseFromString(data.front())) {
        NOTREACHED()
            << "Database is corrupt";  // TODO(crbug.com/780027): Nuke it.
      }
      StartDownload();
      return;
  }
  NOTREACHED();
}

void StartNextPendingRequestTask::CreateAndStoreActiveRequest() {
  proto::BackgroundFetchActiveRequest active_request;

  active_request_.set_download_guid(base::GenerateGUID());
  active_request_.set_unique_id(pending_request_.unique_id());
  active_request_.set_request_index(pending_request_.request_index());
  // Transfer ownership of the request to avoid a potentially expensive copy.
  active_request_.set_allocated_serialized_request(
      pending_request_.release_serialized_request());

  service_worker_context()->StoreRegistrationUserData(
      service_worker_registration_id_, GURL(metadata_->origin()),
      {{ActiveRequestKey(active_request_.unique_id(),
                         active_request_.request_index()),
        active_request_.SerializeAsString()}},
      base::BindRepeating(&StartNextPendingRequestTask::DidStoreActiveRequest,
                          weak_factory_.GetWeakPtr()));
}

void StartNextPendingRequestTask::DidStoreActiveRequest(
    ServiceWorkerStatusCode status) {
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kOk:
      break;
    case DatabaseStatus::kFailed:
    case DatabaseStatus::kNotFound:
      // TODO(crbug.com/780025): Log failures to UMA.
      std::move(callback_).Run(nullptr /* request */);
      Finished();  // Destroys |this|.
      return;
  }
  StartDownload();
}

void StartNextPendingRequestTask::StartDownload() {
  DCHECK(!active_request_.download_guid().empty());

  auto next_request = base::MakeRefCounted<BackgroundFetchRequestInfo>(
      active_request_.request_index(),
      ServiceWorkerFetchRequest::ParseFromString(
          active_request_.serialized_request()));
  next_request->SetDownloadGuid(active_request_.download_guid());

  std::move(callback_).Run(next_request);

  // Delete the pending request.
  service_worker_context()->ClearRegistrationUserData(
      service_worker_registration_id_,
      {PendingRequestKey(pending_request_.unique_id(),
                         pending_request_.request_index())},
      base::BindOnce(&StartNextPendingRequestTask::DidDeletePendingRequest,
                     weak_factory_.GetWeakPtr()));
}

void StartNextPendingRequestTask::DidDeletePendingRequest(
    ServiceWorkerStatusCode status) {
  // TODO(crbug.com/780025): Log failures to UMA.
  Finished();  // Destroys |this|.
}

}  // namespace background_fetch

}  // namespace content
