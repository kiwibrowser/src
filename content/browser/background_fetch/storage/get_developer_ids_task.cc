// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/storage/get_developer_ids_task.h"

#include <vector>

#include "content/browser/background_fetch/storage/database_helpers.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"

namespace content {

namespace background_fetch {

GetDeveloperIdsTask::GetDeveloperIdsTask(
    BackgroundFetchDataManager* data_manager,
    int64_t service_worker_registration_id,
    const url::Origin& origin,
    blink::mojom::BackgroundFetchService::GetDeveloperIdsCallback callback)
    : DatabaseTask(data_manager),
      service_worker_registration_id_(service_worker_registration_id),
      origin_(origin),
      callback_(std::move(callback)),
      weak_factory_(this) {}

GetDeveloperIdsTask::~GetDeveloperIdsTask() = default;

void GetDeveloperIdsTask::Start() {
  service_worker_context()->GetRegistrationUserKeysAndDataByKeyPrefix(
      service_worker_registration_id_, {kActiveRegistrationUniqueIdKeyPrefix},
      base::BindOnce(&GetDeveloperIdsTask::DidGetUniqueIds,
                     weak_factory_.GetWeakPtr()));
}

void GetDeveloperIdsTask::DidGetUniqueIds(
    const base::flat_map<std::string, std::string>& data_map,
    ServiceWorkerStatusCode status) {
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kNotFound:
      std::move(callback_).Run(blink::mojom::BackgroundFetchError::NONE,
                               {} /* No results */);
      break;
    case DatabaseStatus::kOk: {
      auto ids = std::vector<std::string>();
      ids.reserve(data_map.size());
      for (const auto& pair : data_map)
        ids.push_back(pair.first);
      std::move(callback_).Run(blink::mojom::BackgroundFetchError::NONE, ids);
      break;
    }
    case DatabaseStatus::kFailed:
      std::move(callback_).Run(
          blink::mojom::BackgroundFetchError::STORAGE_ERROR,
          {} /* No results */);
      break;
  }
  Finished();  // Destroys |this|.
}

}  // namespace background_fetch

}  // namespace content
