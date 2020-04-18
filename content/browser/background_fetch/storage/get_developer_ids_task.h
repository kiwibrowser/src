// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_GET_DEVELOPER_IDS_TASK_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_GET_DEVELOPER_IDS_TASK_H_

#include <string>
#include <utility>

#include "base/callback_forward.h"
#include "base/containers/flat_map.h"
#include "content/browser/background_fetch/storage/database_task.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "third_party/blink/public/platform/modules/background_fetch/background_fetch.mojom.h"
#include "url/origin.h"

namespace content {

namespace background_fetch {

// Gets the developer ids for all active registrations - registrations that have
// not completed.
class GetDeveloperIdsTask : public DatabaseTask {
 public:
  GetDeveloperIdsTask(
      BackgroundFetchDataManager* data_manager,
      int64_t service_worker_registration_id,
      const url::Origin& origin,
      blink::mojom::BackgroundFetchService::GetDeveloperIdsCallback callback);

  ~GetDeveloperIdsTask() override;

  // DatabaseTask implementation:
  void Start() override;

 private:
  void DidGetUniqueIds(const base::flat_map<std::string, std::string>& data_map,
                       ServiceWorkerStatusCode status);

  int64_t service_worker_registration_id_;
  url::Origin origin_;

  blink::mojom::BackgroundFetchService::GetDeveloperIdsCallback callback_;

  base::WeakPtrFactory<GetDeveloperIdsTask> weak_factory_;  // Keep as last.

  DISALLOW_COPY_AND_ASSIGN(GetDeveloperIdsTask);
};

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_GET_DEVELOPER_IDS_TASK_H_
