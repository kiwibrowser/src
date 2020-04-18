// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_GET_NUM_REQUESTS_TASK_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_GET_NUM_REQUESTS_TASK_H_

#include "base/callback_forward.h"
#include "content/browser/background_fetch/storage/database_task.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_status_code.h"

namespace content {

namespace background_fetch {

enum class RequestType { kAny, kPending, kActive, kCompleted };

// Gets the number of requests per registration of the given type.
class CONTENT_EXPORT GetNumRequestsTask : public DatabaseTask {
 public:
  using NumRequestsCallback = base::OnceCallback<void(size_t)>;

  GetNumRequestsTask(BackgroundFetchDataManager* data_manager,
                     const BackgroundFetchRegistrationId& registration_id,
                     RequestType type,
                     NumRequestsCallback callback);

  ~GetNumRequestsTask() override;

  // DatabaseTask implementation:
  void Start() override;

 private:
  void GetMetadata();
  void GetRequests(const std::string& key_prefix);
  void DidGetRequests(const std::vector<std::string>& data,
                      ServiceWorkerStatusCode status);

  BackgroundFetchRegistrationId registration_id_;
  RequestType type_;
  NumRequestsCallback callback_;

  base::WeakPtrFactory<GetNumRequestsTask> weak_factory_;  // Keep as last.

  DISALLOW_COPY_AND_ASSIGN(GetNumRequestsTask);
};

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_GET_NUM_REQUESTS_TASK_H_
