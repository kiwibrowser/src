// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/storage/mark_registration_for_deletion_task.h"

#include <utility>

#include "content/browser/background_fetch/background_fetch.pb.h"
#include "content/browser/background_fetch/background_fetch_data_manager.h"
#include "content/browser/background_fetch/storage/database_helpers.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"

namespace content {

namespace background_fetch {

MarkRegistrationForDeletionTask::MarkRegistrationForDeletionTask(
    BackgroundFetchDataManager* data_manager,
    const BackgroundFetchRegistrationId& registration_id,
    HandleBackgroundFetchErrorCallback callback)
    : DatabaseTask(data_manager),
      registration_id_(registration_id),
      callback_(std::move(callback)),
      weak_factory_(this) {}

MarkRegistrationForDeletionTask::~MarkRegistrationForDeletionTask() = default;

void MarkRegistrationForDeletionTask::Start() {
  // Look up if there is already an active |unique_id| entry for this
  // |developer_id|.
  service_worker_context()->GetRegistrationUserData(
      registration_id_.service_worker_registration_id(),
      {ActiveRegistrationUniqueIdKey(registration_id_.developer_id()),
       RegistrationKey(registration_id_.unique_id())},
      base::BindOnce(&MarkRegistrationForDeletionTask::DidGetActiveUniqueId,
                     weak_factory_.GetWeakPtr()));
}

void MarkRegistrationForDeletionTask::DidGetActiveUniqueId(
    const std::vector<std::string>& data,
    ServiceWorkerStatusCode status) {
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kOk:
      break;
    case DatabaseStatus::kNotFound:
      std::move(callback_).Run(blink::mojom::BackgroundFetchError::INVALID_ID);
      Finished();  // Destroys |this|.
      return;
    case DatabaseStatus::kFailed:
      std::move(callback_).Run(
          blink::mojom::BackgroundFetchError::STORAGE_ERROR);
      Finished();  // Destroys |this|.
      return;
  }

  DCHECK_EQ(2u, data.size());

  // If the |unique_id| does not match, then the registration identified by
  // |registration_id_.unique_id()| was already deactivated.
  if (data[0] != registration_id_.unique_id()) {
    std::move(callback_).Run(blink::mojom::BackgroundFetchError::INVALID_ID);
    Finished();  // Destroys |this|.
    return;
  }

  proto::BackgroundFetchMetadata metadata_proto;
  if (metadata_proto.ParseFromString(data[1])) {
    // Mark registration as no longer active. Also deletes pending request
    // keys, since those are globally sorted and requests within deactivated
    // registrations are no longer eligible to be started. Pending request
    // keys are not required by GetRegistration.
    service_worker_context()->ClearRegistrationUserDataByKeyPrefixes(
        registration_id_.service_worker_registration_id(),
        {ActiveRegistrationUniqueIdKey(registration_id_.developer_id()),
         PendingRequestKeyPrefix(registration_id_.unique_id())},
        base::BindOnce(&MarkRegistrationForDeletionTask::DidDeactivate,
                       weak_factory_.GetWeakPtr()));
  } else {
    NOTREACHED() << "Database is corrupt";  // TODO(crbug.com/780027): Nuke it.
  }
}

void MarkRegistrationForDeletionTask::DidDeactivate(
    ServiceWorkerStatusCode status) {
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kOk:
    case DatabaseStatus::kNotFound:
      break;
    case DatabaseStatus::kFailed:
      std::move(callback_).Run(
          blink::mojom::BackgroundFetchError::STORAGE_ERROR);
      Finished();  // Destroys |this|.
      return;
  }

  // If CleanupTask runs after this, it shouldn't clean up the
  // |unique_id| as there may still be JavaScript references to it.
  ref_counted_unique_ids().emplace(registration_id_.unique_id());

  std::move(callback_).Run(blink::mojom::BackgroundFetchError::NONE);
  Finished();  // Destroys |this|.
}

}  // namespace background_fetch

}  // namespace content
