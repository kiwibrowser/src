// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_MANAGER_H_
#define CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_MANAGER_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/clock.h"
#include "content/browser/background_sync/background_sync.pb.h"
#include "content/browser/background_sync/background_sync_registration.h"
#include "content/browser/background_sync/background_sync_status.h"
#include "content/browser/cache_storage/cache_storage_scheduler.h"
#include "content/browser/service_worker/service_worker_context_core_observer.h"
#include "content/browser/service_worker/service_worker_storage.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/public/browser/background_sync_parameters.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/blink/public/platform/modules/background_sync/background_sync.mojom.h"
#include "third_party/blink/public/platform/modules/permissions/permission_status.mojom.h"
#include "url/gurl.h"

namespace blink {
namespace mojom {
enum class PermissionStatus;
}
}

namespace content {

class BackgroundSyncNetworkObserver;
class ServiceWorkerContextWrapper;

// BackgroundSyncManager manages and stores the set of background sync
// registrations across all registered service workers for a profile.
// Registrations are stored along with their associated Service Worker
// registration in ServiceWorkerStorage. If the ServiceWorker is unregistered,
// the sync registrations are removed. This class must be run on the IO
// thread. The asynchronous methods are executed sequentially.
class CONTENT_EXPORT BackgroundSyncManager
    : public ServiceWorkerContextCoreObserver {
 public:
  using BoolCallback = base::OnceCallback<void(bool)>;
  using StatusAndRegistrationCallback =
      base::OnceCallback<void(BackgroundSyncStatus,
                              std::unique_ptr<BackgroundSyncRegistration>)>;
  using StatusAndRegistrationsCallback = base::OnceCallback<void(
      BackgroundSyncStatus,
      std::vector<std::unique_ptr<BackgroundSyncRegistration>>)>;

  static std::unique_ptr<BackgroundSyncManager> Create(
      scoped_refptr<ServiceWorkerContextWrapper> service_worker_context);
  ~BackgroundSyncManager() override;

  // Stores the given background sync registration and adds it to the scheduling
  // queue. It will overwrite an existing registration with the same tag unless
  // they're identical (save for the id). Calls |callback| with
  // BACKGROUND_SYNC_STATUS_OK and the accepted registration on success.
  // The accepted registration will have a unique id. It may also have altered
  // parameters if the user or UA chose different parameters than those
  // supplied.
  void Register(int64_t sw_registration_id,
                const BackgroundSyncRegistrationOptions& options,
                StatusAndRegistrationCallback callback);

  // Finds the background sync registrations associated with
  // |sw_registration_id|. Calls |callback| with BACKGROUND_SYNC_STATUS_OK on
  // success.
  void GetRegistrations(int64_t sw_registration_id,
                        StatusAndRegistrationsCallback callback);

  // ServiceWorkerContextCoreObserver overrides.
  void OnRegistrationDeleted(int64_t sw_registration_id,
                             const GURL& pattern) override;
  void OnStorageWiped() override;

  // Sets the max number of sync attempts after any pending operations have
  // completed.
  void SetMaxSyncAttemptsForTesting(int max_attempts);

  BackgroundSyncNetworkObserver* GetNetworkObserverForTesting() {
    return network_observer_.get();
  }

  void set_clock(base::Clock* clock) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    clock_ = clock;
  }

  // Called from DevTools
  void EmulateDispatchSyncEvent(
      const std::string& tag,
      scoped_refptr<ServiceWorkerVersion> active_version,
      bool last_chance,
      ServiceWorkerVersion::StatusCallback callback);

  // Called from DevTools to toggle service worker "offline" status
  void EmulateServiceWorkerOffline(int64_t service_worker_id, bool is_offline);

 protected:
  explicit BackgroundSyncManager(
      scoped_refptr<ServiceWorkerContextWrapper> context);

  // Init must be called before any public member function. Only call it once.
  void Init();

