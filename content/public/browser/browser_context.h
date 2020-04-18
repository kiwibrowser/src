// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
#define CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/hash_tables.h"
#include "base/supports_user_data.h"
#include "content/common/content_export.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_job_factory.h"
#include "services/service_manager/embedder/embedded_service_info.h"

#if !defined(OS_ANDROID)
#include "content/public/browser/zoom_level_delegate.h"
#endif

class GURL;

namespace base {
class FilePath;
}

namespace service_manager {
class Connector;
}

namespace storage {
class ExternalMountPoints;
}

namespace media {
class VideoDecodePerfHistory;
}

namespace net {
class URLRequestContextGetter;
}

namespace storage {
class BlobStorageContext;
class SpecialStoragePolicy;
}

namespace content {

namespace mojom {
enum class PushDeliveryStatus;
}

class BackgroundFetchDelegate;
class BackgroundSyncController;
class BlobHandle;
class BrowserPluginGuestManager;
class BrowsingDataRemover;
class BrowsingDataRemoverDelegate;
class DownloadManager;
class DownloadManagerDelegate;
class PermissionManager;
struct PushEventPayload;
class PushMessagingService;
class ResourceContext;
class ServiceManagerConnection;
class SiteInstance;
class StoragePartition;
class SSLHostStateDelegate;

// A mapping from the scheme name to the protocol handler that services its
// content.
using ProtocolHandlerMap =
    std::map<std::string,
             std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>>;

// A owning vector of protocol interceptors.
using URLRequestInterceptorScopedVector =
    std::vector<std::unique_ptr<net::URLRequestInterceptor>>;

// This class holds the context needed for a browsing session.
// It lives on the UI thread. All these methods must only be called on the UI
// thread.
class CONTENT_EXPORT BrowserContext : public base::SupportsUserData {
 public:
  static DownloadManager* GetDownloadManager(BrowserContext* browser_context);

  // Returns BrowserContext specific external mount points. It may return
  // nullptr if the context doesn't have any BrowserContext specific external
  // mount points. Currenty, non-nullptr value is returned only on ChromeOS.
  static storage::ExternalMountPoints* GetMountPoints(BrowserContext* context);

  // Returns a BrowsingDataRemover that can schedule data deletion tasks
  // for this |context|.
  static BrowsingDataRemover* GetBrowsingDataRemover(BrowserContext* context);

  // Returns a StoragePartition for the given SiteInstance. By default this will
  // create a new StoragePartition if it doesn't exist, unless |can_create| is
  // false.
  static StoragePartition* GetStoragePartition(BrowserContext* browser_context,
                                               SiteInstance* site_instance,
                                               bool can_create = true);
  static StoragePartition* GetStoragePartitionForSite(
      BrowserContext* browser_context,
      const GURL& site,
      bool can_create = true);
  using StoragePartitionCallback = base::Callback<void(StoragePartition*)>;
  static void ForEachStoragePartition(
      BrowserContext* browser_context,
      const StoragePartitionCallback& callback);
  static void AsyncObliterateStoragePartition(
      BrowserContext* browser_context,
      const GURL& site,
      const base::Closure& on_gc_required);

  // This function clears the contents of |active_paths| but does not take
  // ownership of the pointer.
  static void GarbageCollectStoragePartitions(
      BrowserContext* browser_context,
      std::unique_ptr<base::hash_set<base::FilePath>> active_paths,
      const base::Closure& done);

  static StoragePartition* GetDefaultStoragePartition(
      BrowserContext* browser_context);

  using BlobCallback = base::OnceCallback<void(std::unique_ptr<BlobHandle>)>;
  using BlobContextGetter =
      base::RepeatingCallback<base::WeakPtr<storage::BlobStorageContext>()>;

  // This method should be called on UI thread and calls back on UI thread
  // as well. Note that retrieving a blob ptr out of BlobHandle can only be
  // done on IO. |callback| returns a nullptr on failure.
  static void CreateMemoryBackedBlob(BrowserContext* browser_context,
                                     const char* data,
                                     size_t length,
                                     const std::string& content_type,
                                     BlobCallback callback);

  // Get a BlobStorageContext getter that needs to run on IO thread.
  static BlobContextGetter GetBlobStorageContext(
      BrowserContext* browser_context);

  // Delivers a push message with |data| to the Service Worker identified by
  // |origin| and |service_worker_registration_id|.
  static void DeliverPushMessage(
      BrowserContext* browser_context,
      const GURL& origin,
      int64_t service_worker_registration_id,
      const PushEventPayload& payload,
      const base::Callback<void(mojom::PushDeliveryStatus)>& callback);

  static void NotifyWillBeDestroyed(BrowserContext* browser_context);

  // Ensures that the corresponding ResourceContext is initialized. Normally the
  // BrowserContext initializs the corresponding getters when its objects are
  // created, but if the embedder wants to pass the ResourceContext to another
  // thread before they use BrowserContext, they should call this to make sure
  // that the ResourceContext is ready.
  static void EnsureResourceContextInitialized(BrowserContext* browser_context);

  // Tells the HTML5 objects on this context to persist their session state
  // across the next restart.
  static void SaveSessionState(BrowserContext* browser_context);

  static void SetDownloadManagerForTesting(
      BrowserContext* browser_context,
      std::unique_ptr<content::DownloadManager> download_manager);

  // Makes the Service Manager aware of this BrowserContext, and assigns a user
  // ID number to it. Should be called for each BrowserContext created.
  static void Initialize(BrowserContext* browser_context,
                         const base::FilePath& path);

