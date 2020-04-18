// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_MARK_REGISTRATION_FOR_DELETION_TASK_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_MARK_REGISTRATION_FOR_DELETION_TASK_H_

#include <string>
#include <vector>

#include "content/browser/background_fetch/storage/database_task.h"
#include "content/common/service_worker/service_worker_status_code.h"

namespace content {

namespace background_fetch {

// Marks Background Fetch registrations for deletion from the database. This is
// used when some parts of the registration may still be in use and cannot be
// completely removed.
class MarkRegistrationForDeletionTask : public background_fetch::DatabaseTask {
 public:
  MarkRegistrationForDeletionTask(
      BackgroundFetchDataManager* data_manager,
      const BackgroundFetchRegistrationId& registration_id,
      HandleBackgroundFetchErrorCallback callback);

  ~MarkRegistrationForDeletionTask() override;

  void Start() override;

 private:
  void DidGetActiveUniqueId(const std::vector<std::string>& data,
                            ServiceWorkerStatusCode status);

  void DidDeactivate(ServiceWorkerStatusCode status);

  BackgroundFetchRegistrationId registration_id_;
  HandleBackgroundFetchErrorCallback callback_;

  base::WeakPtrFactory<MarkRegistrationForDeletionTask>
      weak_factory_;  // Keep as last.

  DISALLOW_COPY_AND_ASSIGN(MarkRegistrationForDeletionTask);
};

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_STORAGE_MARK_REGISTRATION_FOR_DELETION_TASK_H_