  // The following methods are virtual for testing.
  virtual void StoreDataInBackend(
      int64_t sw_registration_id,
      const GURL& origin,
      const std::string& backend_key,
      const std::string& data,
      ServiceWorkerStorage::StatusCallback callback);
  virtual void GetDataFromBackend(
      const std::string& backend_key,
      ServiceWorkerStorage::GetUserDataForAllRegistrationsCallback callback);
  virtual void DispatchSyncEvent(
      const std::string& tag,
      scoped_refptr<ServiceWorkerVersion> active_version,
      bool last_chance,
      ServiceWorkerVersion::StatusCallback callback);
  virtual void ScheduleDelayedTask(base::OnceClosure callback,
                                   base::TimeDelta delay);
  virtual void HasMainFrameProviderHost(const GURL& origin,
                                        BoolCallback callback);

 private:
  friend class TestBackgroundSyncManager;
  friend class BackgroundSyncManagerTest;

  struct BackgroundSyncRegistrations {
    using RegistrationMap = std::map<std::string, BackgroundSyncRegistration>;

    BackgroundSyncRegistrations();
    BackgroundSyncRegistrations(const BackgroundSyncRegistrations& other);
    ~BackgroundSyncRegistrations();

    RegistrationMap registration_map;
    BackgroundSyncRegistration::RegistrationId next_id;
    GURL origin;
  };

  using SWIdToRegistrationsMap = std::map<int64_t, BackgroundSyncRegistrations>;

  static const size_t kMaxTagLength = 10240;

  // Disable the manager. Already queued operations will abort once they start
  // to run (in their impl methods). Future operations will not queue.
  // The list of active registrations is cleared and the backend is also cleared
  // (if it's still functioning). The manager will reenable itself once it
  // receives the OnStorageWiped message or on browser restart.
  void DisableAndClearManager(base::OnceClosure callback);
  void DisableAndClearDidGetRegistrations(
      base::OnceClosure callback,
      const std::vector<std::pair<int64_t, std::string>>& user_data,
      ServiceWorkerStatusCode status);
  void DisableAndClearManagerClearedOne(base::OnceClosure barrier_closure,
                                        ServiceWorkerStatusCode status);

  // Returns the existing registration or nullptr if it cannot be found.
  BackgroundSyncRegistration* LookupActiveRegistration(
      int64_t sw_registration_id,
      const std::string& tag);

  // Write all registrations for a given |sw_registration_id| to persistent
  // storage.
  void StoreRegistrations(int64_t sw_registration_id,
                          ServiceWorkerStorage::StatusCallback callback);

  // Removes the active registration if it is in the map.
  void RemoveActiveRegistration(int64_t sw_registration_id,
                                const std::string& tag);

  void AddActiveRegistration(
      int64_t sw_registration_id,
      const GURL& origin,
      const BackgroundSyncRegistration& sync_registration);

  void InitImpl(base::OnceClosure callback);
  void InitDidGetControllerParameters(
      base::OnceClosure callback,
      std::unique_ptr<BackgroundSyncParameters> parameters);
  void InitDidGetDataFromBackend(
      base::OnceClosure callback,
      const std::vector<std::pair<int64_t, std::string>>& user_data,
      ServiceWorkerStatusCode status);

  // Register callbacks
  void RegisterCheckIfHasMainFrame(
      int64_t sw_registration_id,
      const BackgroundSyncRegistrationOptions& options,
      StatusAndRegistrationCallback callback);
  void RegisterDidCheckIfMainFrame(
      int64_t sw_registration_id,
      const BackgroundSyncRegistrationOptions& options,
      StatusAndRegistrationCallback callback,
      bool has_main_frame_client);
  void RegisterImpl(int64_t sw_registration_id,
                    const BackgroundSyncRegistrationOptions& options,
                    StatusAndRegistrationCallback callback);
  void RegisterDidAskForPermission(
      int64_t sw_registration_id,
      const BackgroundSyncRegistrationOptions& options,
      StatusAndRegistrationCallback callback,
      blink::mojom::PermissionStatus permission_status);
  void RegisterDidStore(int64_t sw_registration_id,
                        const BackgroundSyncRegistration& new_registration,
                        StatusAndRegistrationCallback callback,
                        ServiceWorkerStatusCode status);