  // Returns a Service User ID associated with this BrowserContext. This ID is
  // not persistent across runs. See
  // services/service_manager/public/mojom/connector.mojom. By default,
  // this user id is randomly generated when Initialize() is called.
  static const std::string& GetServiceUserIdFor(
      BrowserContext* browser_context);

  // Returns the BrowserContext associated with |user_id|, or nullptr if no
  // BrowserContext exists for that |user_id|.
  static BrowserContext* GetBrowserContextForServiceUserId(
      const std::string& user_id);

  // Returns a Connector associated with this BrowserContext, which can be used
  // to connect to service instances bound as this user.
  static service_manager::Connector* GetConnectorFor(
      BrowserContext* browser_context);
  static ServiceManagerConnection* GetServiceManagerConnectionFor(
      BrowserContext* browser_context);

  BrowserContext();

  ~BrowserContext() override;

  // Shuts down the storage partitions associated to this browser context.
  // This must be called before the browser context is actually destroyed
  // and before a clean-up task for its corresponding IO thread residents (e.g.
  // ResourceContext) is posted, so that the classes that hung on
  // StoragePartition can have time to do necessary cleanups on IO thread.
  void ShutdownStoragePartitions();

#if !defined(OS_ANDROID)
  // Creates a delegate to initialize a HostZoomMap and persist its information.
  // This is called during creation of each StoragePartition.
  virtual std::unique_ptr<ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) = 0;
#endif

  // Returns the path of the directory where this context's data is stored.
  virtual base::FilePath GetPath() const = 0;

  // Return whether this context is incognito. Default is false.
  virtual bool IsOffTheRecord() const = 0;

  // Returns the resource context.
  virtual ResourceContext* GetResourceContext() = 0;

  // Returns the DownloadManagerDelegate for this context. This will be called
  // once per context. The embedder owns the delegate and is responsible for
  // ensuring that it outlives DownloadManager. Note in particular that it is
  // unsafe to destroy the delegate in the destructor of a subclass of
  // BrowserContext, since it needs to be alive in ~BrowserContext.
  // It's valid to return nullptr.
  virtual DownloadManagerDelegate* GetDownloadManagerDelegate() = 0;

  // Returns the guest manager for this context.
  virtual BrowserPluginGuestManager* GetGuestManager() = 0;

  // Returns a special storage policy implementation, or nullptr.
  virtual storage::SpecialStoragePolicy* GetSpecialStoragePolicy() = 0;

  // Returns a push messaging service. The embedder owns the service, and is
  // responsible for ensuring that it outlives RenderProcessHost. It's valid to
  // return nullptr.
  virtual PushMessagingService* GetPushMessagingService() = 0;

  // Returns the SSL host state decisions for this context. The context may
  // return nullptr, implementing the default exception storage strategy.
  virtual SSLHostStateDelegate* GetSSLHostStateDelegate() = 0;

  // Returns the PermissionManager associated with that context if any, nullptr
  // otherwise.
  virtual PermissionManager* GetPermissionManager() = 0;

  // Returns the BackgroundFetchDelegate associated with that context if any,
  // nullptr otherwise.
  virtual BackgroundFetchDelegate* GetBackgroundFetchDelegate() = 0;

  // Returns the BackgroundSyncController associated with that context if any,
  // nullptr otherwise.
  virtual BackgroundSyncController* GetBackgroundSyncController() = 0;

  // Returns the BrowsingDataRemoverDelegate for this context. This will be
  // called once per context. It's valid to return nullptr.
  virtual BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate() = 0;

  // Creates the main net::URLRequestContextGetter. It's called only once.
  virtual net::URLRequestContextGetter* CreateRequestContext(
      ProtocolHandlerMap* protocol_handlers,
      URLRequestInterceptorScopedVector request_interceptors) = 0;

  // Creates the net::URLRequestContextGetter for a StoragePartition. It's
  // called only once per partition_path.
  virtual net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory,
      ProtocolHandlerMap* protocol_handlers,
      URLRequestInterceptorScopedVector request_interceptors) = 0;

  // Creates the main net::URLRequestContextGetter for media resources. It's
  // called only once.
  virtual net::URLRequestContextGetter* CreateMediaRequestContext() = 0;

  // Creates the media net::URLRequestContextGetter for a StoragePartition. It's
  // called only once per partition_path.
  virtual net::URLRequestContextGetter*
      CreateMediaRequestContextForStoragePartition(
          const base::FilePath& partition_path,
          bool in_memory) = 0;

  using StaticServiceMap =
      std::map<std::string, service_manager::EmbeddedServiceInfo>;

  // Registers per-browser-context services to be loaded in the browser process
  // by the Service Manager.
  virtual void RegisterInProcessServices(StaticServiceMap* services) {}

  // Returns a unique string associated with this browser context.
  virtual const std::string& UniqueId() const;

  // Returns a random salt string that is used for creating media device IDs.
  // Returns a random string by default.
  virtual std::string GetMediaDeviceIDSalt();

  // Utility function useful for embedders. Only needs to be called if
  // 1) The embedder needs to use a new salt, and
  // 2) The embedder saves its salt across restarts.
  static std::string CreateRandomMediaDeviceIDSalt();

  // Media service for storing/retrieving video decoding performance stats.
  // Exposed here rather than StoragePartition because all SiteInstances should
  // have similar decode performance and stats are not exposed to the web
  // directly, so privacy is not compromised.
  virtual media::VideoDecodePerfHistory* GetVideoDecodePerfHistory();

 private:
  const std::string unique_id_;
  bool was_notify_will_be_destroyed_called_ = false;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
