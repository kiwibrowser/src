// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/storage/get_settled_fetches_task.h"

#include "base/barrier_closure.h"
#include "content/browser/background_fetch/background_fetch.pb.h"
#include "content/browser/background_fetch/storage/database_helpers.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"

namespace content {

namespace background_fetch {

GetSettledFetchesTask::GetSettledFetchesTask(
    BackgroundFetchDataManager* data_manager,
    BackgroundFetchRegistrationId registration_id,
    SettledFetchesCallback callback)
    : DatabaseTask(data_manager),
      registration_id_(registration_id),
      settled_fetches_callback_(std::move(callback)),
      weak_factory_(this) {}

GetSettledFetchesTask::~GetSettledFetchesTask() = default;

void GetSettledFetchesTask::Start() {
  service_worker_context()->GetRegistrationUserDataByKeyPrefix(
      registration_id_.service_worker_registration_id(),
      {CompletedRequestKeyPrefix(registration_id_.unique_id())},
      base::BindOnce(&GetSettledFetchesTask::DidGetCompletedRequests,
                     weak_factory_.GetWeakPtr()));
}

void GetSettledFetchesTask::DidGetCompletedRequests(
    const std::vector<std::string>& data,
    ServiceWorkerStatusCode status) {
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kOk:
      break;
    // TODO(crbug.com/780025): Log failures to UMA.
    case DatabaseStatus::kFailed:
      FinishTaskWithErrorCode(
          blink::mojom::BackgroundFetchError::STORAGE_ERROR);
      return;
    case DatabaseStatus::kNotFound:
      background_fetch_succeeded_ = false;
      FinishTaskWithErrorCode(blink::mojom::BackgroundFetchError::INVALID_ID);
      return;
  }

  // Nothing failed yet so the default state is success.
  if (data.empty()) {
    FinishTaskWithErrorCode(blink::mojom::BackgroundFetchError::NONE);
    return;
  }

  base::RepeatingClosure barrier_closure = base::BarrierClosure(
      data.size(),
      base::BindOnce(&GetSettledFetchesTask::FinishTaskWithErrorCode,
                     weak_factory_.GetWeakPtr(),
                     blink::mojom::BackgroundFetchError::NONE));

  settled_fetches_.reserve(data.size());
  for (const std::string& serialized_completed_request : data) {
    proto::BackgroundFetchCompletedRequest completed_request;
    if (!completed_request.ParseFromString(serialized_completed_request)) {
      NOTREACHED()
          << "Database is corrupt";  // TODO(crbug.com/780027): Nuke it.
    }

    settled_fetches_.emplace_back(BackgroundFetchSettledFetch());
    settled_fetches_.back().request =
        std::move(ServiceWorkerFetchRequest::ParseFromString(
            completed_request.serialized_request()));
    if (!completed_request.succeeded()) {
      FillFailedResponse(&settled_fetches_.back().response, barrier_closure);
      continue;
    }
    FillSuccessfulResponse(&settled_fetches_.back().response, barrier_closure);
  }
}

void GetSettledFetchesTask::FillFailedResponse(
    ServiceWorkerResponse* response,
    const base::RepeatingClosure& callback) {
  DCHECK(response);
  background_fetch_succeeded_ = false;
  // TODO(rayankans): Fill failed response with error reports.
  std::move(callback).Run();
}

void GetSettledFetchesTask::FillSuccessfulResponse(
    ServiceWorkerResponse* response,
    const base::RepeatingClosure& callback) {
  DCHECK(response);
  // TODO(rayankans): Get the response stored in Cache Storage.
  std::move(callback).Run();
}

void GetSettledFetchesTask::FinishTaskWithErrorCode(
    blink::mojom::BackgroundFetchError error) {
  std::move(settled_fetches_callback_)
      .Run(error, background_fetch_succeeded_, std::move(settled_fetches_),
           {} /* blob_data_handles */);
  Finished();  // Destroys |this|.
}

}  // namespace background_fetch

}  // namespace content