  // GetRegistrations callbacks
  void GetRegistrationsImpl(int64_t sw_registration_id,
                            StatusAndRegistrationsCallback callback);

  bool AreOptionConditionsMet(const BackgroundSyncRegistrationOptions& options);
  bool IsRegistrationReadyToFire(const BackgroundSyncRegistration& registration,
                                 int64_t service_worker_id);

  // Determines if the browser needs to be able to run in the background (e.g.,
  // to run a pending registration or verify that a firing registration
  // completed). If background processing is required it calls out to the
  // BackgroundSyncController to enable it.
  // Assumes that all registrations in the pending state are not currently ready
  // to fire. Therefore this should not be called directly and should only be
  // called by FireReadyEvents.
  void RunInBackgroundIfNecessary();

  // FireReadyEvents scans the list of available events and fires those that are
  // ready to fire. For those that can't yet be fired, wakeup alarms are set.
  void FireReadyEvents();
  void FireReadyEventsImpl(base::OnceClosure callback);
  void FireReadyEventsDidFindRegistration(
      const std::string& tag,
      BackgroundSyncRegistration::RegistrationId registration_id,
      base::OnceClosure event_fired_callback,
      base::OnceClosure event_completed_callback,
      ServiceWorkerStatusCode service_worker_status,
      scoped_refptr<ServiceWorkerRegistration> service_worker_registration);
  void FireReadyEventsAllEventsFiring(base::OnceClosure callback);

  // Called when a sync event has completed.
  void EventComplete(
      scoped_refptr<ServiceWorkerRegistration> service_worker_registration,
      int64_t service_worker_id,
      const std::string& tag,
      base::OnceClosure callback,
      ServiceWorkerStatusCode status_code);
  void EventCompleteImpl(int64_t service_worker_id,
                         const std::string& tag,
                         ServiceWorkerStatusCode status_code,
                         base::OnceClosure callback);
  void EventCompleteDidStore(int64_t service_worker_id,
                             base::OnceClosure callback,
                             ServiceWorkerStatusCode status_code);

  // Called when all sync events have completed.
  static void OnAllSyncEventsCompleted(const base::TimeTicks& start_time,
                                       int number_of_batched_sync_events);

  // OnRegistrationDeleted callbacks
  void OnRegistrationDeletedImpl(int64_t sw_registration_id,
                                 base::OnceClosure callback);

  // OnStorageWiped callbacks
  void OnStorageWipedImpl(base::OnceClosure callback);

  void OnNetworkChanged();

  // SetMaxSyncAttempts callback
  void SetMaxSyncAttemptsImpl(int max_sync_attempts,
                              base::OnceClosure callback);

  base::OnceClosure MakeEmptyCompletion();

  ServiceWorkerStatusCode CanEmulateSyncEvent(
      scoped_refptr<ServiceWorkerVersion> active_version);

  SWIdToRegistrationsMap active_registrations_;
  CacheStorageScheduler op_scheduler_;
  scoped_refptr<ServiceWorkerContextWrapper> service_worker_context_;

  std::unique_ptr<BackgroundSyncParameters> parameters_;

  // True if the manager is disabled and registrations should fail.
  bool disabled_;

  // The number of registrations currently in the firing state.
  int num_firing_registrations_;

  base::CancelableCallback<void()> delayed_sync_task_;

  std::unique_ptr<BackgroundSyncNetworkObserver> network_observer_;

  base::Clock* clock_;

  std::map<int64_t, int> emulated_offline_sw_;

  base::WeakPtrFactory<BackgroundSyncManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundSyncManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_MANAGER_H_
