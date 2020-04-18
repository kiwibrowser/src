// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/storage/mark_request_complete_task.h"

#include "content/browser/background_fetch/background_fetch_data_manager.h"
#include "content/browser/background_fetch/storage/database_helpers.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "third_party/blink/public/mojom/blob/blob.mojom.h"

namespace content {

namespace background_fetch {

MarkRequestCompleteTask::MarkRequestCompleteTask(
    BackgroundFetchDataManager* data_manager,
    BackgroundFetchRegistrationId registration_id,
    scoped_refptr<BackgroundFetchRequestInfo> request_info,
    MarkedCompleteCallback callback)
    : DatabaseTask(data_manager),
      registration_id_(registration_id),
      request_info_(std::move(request_info)),
      callback_(std::move(callback)),
      weak_factory_(this) {}

MarkRequestCompleteTask::~MarkRequestCompleteTask() = default;

void MarkRequestCompleteTask::Start() {
  StoreResponse();
}

void MarkRequestCompleteTask::StoreResponse() {
  ServiceWorkerResponse response;
  bool is_response_valid = data_manager()->FillServiceWorkerResponse(
      *request_info_, registration_id_.origin(), &response);

  if (!is_response_valid) {
    // No point in caching the response, just do the request state transition.
    CreateAndStoreCompletedRequest(false);
    return;
  }
  // TODO(rayankans): Store the request/response pair in cache storage, and call
  // `CreateAndStoreActiveRequest` via a callback.
  CreateAndStoreCompletedRequest(true);
}

void MarkRequestCompleteTask::CreateAndStoreCompletedRequest(bool succeeded) {
  completed_request_.set_unique_id(registration_id_.unique_id());
  completed_request_.set_request_index(request_info_->request_index());
  completed_request_.set_serialized_request(
      request_info_->fetch_request().Serialize());
  completed_request_.set_download_guid(request_info_->download_guid());
  completed_request_.set_succeeded(succeeded);

  service_worker_context()->StoreRegistrationUserData(
      registration_id_.service_worker_registration_id(),
      registration_id_.origin().GetURL(),
      {{CompletedRequestKey(completed_request_.unique_id(),
                            completed_request_.request_index()),
        completed_request_.SerializeAsString()}},
      base::BindRepeating(&MarkRequestCompleteTask::DidStoreCompletedRequest,
                          weak_factory_.GetWeakPtr()));
}

void MarkRequestCompleteTask::DidStoreCompletedRequest(
    ServiceWorkerStatusCode status) {
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kOk:
      break;
    case DatabaseStatus::kFailed:
    case DatabaseStatus::kNotFound:
      // TODO(crbug.com/780025): Log failures to UMA.
      Finished();  // Destroys |this|.
      return;
  }

  // Delete the active request.
  service_worker_context()->ClearRegistrationUserData(
      registration_id_.service_worker_registration_id(),
      {ActiveRequestKey(completed_request_.unique_id(),
                        completed_request_.request_index())},
      base::BindOnce(&MarkRequestCompleteTask::DidDeleteActiveRequest,
                     weak_factory_.GetWeakPtr()));
}

void MarkRequestCompleteTask::DidDeleteActiveRequest(
    ServiceWorkerStatusCode status) {
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kNotFound:
    case DatabaseStatus::kFailed:
      // TODO(crbug.com/780025): Log failures to UMA.
      break;
    case DatabaseStatus::kOk:
      std::move(callback_).Run();
      break;
  }

  Finished();  // Destroys |this|.

  // TODO(rayankans): Update download stats.
}

}  // namespace background_fetch

}  // namespace content