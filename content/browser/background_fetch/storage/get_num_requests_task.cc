// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/storage/get_num_requests_task.h"

#include "content/browser/background_fetch/background_fetch.pb.h"
#include "content/browser/background_fetch/storage/database_helpers.h"
#include "content/browser/background_fetch/storage/get_metadata_task.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"

namespace content {

namespace background_fetch {

namespace {

void HandleGetMetadataCallback(
    GetNumRequestsTask::NumRequestsCallback callback,
    blink::mojom::BackgroundFetchError error,
    std::unique_ptr<proto::BackgroundFetchMetadata> metadata) {
  if (error != blink::mojom::BackgroundFetchError::NONE) {
    std::move(callback).Run(0u);
    return;
  }

  DCHECK(metadata);
  std::move(callback).Run(metadata->num_fetches());
}

}  // namespace

GetNumRequestsTask::GetNumRequestsTask(
    BackgroundFetchDataManager* data_manager,
    const BackgroundFetchRegistrationId& registration_id,
    RequestType type,
    NumRequestsCallback callback)
    : DatabaseTask(data_manager),
      registration_id_(registration_id),
      type_(type),
      callback_(std::move(callback)),
      weak_factory_(this) {}

GetNumRequestsTask::~GetNumRequestsTask() = default;

void GetNumRequestsTask::Start() {
  switch (type_) {
    case RequestType::kAny:
      GetMetadata();
      return;
    case RequestType::kPending:
      GetRequests(PendingRequestKeyPrefix(registration_id_.unique_id()));
      return;
    case RequestType::kActive:
      GetRequests(ActiveRequestKeyPrefix(registration_id_.unique_id()));
      return;
    case RequestType::kCompleted:
      GetRequests(CompletedRequestKeyPrefix(registration_id_.unique_id()));
      return;
  }
  NOTREACHED();
}

void GetNumRequestsTask::GetMetadata() {
  AddDatabaseTask(std::make_unique<GetMetadataTask>(
      data_manager(), registration_id_.service_worker_registration_id(),
      registration_id_.origin(), registration_id_.developer_id(),
      base::BindOnce(&HandleGetMetadataCallback, std::move(callback_))));
  Finished();  // Destroys |this|.
}

void GetNumRequestsTask::GetRequests(const std::string& key_prefix) {
  service_worker_context()->GetRegistrationUserDataByKeyPrefix(
      registration_id_.service_worker_registration_id(), key_prefix,
      base::BindOnce(&GetNumRequestsTask::DidGetRequests,
                     weak_factory_.GetWeakPtr()));
}

void GetNumRequestsTask::DidGetRequests(const std::vector<std::string>& data,
                                        ServiceWorkerStatusCode status) {
  DCHECK_EQ(ToDatabaseStatus(status), DatabaseStatus::kOk);
  std::move(callback_).Run(data.size());
  Finished();  // Destroys |this|.
}

}  // namespace background_fetch

}  // namespace content
