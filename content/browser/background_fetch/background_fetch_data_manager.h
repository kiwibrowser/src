// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DATA_MANAGER_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DATA_MANAGER_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/queue.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "content/browser/background_fetch/background_fetch.pb.h"
#include "content/browser/background_fetch/background_fetch_registration_id.h"
#include "content/browser/background_fetch/background_fetch_scheduler.h"
#include "content/browser/background_fetch/storage/database_task.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/modules/background_fetch/background_fetch.mojom.h"
#include "url/origin.h"

namespace storage {
class BlobDataHandle;
}

namespace content {

class BackgroundFetchRequestInfo;
struct BackgroundFetchSettledFetch;
class BrowserContext;
class ChromeBlobStorageContext;
class ServiceWorkerContextWrapper;

// The BackgroundFetchDataManager is a wrapper around persistent storage (the
// Service Worker database), exposing APIs for the read and write queries needed
// for Background Fetch.
//
// There must only be a single instance of this class per StoragePartition, and
// it must only be used on the IO thread, since it relies on there being no
// other code concurrently reading/writing the Background Fetch keys of the same
// Service Worker database (except for deletions, e.g. it's safe for the Service
// Worker code to remove a ServiceWorkerRegistration and all its keys).
//
// Storage schema is documented in storage/README.md
class CONTENT_EXPORT BackgroundFetchDataManager
    : public BackgroundFetchScheduler::RequestProvider {
 public:
  using SettledFetchesCallback = base::OnceCallback<void(
      blink::mojom::BackgroundFetchError,
      bool /* background_fetch_succeeded */,
      std::vector<BackgroundFetchSettledFetch>,
      std::vector<std::unique_ptr<storage::BlobDataHandle>>)>;
  using GetMetadataCallback =
      base::OnceCallback<void(blink::mojom::BackgroundFetchError,
                              std::unique_ptr<proto::BackgroundFetchMetadata>)>;
  using GetRegistrationCallback =
      base::OnceCallback<void(blink::mojom::BackgroundFetchError,
                              std::unique_ptr<BackgroundFetchRegistration>)>;
  using NextRequestCallback =
      base::OnceCallback<void(scoped_refptr<BackgroundFetchRequestInfo>)>;
  using NumRequestsCallback = base::OnceCallback<void(size_t)>;

  BackgroundFetchDataManager(
      BrowserContext* browser_context,
      scoped_refptr<ServiceWorkerContextWrapper> service_worker_context);

  ~BackgroundFetchDataManager() override;

  // Creates and stores a new registration with the given properties. Will
  // invoke the |callback| when the registration has been created, which may
  // fail due to invalid input or storage errors.
  void CreateRegistration(
      const BackgroundFetchRegistrationId& registration_id,
      const std::vector<ServiceWorkerFetchRequest>& requests,
      const BackgroundFetchOptions& options,
      const SkBitmap& icon,
      GetRegistrationCallback callback);

  // Get the BackgroundFetchMetadata.
  void GetMetadata(int64_t service_worker_registration_id,
                   const url::Origin& origin,
                   const std::string& developer_id,
                   GetMetadataCallback callback);

  // Get the BackgroundFetchRegistration.
  void GetRegistration(int64_t service_worker_registration_id,
                       const url::Origin& origin,
                       const std::string& developer_id,
                       GetRegistrationCallback callback);

  // Updates the UI values for a Background Fetch registration.
  void UpdateRegistrationUI(
      const BackgroundFetchRegistrationId& registration_id,
      const std::string& title,
      blink::mojom::BackgroundFetchService::UpdateUICallback callback);

  // Reads all settled fetches for the given |registration_id|. Both the Request
  // and Response objects will be initialised based on the stored data. Will
  // invoke the |callback| when the list of fetches has been compiled.
  void GetSettledFetchesForRegistration(
      const BackgroundFetchRegistrationId& registration_id,
      SettledFetchesCallback callback);

  // Marks that the backgroundfetched/backgroundfetchfail/backgroundfetchabort
  // event is being dispatched. It's not possible to call DeleteRegistration at
  // this point as JavaScript may hold a reference to a
  // BackgroundFetchRegistration object and we need to keep the corresponding
  // data around until the last such reference is released (or until shutdown).
  // We can't just move the Background Fetch registration's data to RAM as it
  // might consume too much memory. So instead this step disassociates the
  // |developer_id| from the |unique_id|, so that existing JS objects with a
  // reference to |unique_id| can still access the data, but it can no longer be
  // reached using GetIds or GetRegistration.
  void MarkRegistrationForDeletion(
      const BackgroundFetchRegistrationId& registration_id,
      HandleBackgroundFetchErrorCallback callback);

  // Deletes the registration identified by |registration_id|. Should only be
  // called once the refcount of JavaScript BackgroundFetchRegistration objects
  // referring to this registration drops to zero. Will invoke the |callback|
  // when the registration has been deleted from storage.
  void DeleteRegistration(const BackgroundFetchRegistrationId& registration_id,
                          HandleBackgroundFetchErrorCallback callback);

  // List all Background Fetch registration |developer_id|s for a Service
  // Worker.
  void GetDeveloperIdsForServiceWorker(
      int64_t service_worker_registration_id,
      const url::Origin& origin,
      blink::mojom::BackgroundFetchService::GetDeveloperIdsCallback callback);

  // Gets the number of fetch requests that have been completed for a given
  // registration.
  void GetNumCompletedRequests(
      const BackgroundFetchRegistrationId& registration_id,
      NumRequestsCallback callback);

  // BackgroundFetchScheduler::RequestProvider implementation:
  void PopNextRequest(const BackgroundFetchRegistrationId& registration_id,
                      NextRequestCallback callback) override;

  void MarkRequestAsComplete(
      const BackgroundFetchRegistrationId& registration_id,
      BackgroundFetchRequestInfo* request,
      BackgroundFetchScheduler::MarkedCompleteCallback callback) override;

  // TODO(rayankans): Move this function to MarkRequestCompleteTask after
  // non-persistent background fetch support is removed.
  bool FillServiceWorkerResponse(const BackgroundFetchRequestInfo& request,
                                 const url::Origin& origin,
                                 ServiceWorkerResponse* response);

 private:
  FRIEND_TEST_ALL_PREFIXES(BackgroundFetchDataManagerTest, Cleanup);
  friend class BackgroundFetchDataManagerTest;
  friend class background_fetch::DatabaseTask;

  class RegistrationData;

  void AddStartNextPendingRequestTask(
      int64_t service_worker_registration_id,
      NextRequestCallback callback,
      blink::mojom::BackgroundFetchError error,
      std::unique_ptr<proto::BackgroundFetchMetadata> metadata);

  void AddDatabaseTask(std::unique_ptr<background_fetch::DatabaseTask> task);

  void OnDatabaseTaskFinished(background_fetch::DatabaseTask* task);

  // Returns true if not aborted/completed/failed.
  bool IsActive(const BackgroundFetchRegistrationId& registration_id);

  void Cleanup();

  scoped_refptr<ServiceWorkerContextWrapper> service_worker_context_;

  // The blob storage request with which response information will be stored.
  scoped_refptr<ChromeBlobStorageContext> blob_storage_context_;

  // Map from {service_worker_registration_id, origin, developer_id} tuples to
  // the |unique_id|s of active background fetch registrations (not
  // completed/failed/aborted, so there will never be more than one entry for a
  // given key).
  std::map<std::tuple<int64_t, url::Origin, std::string>, std::string>
      active_registration_unique_ids_;

  // Map from the |unique_id|s of known (but possibly inactive) background fetch
  // registrations to their associated data.
  std::map<std::string, std::unique_ptr<RegistrationData>> registrations_;

  // Pending database operations, serialized to ensure consistency.
  // Invariant: the frontmost task, if any, has already been started.
  base::queue<std::unique_ptr<background_fetch::DatabaseTask>> database_tasks_;

  // The |unique_id|s of registrations that have been deactivated since the
  // browser was last started. They will be automatically deleted when the
  // refcount of JavaScript objects that refers to them goes to zero, unless
  // the browser is shutdown first.
  std::set<std::string> ref_counted_unique_ids_;

  base::WeakPtrFactory<BackgroundFetchDataManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchDataManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DATA_MANAGER_H_
