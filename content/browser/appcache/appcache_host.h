// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_HOST_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_HOST_H_

#include <stdint.h>

#include <memory>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "content/browser/appcache/appcache_group.h"
#include "content/browser/appcache/appcache_service_impl.h"
#include "content/browser/appcache/appcache_storage.h"
#include "content/common/appcache_interfaces.h"
#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace net {
class URLRequest;
}  // namespace net

namespace content {
FORWARD_DECLARE_TEST(AppCacheGroupTest, CleanupUnusedGroup);
FORWARD_DECLARE_TEST(AppCacheGroupTest, QueueUpdate);
FORWARD_DECLARE_TEST(AppCacheHostTest, Basic);
FORWARD_DECLARE_TEST(AppCacheHostTest, SelectNoCache);
FORWARD_DECLARE_TEST(AppCacheHostTest, ForeignEntry);
FORWARD_DECLARE_TEST(AppCacheHostTest, FailedCacheLoad);
FORWARD_DECLARE_TEST(AppCacheHostTest, FailedGroupLoad);
FORWARD_DECLARE_TEST(AppCacheHostTest, SetSwappableCache);
FORWARD_DECLARE_TEST(AppCacheHostTest, ForDedicatedWorker);
FORWARD_DECLARE_TEST(AppCacheHostTest, SelectCacheAllowed);
FORWARD_DECLARE_TEST(AppCacheHostTest, SelectCacheBlocked);
FORWARD_DECLARE_TEST(AppCacheHostTest, SelectCacheTwice);
FORWARD_DECLARE_TEST(AppCacheTest, CleanupUnusedCache);
class AppCache;
class AppCacheFrontend;
class AppCacheGroupTest;
class AppCacheHostTest;
class AppCacheRequest;
class AppCacheRequestHandler;
class AppCacheRequestHandlerTest;
class AppCacheStorageImplTest;
class AppCacheSubresourceURLFactory;
class AppCacheTest;

namespace appcache_update_job_unittest {
class AppCacheUpdateJobTest;
}

using GetStatusCallback = base::OnceCallback<void(AppCacheStatus)>;
using StartUpdateCallback = base::OnceCallback<void(bool)>;
using SwapCacheCallback = base::OnceCallback<void(bool)>;

// Server-side representation of an application cache host.
class CONTENT_EXPORT AppCacheHost
    : public AppCacheStorage::Delegate,
      public AppCacheGroup::UpdateObserver,
      public AppCacheServiceImpl::Observer {
 public:

  class CONTENT_EXPORT Observer {
   public:
    // Called just after the cache selection algorithm completes.
    virtual void OnCacheSelectionComplete(AppCacheHost* host) = 0;

    // Called just prior to the instance being deleted.
    virtual void OnDestructionImminent(AppCacheHost* host) = 0;

    virtual ~Observer() {}
  };

  AppCacheHost(int host_id, AppCacheFrontend* frontend,
               AppCacheServiceImpl* service);
  ~AppCacheHost() override;

  // Adds/removes an observer, the AppCacheHost does not take
  // ownership of the observer.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Support for cache selection and scriptable method calls.
  bool SelectCache(const GURL& document_url,
                   const int64_t cache_document_was_loaded_from,
                   const GURL& manifest_url);
  bool SelectCacheForSharedWorker(int64_t appcache_id);
  bool MarkAsForeignEntry(const GURL& document_url,
                          int64_t cache_document_was_loaded_from);
  void GetStatusWithCallback(GetStatusCallback callback);
  void StartUpdateWithCallback(StartUpdateCallback callback);
  void SwapCacheWithCallback(SwapCacheCallback callback);

  // Called prior to the main resource load. When the system contains multiple
  // candidates for a main resource load, the appcache preferred by the host
  // that created this host is used to break ties.
  void SetSpawningHostId(int spawning_process_id, int spawning_host_id);

  // May return NULL if the spawning host context has been closed, or if a
  // spawning host context was never identified.
  const AppCacheHost* GetSpawningHost() const;

  const GURL& preferred_manifest_url() const {
    return preferred_manifest_url_;
  }
  void set_preferred_manifest_url(const GURL& url) {
    preferred_manifest_url_ = url;
  }

  // Support for loading resources out of the appcache.
  // May return NULL if the request isn't subject to retrieval from an appache.
  std::unique_ptr<AppCacheRequestHandler> CreateRequestHandler(
      std::unique_ptr<AppCacheRequest> request,
      ResourceType resource_type,
      bool should_reset_appcache);

  // Support for devtools inspecting appcache resources.
  void GetResourceList(std::vector<AppCacheResourceInfo>* resource_infos);

  // Breaks any existing association between this host and a cache.
  // 'manifest_url' is sent to DevTools as the manifest url that could have
  // been associated before or could be associated later with this host.
  // Associations are broken either thru the cache selection algorithm
  // implemented in this class, or by the update algorithm (see
  // AppCacheUpdateJob).
  void AssociateNoCache(const GURL& manifest_url);

  // Establishes an association between this host and an incomplete cache.
  // 'manifest_url' is manifest url of the cache group being updated.
  // Associations with incomplete caches are established by the update algorithm
  // (see AppCacheUpdateJob).
  void AssociateIncompleteCache(AppCache* cache, const GURL& manifest_url);

  // Establishes an association between this host and a complete cache.
  // Associations with complete caches are established either thru the cache
  // selection algorithm implemented (in this class), or by the update algorithm
  // (see AppCacheUpdateJob).
  void AssociateCompleteCache(AppCache* cache);

  // Adds a reference to the newest complete cache in a group, unless it's the
  // same as the cache that is currently associated with the host.
  void SetSwappableCache(AppCacheGroup* group);

  // Used to ensure that a loaded appcache survives a frame navigation.
  void LoadMainResourceCache(int64_t cache_id);

  // Used to notify the host that a namespace resource is being delivered as
  // the main resource of the page and to provide its url.
  void NotifyMainResourceIsNamespaceEntry(const GURL& namespace_entry_url);

  // Used to notify the host that the main resource was blocked by a policy. To
  // work properly, this method needs to by invoked prior to cache selection.
  void NotifyMainResourceBlocked(const GURL& manifest_url);

  // Used by the update job to keep track of which hosts are associated
  // with which pending master entries.
  const GURL& pending_master_entry_url() const {
    return new_master_entry_url_;
  }

  int host_id() const { return host_id_; }

  AppCacheServiceImpl* service() const { return service_; }
  AppCacheStorage* storage() const { return storage_; }
  AppCacheFrontend* frontend() const { return frontend_; }

  // PlzNavigate:
  // The AppCacheHost instance is created with a dummy AppCacheFrontend
  // pointer when the navigation starts. We need to switch it to the
  // actual frontend when the navigation commits.
  void set_frontend(AppCacheFrontend* frontend) { frontend_ = frontend; }

  AppCache* associated_cache() const { return associated_cache_.get(); }

  void enable_cache_selection(bool enable) {
    is_cache_selection_enabled_ = enable;
  }

  bool is_selection_pending() const {
    return pending_selected_cache_id_ != kAppCacheNoCacheId ||
           !pending_selected_manifest_url_.is_empty();
  }

  const GURL& first_party_url() const { return first_party_url_; }

  // Methods to support cross site navigations.
  void PrepareForTransfer();
  void CompleteTransfer(int host_id, AppCacheFrontend* frontend);

  // Returns a weak pointer reference to the host.
  base::WeakPtr<AppCacheHost> GetWeakPtr();

  // In the network service world, we need to pass the URLLoaderFactory
  // instance to the renderer which it can use to request subresources.
  // This ensures that they can be served out of the AppCache.
  void MaybePassSubresourceFactory();

  // This is called when the frame is navigated to a page which loads from
  // the AppCache.
  void SetAppCacheSubresourceFactory(
      AppCacheSubresourceURLFactory* subresource_factory);

 private:
  friend class content::AppCacheHostTest;
  friend class content::AppCacheStorageImplTest;
  friend class content::AppCacheRequestHandlerTest;
  friend class content::appcache_update_job_unittest::AppCacheUpdateJobTest;

  AppCacheStatus GetStatus();
  void LoadSelectedCache(int64_t cache_id);
  void LoadOrCreateGroup(const GURL& manifest_url);

  // See public Associate*Host() methods above.
  void AssociateCacheHelper(AppCache* cache, const GURL& manifest_url);

  // AppCacheStorage::Delegate impl
  void OnCacheLoaded(AppCache* cache, int64_t cache_id) override;
  void OnGroupLoaded(AppCacheGroup* group, const GURL& manifest_url) override;
  // AppCacheServiceImpl::Observer impl
  void OnServiceReinitialized(
      AppCacheStorageReference* old_storage_ref) override;

  void FinishCacheSelection(AppCache* cache, AppCacheGroup* group);
  void DoPendingGetStatus();
  void DoPendingStartUpdate();
  void DoPendingSwapCache();

  void ObserveGroupBeingUpdated(AppCacheGroup* group);

  // AppCacheGroup::UpdateObserver methods.
  void OnUpdateComplete(AppCacheGroup* group) override;

  // Returns true if this host is for a dedicated worker context.
  bool is_for_dedicated_worker() const {
    return parent_host_id_ != kAppCacheNoHostId;
  }

  // Returns the parent context's host instance. This is only valid
  // to call when this instance is_for_dedicated_worker.
  AppCacheHost* GetParentAppCacheHost() const;

  // Identifies the corresponding appcache host in the child process.
  int host_id_;

  // Information about the host that created this one; the manifest
  // preferred by our creator influences which cache our main resource
  // should be loaded from.
  int spawning_host_id_;
  int spawning_process_id_;
  GURL preferred_manifest_url_;

  // Hosts for dedicated workers are special cased to shunt
  // request handling off to the dedicated worker's parent.
  // The scriptable api is not accessible in dedicated workers
  // so the other aspects of this class are not relevant for
  // these special case instances.
  int parent_host_id_;
  int parent_process_id_;

  // Defined prior to refs to AppCaches and Groups because destruction
  // order matters, the disabled_storage_reference_ must outlive those
  // objects. See additional comments for the storage_ member.
  scoped_refptr<AppCacheStorageReference> disabled_storage_reference_;

  // The cache associated with this host, if any.
  scoped_refptr<AppCache> associated_cache_;

  // Hold a reference to the newest complete cache (if associated cache is
  // not the newest) to keep the newest cache in existence while the app cache
  // group is in use. The newest complete cache may have no associated hosts
  // holding any references to it and would otherwise be deleted prematurely.
  scoped_refptr<AppCache> swappable_cache_;

  // Keep a reference to the group being updated until the update completes.
  scoped_refptr<AppCacheGroup> group_being_updated_;

  // Similarly, keep a reference to the newest cache of the group until the
  // update completes. When adding a new master entry to a cache that is not
  // in use in any other host, this reference keeps the cache in  memory.
  scoped_refptr<AppCache> newest_cache_of_group_being_updated_;

  // Keep a reference to the cache of the main resource so it survives frame
  // navigations.
  scoped_refptr<AppCache> main_resource_cache_;
  int64_t pending_main_resource_cache_id_;

  // Cache loading is async, if we're loading a specific cache or group
  // for the purposes of cache selection, one or the other of these will
  // indicate which cache or group is being loaded.
  int64_t pending_selected_cache_id_;
  GURL pending_selected_manifest_url_;

  // Used to defend against bad IPC messages.
  bool was_select_cache_called_;

  // Used to avoid stepping on pages controlled by ServiceWorkers.
  bool is_cache_selection_enabled_;

  // A new master entry to be added to the cache, may be empty.
  GURL new_master_entry_url_;

  // The frontend proxy to deliver notifications to the child process.
  AppCacheFrontend* frontend_;

  // Our central service object.
  AppCacheServiceImpl* service_;

  // And the equally central storage object, with a twist. In some error
  // conditions the storage object gets recreated and reinitialized. The
  // disabled_storage_reference_ (defined earlier) allows for cleanup of an
  // instance that got disabled  after we had latched onto it. In normal
  // circumstances, disabled_storage_reference_ is expected to be NULL.
  // When non-NULL both storage_ and disabled_storage_reference_ refer to the
  // same instance.
  AppCacheStorage* storage_;

  // Since these are synchronous scriptable API calls in the client, there can
  // only be one type of callback pending. Also, we have to wait until we have a
  // cache selection prior to responding to these calls, as cache selection
  // involves async loading of a cache or a group from storage.
  GetStatusCallback pending_get_status_callback_;
  StartUpdateCallback pending_start_update_callback_;
  SwapCacheCallback pending_swap_cache_callback_;

  // True if an intercept or fallback namespace resource was
  // delivered as the main resource.
  bool main_resource_was_namespace_entry_;
  GURL namespace_entry_url_;

  // True if requests for this host were blocked by a policy.
  bool main_resource_blocked_;
  GURL blocked_manifest_url_;

  // Tells if info about associated cache is pending. Info is pending
  // when update job has not returned success yet.
  bool associated_cache_info_pending_;

  // List of objects observing us.
  base::ObserverList<Observer> observers_;

  // Used to inform the QuotaManager of what origins are currently in use.
  url::Origin origin_in_use_;

  // First party url to be used in policy checks.
  GURL first_party_url_;

  FRIEND_TEST_ALL_PREFIXES(content::AppCacheGroupTest, CleanupUnusedGroup);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheGroupTest, QueueUpdate);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheHostTest, Basic);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheHostTest, SelectNoCache);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheHostTest, ForeignEntry);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheHostTest, FailedCacheLoad);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheHostTest, FailedGroupLoad);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheHostTest, SetSwappableCache);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheHostTest, ForDedicatedWorker);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheHostTest, SelectCacheAllowed);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheHostTest, SelectCacheBlocked);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheHostTest, SelectCacheTwice);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheTest, CleanupUnusedCache);

  // In the network service world points to the subresource URLLoaderFactory.
  base::WeakPtr<AppCacheSubresourceURLFactory> subresource_url_factory_;

  base::WeakPtrFactory<AppCacheHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppCacheHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_HOST_H_
