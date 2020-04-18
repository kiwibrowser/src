// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/storage/delete_registration_task.h"

#include <utility>

#include "content/browser/background_fetch/background_fetch.pb.h"
#include "content/browser/background_fetch/storage/database_helpers.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"

namespace content {

namespace background_fetch {

namespace {
#if DCHECK_IS_ON()
// Checks that the |ActiveRegistrationUniqueIdKey| either does not exist, or is
// associated with a different |unique_id| than the given one which should have
// been already marked for deletion.
void DCheckRegistrationNotActive(const std::string& unique_id,
                                 const std::vector<std::string>& data,
                                 ServiceWorkerStatusCode status) {
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kOk:
      DCHECK_EQ(1u, data.size());
      DCHECK_NE(unique_id, data[0])
          << "Must call MarkRegistrationForDeletion before DeleteRegistration";
      return;
    case DatabaseStatus::kFailed:
      return;  // TODO(crbug.com/780025): Consider logging failure to UMA.
    case DatabaseStatus::kNotFound:
      return;
  }
}
#endif  // DCHECK_IS_ON()
}  // namespace

DeleteRegistrationTask::DeleteRegistrationTask(
    BackgroundFetchDataManager* data_manager,
    int64_t service_worker_registration_id,
    const std::string& unique_id,
    HandleBackgroundFetchErrorCallback callback)
    : DatabaseTask(data_manager),
      service_worker_registration_id_(service_worker_registration_id),
      unique_id_(unique_id),
      callback_(std::move(callback)),
      weak_factory_(this) {}

DeleteRegistrationTask::~DeleteRegistrationTask() = default;

void DeleteRegistrationTask::Start() {
#if DCHECK_IS_ON()
  // Get the registration |developer_id| to check it was deactivated.
  service_worker_context()->GetRegistrationUserData(
      service_worker_registration_id_, {RegistrationKey(unique_id_)},
      base::BindOnce(&DeleteRegistrationTask::DidGetRegistration,
                     weak_factory_.GetWeakPtr()));
#else
  DidGetRegistration({}, SERVICE_WORKER_OK);
#endif  // DCHECK_IS_ON()
}

void DeleteRegistrationTask::DidGetRegistration(
    const std::vector<std::string>& data,
    ServiceWorkerStatusCode status) {
#if DCHECK_IS_ON()
  if (ToDatabaseStatus(status) == DatabaseStatus::kOk) {
    DCHECK_EQ(1u, data.size());
    proto::BackgroundFetchMetadata metadata_proto;
    if (metadata_proto.ParseFromString(data[0]) &&
        metadata_proto.registration().has_developer_id()) {
      service_worker_context()->GetRegistrationUserData(
          service_worker_registration_id_,
          {ActiveRegistrationUniqueIdKey(
              metadata_proto.registration().developer_id())},
          base::BindOnce(&DCheckRegistrationNotActive, unique_id_));
    } else {
      NOTREACHED()
          << "Database is corrupt";  // TODO(crbug.com/780027): Nuke it.
    }
  } else {
    // TODO(crbug.com/780025): Log failure to UMA.
  }
#endif  // DCHECK_IS_ON()

  service_worker_context()->ClearRegistrationUserDataByKeyPrefixes(
      service_worker_registration_id_,
      {RegistrationKey(unique_id_), TitleKey(unique_id_)},
      base::BindOnce(&DeleteRegistrationTask::DidDeleteRegistration,
                     weak_factory_.GetWeakPtr()));
}

void DeleteRegistrationTask::DidDeleteRegistration(
    ServiceWorkerStatusCode status) {
  switch (ToDatabaseStatus(status)) {
    case DatabaseStatus::kOk:
    case DatabaseStatus::kNotFound:
      std::move(callback_).Run(blink::mojom::BackgroundFetchError::NONE);
      Finished();  // Destroys |this|.
      return;
    case DatabaseStatus::kFailed:
      std::move(callback_).Run(
          blink::mojom::BackgroundFetchError::STORAGE_ERROR);
      Finished();  // Destroys |this|.
      return;
  }
}

}  // namespace background_fetch

}  // namespace content
