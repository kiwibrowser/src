// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_MARK_REQUEST_COMPLETE_TASK_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_MARK_REQUEST_COMPLETE_TASK_H_

#include "base/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "content/browser/background_fetch/background_fetch.pb.h"
#include "content/browser/background_fetch/background_fetch_request_info.h"
#include "content/browser/background_fetch/storage/database_task.h"
#include "content/common/service_worker/service_worker_status_code.h"

namespace content {

namespace background_fetch {

// Moves the request from an active state to a complete state. Stores the
// download response in cache storage.
class MarkRequestCompleteTask : public DatabaseTask {
 public:
  using MarkedCompleteCallback = base::OnceCallback<void()>;

  MarkRequestCompleteTask(
      BackgroundFetchDataManager* data_manager,
      BackgroundFetchRegistrationId registration_id,
      scoped_refptr<BackgroundFetchRequestInfo> request_info,
      MarkedCompleteCallback callback);

  ~MarkRequestCompleteTask() override;

  // DatabaseTask implementation:
  void Start() override;

 private:
  void StoreResponse();

  void CreateAndStoreCompletedRequest(bool succeeded);

  void DidStoreCompletedRequest(ServiceWorkerStatusCode status);

  void DidDeleteActiveRequest(ServiceWorkerStatusCode status);

  BackgroundFetchRegistrationId registration_id_;
  scoped_refptr<BackgroundFetchRequestInfo> request_info_;
  MarkedCompleteCallback callback_;

  proto::BackgroundFetchCompletedRequest completed_request_;

  base::WeakPtrFactory<MarkRequestCompleteTask> weak_factory_;  // Keep as last.

  DISALLOW_COPY_AND_ASSIGN(MarkRequestCompleteTask);
};

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_MARK_REQUEST_COMPLETE_TASK_H_