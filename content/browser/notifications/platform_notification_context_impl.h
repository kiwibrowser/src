// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_CONTEXT_IMPL_H_
#define CONTENT_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_CONTEXT_IMPL_H_

#include <stdint.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/browser/notifications/notification_id_generator.h"
#include "content/browser/service_worker/service_worker_context_core_observer.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/platform_notification_context.h"
#include "third_party/blink/public/platform/modules/notifications/notification_service.mojom.h"

class GURL;

namespace base {
class SequencedTaskRunner;
}

namespace url {
class Origin;
}

namespace content {

class BlinkNotificationServiceImpl;
class BrowserContext;
class NotificationDatabase;
struct NotificationDatabaseData;
class ResourceContext;
class ServiceWorkerContextWrapper;

// Implementation of the Web Notification storage context. The public methods
// defined in this interface must only be called on the IO thread unless
// otherwise specified.
class CONTENT_EXPORT PlatformNotificationContextImpl
    : public PlatformNotificationContext,
      public ServiceWorkerContextCoreObserver {
 public:
  // Constructs a new platform notification context. If |path| is non-empty, the
  // database will be initialized in the "Platform Notifications" subdirectory
  // of |path|. Otherwise, the database will be initialized in memory. The
  // constructor must only be called on the IO thread.
  PlatformNotificationContextImpl(
      const base::FilePath& path,
      BrowserContext* browser_context,
      const scoped_refptr<ServiceWorkerContextWrapper>& service_worker_context);

  // To be called on the UI thread to initialize the instance.
  void Initialize();

  // To be called on the UI thread when the context is being shut down.
  void Shutdown();

  // Creates a BlinkNotificationServiceImpl that is owned by this context. Must
  // be called on the UI thread, although the service will be created on and
  // bound to the IO thread.
  void CreateService(int render_process_id,
                     const url::Origin& origin,
                     blink::mojom::NotificationServiceRequest request);

  // Removes |service| from the list of owned services, for example because the
  // Mojo pipe disconnected. Must be called on the IO thread.
  void RemoveService(BlinkNotificationServiceImpl* service);

  // Returns the notification Id generator owned by the context.
  NotificationIdGenerator* notification_id_generator() {
    return &notification_id_generator_;
  }

  // PlatformNotificationContext implementation.
  void ReadNotificationData(const std::string& notification_id,
                            const GURL& origin,
                            const ReadResultCallback& callback) override;
  void WriteNotificationData(const GURL& origin,
                             const NotificationDatabaseData& database_data,
                             const WriteResultCallback& callback) override;
  void DeleteNotificationData(const std::string& notification_id,
                              const GURL& origin,
                              const DeleteResultCallback& callback) override;
  void ReadAllNotificationDataForServiceWorkerRegistration(
      const GURL& origin,
      int64_t service_worker_registration_id,
      const ReadAllResultCallback& callback) override;

  // ServiceWorkerContextCoreObserver implementation.
  void OnRegistrationDeleted(int64_t registration_id,
                             const GURL& pattern) override;
  void OnStorageWiped() override;

 private:
  friend class PlatformNotificationContextTest;

  ~PlatformNotificationContextImpl() override;

  void DidGetNotificationsOnUI(
      std::unique_ptr<std::set<std::string>> displayed_notifications,
      bool supports_synchronization);
  void InitializeOnIO(
      std::unique_ptr<std::set<std::string>> displayed_notifications,
      bool supports_synchronization);
  void ShutdownOnIO();
  void CreateServiceOnIO(
      int render_process_id,
      const url::Origin& origin,
      ResourceContext* resource_context,
      mojo::InterfaceRequest<blink::mojom::NotificationService> request);

  // Initializes the database if neccesary. Must be called on the IO thread.
  // |success_closure| will be invoked on a the |task_runner_| thread when
  // everything is available, or |failure_closure_| will be invoked on the
  // IO thread when initialization fails.
  void LazyInitialize(const base::Closure& success_closure,
                      const base::Closure& failure_closure);

  // Opens the database. Must be called on the |task_runner_| thread. When the
  // database has been opened, |success_closure| will be invoked on the task
  // thread, otherwise |failure_closure_| will be invoked on the IO thread.
  void OpenDatabase(const base::Closure& success_closure,
                    const base::Closure& failure_closure);

  // Actually reads the notification data from the database. Must only be
  // called on the |task_runner_| thread. |callback| will be invoked on the
  // IO thread when the operation has completed.
  void DoReadNotificationData(const std::string& notification_id,
                              const GURL& origin,
                              const ReadResultCallback& callback);

  // Updates the database (and the result callback) based on
  // |displayed_notifications| if |supports_synchronization|.
  void SynchronizeDisplayedNotificationsForServiceWorkerRegistrationOnUI(
      const GURL& origin,
      int64_t service_worker_registration_id,
      const ReadAllResultCallback& callback,
      std::unique_ptr<std::set<std::string>> displayed_notifications,
      bool supports_synchronization);

  // Updates the database (and the result callback) based on
  // |displayed_notifications| if |supports_synchronization|.
  void SynchronizeDisplayedNotificationsForServiceWorkerRegistrationOnIO(
      const GURL& origin,
      int64_t service_worker_registration_id,
      const ReadAllResultCallback& callback,
      std::unique_ptr<std::set<std::string>> displayed_notifications,
      bool supports_synchronization);

  // Actually reads all notification data from the database. Must only be
  // called on the |task_runner_| thread. |callback| will be invoked on the
  // IO thread when the operation has completed.
  void DoReadAllNotificationDataForServiceWorkerRegistration(
      const GURL& origin,
      int64_t service_worker_registration_id,
      const ReadAllResultCallback& callback,
      std::unique_ptr<std::set<std::string>> displayed_notifications,
      bool supports_synchronization);

  // Actually writes the notification database to the database. Must only be
  // called on the |task_runner_| thread. |callback| will be invoked on the
  // IO thread when the operation has completed.
  void DoWriteNotificationData(const GURL& origin,
                               const NotificationDatabaseData& database_data,
                               const WriteResultCallback& callback);

  // Actually deletes the notification information from the database. Must only
  // be called on the |task_runner_| thread. |callback| will be invoked on the
  // IO thread when the operation has completed.
  void DoDeleteNotificationData(const std::string& notification_id,
                                const GURL& origin,
                                const DeleteResultCallback& callback);

  // Deletes all notifications associated with |service_worker_registration_id|
  // belonging to |origin|. Must be called on the |task_runner_| thread.
  void DoDeleteNotificationsForServiceWorkerRegistration(
      const GURL& origin,
      int64_t service_worker_registration_id);

  // Destroys the database regardless of its initialization status. This method
  // must only be called on the |task_runner_| thread. Returns if the directory
  // the database was stored in could be emptied.
  bool DestroyDatabase();

  // Returns the path in which the database should be initialized. May be empty.
  base::FilePath GetDatabasePath() const;

  // Sets the task runner to use for testing purposes.
  void SetTaskRunnerForTesting(
      const scoped_refptr<base::SequencedTaskRunner>& task_runner);

  base::FilePath path_;
  BrowserContext* browser_context_;

  scoped_refptr<ServiceWorkerContextWrapper> service_worker_context_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::unique_ptr<NotificationDatabase> database_;

  NotificationIdGenerator notification_id_generator_;

  // Indicates whether the database should be pruned when it's opened.
  bool prune_database_on_open_ = false;

  // The notification services are owned by the platform context, and will be
  // removed when either this class is destroyed or the Mojo pipe disconnects.
  std::vector<std::unique_ptr<BlinkNotificationServiceImpl>> services_;

  DISALLOW_COPY_AND_ASSIGN(PlatformNotificationContextImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_CONTEXT_IMPL_H_
