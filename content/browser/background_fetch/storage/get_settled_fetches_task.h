// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_GET_SETTLED_FETCHES_TASK_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_GET_SETTLED_FETCHES_TASK_H_

#include "base/callback_forward.h"
#include "content/browser/background_fetch/storage/database_task.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "storage/browser/blob/blob_data_handle.h"

namespace content {

namespace background_fetch {

class GetSettledFetchesTask : public DatabaseTask {
 public:
  using SettledFetchesCallback = base::OnceCallback<void(
      blink::mojom::BackgroundFetchError,
      bool,
      std::vector<BackgroundFetchSettledFetch>,
      std::vector<std::unique_ptr<storage::BlobDataHandle>>)>;

  GetSettledFetchesTask(BackgroundFetchDataManager* data_manager,
                        BackgroundFetchRegistrationId registration_id,
                        SettledFetchesCallback callback);

  ~GetSettledFetchesTask() override;

  // DatabaseTask implementation:
  void Start() override;

 private:
  void DidGetCompletedRequests(const std::vector<std::string>& data,
                               ServiceWorkerStatusCode status);

  void FillFailedResponse(ServiceWorkerResponse* response,
                          const base::RepeatingClosure& callback);

  void FillSuccessfulResponse(ServiceWorkerResponse* response,
                              const base::RepeatingClosure& callback);

  void FinishTaskWithErrorCode(blink::mojom::BackgroundFetchError error);

  BackgroundFetchRegistrationId registration_id_;
  SettledFetchesCallback settled_fetches_callback_;

  std::vector<BackgroundFetchSettledFetch> settled_fetches_;
  bool background_fetch_succeeded_ = true;

  base::WeakPtrFactory<GetSettledFetchesTask> weak_factory_;  // Keep as last.

  DISALLOW_COPY_AND_ASSIGN(GetSettledFetchesTask);
};

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_GET_SETTLED_FETCHES_TASK_H_
