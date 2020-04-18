// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_START_NEXT_PENDING_REQUEST_TASK_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_START_NEXT_PENDING_REQUEST_TASK_H_

#include "base/callback_forward.h"
#include "content/browser/background_fetch/background_fetch.pb.h"
#include "content/browser/background_fetch/background_fetch_request_info.h"
#include "content/browser/background_fetch/storage/database_task.h"
#include "content/common/service_worker/service_worker_status_code.h"

namespace content {

namespace background_fetch {

// Gets a pending request for a given registration, and moves it
// to an active state.
class StartNextPendingRequestTask : public DatabaseTask {
 public:
  using NextRequestCallback =
      base::OnceCallback<void(scoped_refptr<BackgroundFetchRequestInfo>)>;

  StartNextPendingRequestTask(
      BackgroundFetchDataManager* data_manager,
      int64_t service_worker_registration_id,
      std::unique_ptr<proto::BackgroundFetchMetadata> metadata,
      NextRequestCallback callback);

  ~StartNextPendingRequestTask() override;

  // DatabaseTask implementation:
  void Start() override;

 private:
  void GetPendingRequests();

  void DidGetPendingRequests(const std::vector<std::string>& data,
                             ServiceWorkerStatusCode status);

  void DidFindActiveRequest(const std::vector<std::string>& data,
                            ServiceWorkerStatusCode status);

  void CreateAndStoreActiveRequest();

  void DidStoreActiveRequest(ServiceWorkerStatusCode status);

  void StartDownload();

  void DidDeletePendingRequest(ServiceWorkerStatusCode status);

  int64_t service_worker_registration_id_;
  std::unique_ptr<proto::BackgroundFetchMetadata> metadata_;
  NextRequestCallback callback_;

  // protos don't support move semantics, so these class members will be used
  // to avoid unnecessary copying within callbacks.
  proto::BackgroundFetchPendingRequest pending_request_;
  proto::BackgroundFetchActiveRequest active_request_;

  base::WeakPtrFactory<StartNextPendingRequestTask>
      weak_factory_;  // Keep as last.

  DISALLOW_COPY_AND_ASSIGN(StartNextPendingRequestTask);
};

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_START_NEXT_PENDING_REQUEST_TASK_H_
