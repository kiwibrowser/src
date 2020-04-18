// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_CONTEXT_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_CONTEXT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/background_fetch/background_fetch_data_manager.h"
#include "content/browser/background_fetch/background_fetch_delegate_proxy.h"
#include "content/browser/background_fetch/background_fetch_event_dispatcher.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/blink/public/platform/modules/background_fetch/background_fetch.mojom.h"

namespace storage {
class BlobDataHandle;
}

namespace content {

class BackgroundFetchJobController;
struct BackgroundFetchOptions;
class BackgroundFetchRegistrationId;
class BackgroundFetchRegistrationNotifier;
class BackgroundFetchScheduler;
class BrowserContext;
class ServiceWorkerContextWrapper;
struct ServiceWorkerFetchRequest;

// The BackgroundFetchContext is the central moderator of ongoing background
// fetch requests from the Mojo service and from other callers.
// Background Fetch requests function similarly to normal fetches except that
// they are persistent across Chromium or service worker shutdown.
class CONTENT_EXPORT BackgroundFetchContext
    : public base::RefCountedThreadSafe<BackgroundFetchContext,
                                        BrowserThread::DeleteOnIOThread> {
 public:
  // The BackgroundFetchContext will watch the ServiceWorkerContextWrapper so
  // that it can respond to service worker events such as unregister.
  BackgroundFetchContext(
      BrowserContext* browser_context,
      const scoped_refptr<ServiceWorkerContextWrapper>& service_worker_context);

  // Gets the active Background Fetch registration identified by |developer_id|
  // for the given |service_worker_id| and |origin|. The |callback| will be
  // invoked with the registration when it has been retrieved.
  void GetRegistration(
      int64_t service_worker_registration_id,
      const url::Origin& origin,
      const std::string& developer_id,
      blink::mojom::BackgroundFetchService::GetRegistrationCallback callback);

  // Gets all the Background Fetch registration |developer_id|s for a Service
  // Worker and invokes |callback| with that list.
  void GetDeveloperIdsForServiceWorker(
      int64_t service_worker_registration_id,
      const url::Origin& origin,
      blink::mojom::BackgroundFetchService::GetDeveloperIdsCallback callback);

  // Starts a Background Fetch for the |registration_id|. The |requests| will be
  // asynchronously fetched. The |callback| will be invoked when the fetch has
  // been registered, or an error occurred that prevents it from doing so.
  void StartFetch(const BackgroundFetchRegistrationId& registration_id,
                  const std::vector<ServiceWorkerFetchRequest>& requests,
                  const BackgroundFetchOptions& options,
                  const SkBitmap& icon,
                  blink::mojom::BackgroundFetchService::FetchCallback callback);

  // Gets display size for the icon for Background Fetch UI.
  void GetIconDisplaySize(
      blink::mojom::BackgroundFetchService::GetIconDisplaySizeCallback
          callback);

  // Aborts the Background Fetch for the |registration_id|. The callback will be
  // invoked with INVALID_ID if the registration has already completed or
  // aborted, STORAGE_ERROR if an I/O error occurs, or NONE for success.
  void Abort(const BackgroundFetchRegistrationId& registration_id,
             blink::mojom::BackgroundFetchService::AbortCallback callback);

  // Registers the |observer| to be notified of progress events for the
  // registration identified by |unique_id| whenever they happen. The observer
  // will unregister itself when the Mojo endpoint goes away.
  void AddRegistrationObserver(
      const std::string& unique_id,
      blink::mojom::BackgroundFetchRegistrationObserverPtr observer);

  // Updates the title of the Background Fetch identified by |registration_id|.
  // The |callback| will be invoked when the title has been updated, or an error
  // occurred that prevents it from doing so.
  void UpdateUI(
      const BackgroundFetchRegistrationId& registration_id,
      const std::string& title,
      blink::mojom::BackgroundFetchService::UpdateUICallback callback);

 private:
  friend class base::DeleteHelper<BackgroundFetchContext>;
  friend struct BrowserThread::DeleteOnThread<BrowserThread::IO>;
  friend class base::RefCountedThreadSafe<BackgroundFetchContext,
                                          BrowserThread::DeleteOnIOThread>;

  ~BackgroundFetchContext();

  // Creates a new Job Controller for the given |registration_id| and |options|,
  // which will start fetching the files that are part of the registration.
  void CreateController(const BackgroundFetchRegistrationId& registration_id,
                        const BackgroundFetchOptions& options,
                        const SkBitmap& icon,
                        size_t num_requests,
                        const BackgroundFetchRegistration& registration,
                        base::OnceClosure done_closure);

  // Initializes the new Job Controller.
  void InitializeController(
      const std::string& unique_id,
      std::unique_ptr<BackgroundFetchJobController> controller,
      base::OnceClosure done_closure,
      size_t total_downloads,
      size_t completed_downloads);

  // Called when an existing registration has been retrieved from the data
  // manager. If the registration does not exist then |registration| is nullptr.
  void DidGetRegistration(
      blink::mojom::BackgroundFetchService::GetRegistrationCallback callback,
      blink::mojom::BackgroundFetchError error,
      std::unique_ptr<BackgroundFetchRegistration> registration);

  // Called when a new registration has been created by the data manager.
  void DidCreateRegistration(
      const BackgroundFetchRegistrationId& registration_id,
      const BackgroundFetchOptions& options,
      const SkBitmap& icon,
      size_t num_requests,
      blink::mojom::BackgroundFetchService::FetchCallback callback,
      blink::mojom::BackgroundFetchError error,
      std::unique_ptr<BackgroundFetchRegistration> registration);

  // Called when the new title has been updated in the data manager.
  void DidUpdateStoredUI(
      const std::string& unique_id,
      const std::string& title,
      blink::mojom::BackgroundFetchService::UpdateUICallback callback,
      blink::mojom::BackgroundFetchError error);

  // Called by a JobController when it finishes processing. Also used to
  // implement |Abort|.
  void DidFinishJob(
      base::OnceCallback<void(blink::mojom::BackgroundFetchError)> callback,
      const BackgroundFetchRegistrationId& registration_id,
      BackgroundFetchReasonToAbort reason_to_abort);

  // Called when the data manager finishes marking a registration as deleted.
  void DidMarkForDeletion(
      const BackgroundFetchRegistrationId& registration_id,
      BackgroundFetchReasonToAbort reason_to_abort,
      base::OnceCallback<void(blink::mojom::BackgroundFetchError)> callback,
      blink::mojom::BackgroundFetchError error);

  // Called when the sequence of settled fetches for |registration_id| have been
  // retrieved from storage, and the Service Worker event can be invoked.
  void DidGetSettledFetches(
      const BackgroundFetchRegistrationId& registration_id,
      blink::mojom::BackgroundFetchError error,
      bool background_fetch_succeeded,
      std::vector<BackgroundFetchSettledFetch> settled_fetches,
      std::vector<std::unique_ptr<storage::BlobDataHandle>> blob_data_handles);

  // Called when all processing for the |registration_id| has been finished and
  // the job is ready to be deleted. |blob_handles| are unused, but some callers
  // use it to keep blobs alive for the right duration.
  void CleanupRegistration(
      const BackgroundFetchRegistrationId& registration_id,
      const std::vector<std::unique_ptr<storage::BlobDataHandle>>&
          blob_data_handles);

  // Called when the last JavaScript BackgroundFetchRegistration object has been
  // garbage collected for a registration marked for deletion, and so it is now
  // safe to delete the underlying registration data.
  void LastObserverGarbageCollected(
      const BackgroundFetchRegistrationId& registration_id);

  // |this| is owned, indirectly, by the BrowserContext.
  BrowserContext* browser_context_;

  BackgroundFetchDataManager data_manager_;
  BackgroundFetchEventDispatcher event_dispatcher_;
  std::unique_ptr<BackgroundFetchRegistrationNotifier> registration_notifier_;
  BackgroundFetchDelegateProxy delegate_proxy_;
  std::unique_ptr<BackgroundFetchScheduler> scheduler_;

  // Map from background fetch registration |unique_id|s to active job
  // controllers. Must be destroyed before |data_manager_| and
  // |registration_notifier_|.
  std::map<std::string, std::unique_ptr<BackgroundFetchJobController>>
      job_controllers_;

  base::WeakPtrFactory<BackgroundFetchContext> weak_factory_;  // Must be last.

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchContext);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_CONTEXT_H_
