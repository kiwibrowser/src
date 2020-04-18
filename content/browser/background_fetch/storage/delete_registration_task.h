// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DELETE_REGISTRATION_TASK_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DELETE_REGISTRATION_TASK_H_

#include <string>
#include <vector>

#include "content/browser/background_fetch/storage/database_task.h"
#include "content/common/service_worker/service_worker_status_code.h"

namespace content {

namespace background_fetch {

// Deletes Background Fetch registration entries from the database.
class DeleteRegistrationTask : public background_fetch::DatabaseTask {
 public:
  DeleteRegistrationTask(BackgroundFetchDataManager* data_manager,
                         int64_t service_worker_registration_id,
                         const std::string& unique_id,
                         HandleBackgroundFetchErrorCallback callback);

  ~DeleteRegistrationTask() override;

  void Start() override;

 private:
  void DidGetRegistration(const std::vector<std::string>& data,
                          ServiceWorkerStatusCode status);

  void DidDeleteRegistration(ServiceWorkerStatusCode status);

  int64_t service_worker_registration_id_;
  std::string unique_id_;
  HandleBackgroundFetchErrorCallback callback_;

  base::WeakPtrFactory<DeleteRegistrationTask> weak_factory_;  // Keep as last.

  DISALLOW_COPY_AND_ASSIGN(DeleteRegistrationTask);
};

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_DELETE_REGISTRATION_TASK_H_
