// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_DOWNLOAD_UI_ADAPTER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_DOWNLOADS_DOWNLOAD_UI_ADAPTER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/supports_user_data.h"
#include "components/offline_items_collection/core/offline_content_aggregator.h"
#include "components/offline_items_collection/core/offline_content_provider.h"
#include "components/offline_items_collection/core/offline_item.h"
#include "components/offline_pages/core/background/request_coordinator.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "url/gurl.h"

using ContentId = offline_items_collection::ContentId;
using OfflineItem = offline_items_collection::OfflineItem;
using OfflineContentProvider = offline_items_collection::OfflineContentProvider;
using OfflineContentAggregator =
    offline_items_collection::OfflineContentAggregator;

namespace offline_pages {
class ThumbnailDecoder;

// C++ side of the UI Adapter. Mimics DownloadManager/Item/History (since we
// share UI with Downloads).
// An instance of this class is owned by OfflinePageModel and is shared between
// UI components if needed. It manages the cache of OfflineItems, which are fed
// to the OfflineContentAggregator which subsequently takes care of notifying
// observers of items being loaded, added, deleted etc. The creator of the
// adapter also passes in the Delegate that determines which items in the
// underlying OfflinePage backend are to be included (visible) in the
// collection.
class DownloadUIAdapter : public OfflineContentProvider,
                          public OfflinePageModel::Observer,
                          public RequestCoordinator::Observer,
                          public base::SupportsUserData::Data {
 public:
  // Delegate, used to customize behavior of this Adapter.
  class Delegate {
   public:
    virtual ~Delegate() = default;
    // Returns true if the page or request with the specified Client Id should
    // be visible in the collection of items exposed by this Adapter. This also
    // indicates if Observers will be notified about changes for the given page.
    virtual bool IsVisibleInUI(const ClientId& client_id) = 0;
    // Sometimes the item should be in the collection but not visible in the UI,
    // temporarily. This is a relatively special case, for example for Last_N
    // snapshots that are only valid while their tab is alive. When the status
    // of temporary visibility changes, the Delegate is supposed to call
    // DownloadUIAdapter::TemporarilyHiddenStatusChanged().
    virtual bool IsTemporarilyHiddenInUI(const ClientId& client_id) = 0;

    // Delegates need a reference to the UI adapter in order to notify it about
    // visibility changes.
    virtual void SetUIAdapter(DownloadUIAdapter* ui_adapter) = 0;

    // Opens an offline item.
    virtual void OpenItem(const OfflineItem& item, int64_t offline_id) = 0;
  };

  // Create the adapter. thumbnail_decoder may be null, in which case,
  // thumbnails will not be provided through GetVisualsForItem.
  DownloadUIAdapter(OfflineContentAggregator* aggregator,
                    OfflinePageModel* model,
                    RequestCoordinator* coordinator,
                    std::unique_ptr<ThumbnailDecoder> thumbnail_decoder,
                    std::unique_ptr<Delegate> delegate);
  ~DownloadUIAdapter() override;

  static DownloadUIAdapter* FromOfflinePageModel(OfflinePageModel* model);
  static void AttachToOfflinePageModel(
      std::unique_ptr<DownloadUIAdapter> adapter,
      OfflinePageModel* model);

  int64_t GetOfflineIdByGuid(const std::string& guid) const;

  // OfflineContentProvider implementation.
  void OpenItem(const ContentId& id) override;
  void RemoveItem(const ContentId& id) override;
  void CancelDownload(const ContentId& id) override;
  void PauseDownload(const ContentId& id) override;
  void ResumeDownload(const ContentId& id, bool has_user_gesture) override;
  void GetItemById(
      const ContentId& id,
      OfflineContentProvider::SingleItemCallback callback) override;
  void GetAllItems(
      OfflineContentProvider::MultipleItemCallback callback) override;
  void GetVisualsForItem(const ContentId& id,
                         const VisualsCallback& callback) override;
  void AddObserver(OfflineContentProvider::Observer* observer) override;
  void RemoveObserver(OfflineContentProvider::Observer* observer) override;

  // OfflinePageModel::Observer
  void OfflinePageModelLoaded(OfflinePageModel* model) override;
  void OfflinePageAdded(OfflinePageModel* model,
                        const OfflinePageItem& added_page) override;
  void OfflinePageDeleted(
      const OfflinePageModel::DeletedPageInfo& page_info) override;
  void ThumbnailAdded(OfflinePageModel* model,
                      const OfflinePageThumbnail& thumbnail) override;

  // RequestCoordinator::Observer
  void OnAdded(const SavePageRequest& request) override;
  void OnCompleted(const SavePageRequest& request,
                   RequestNotifier::BackgroundSavePageResult status) override;
  void OnChanged(const SavePageRequest& request) override;
  void OnNetworkProgress(const SavePageRequest& request,
                         int64_t received_bytes) override;

  // For the DownloadUIAdapter::Delegate, to report the temporary hidden status
  // change.
  void TemporaryHiddenStatusChanged(const ClientId& client_id);

  Delegate* delegate() { return delegate_.get(); }

  // Test method, to verify the internal cache is not loaded too early.
  bool IsCacheLoadedForTest() { return (state_ != State::NOT_LOADED); }

 private:
  enum class State { NOT_LOADED, LOADING_PAGES, LOADING_REQUESTS, LOADED };

  struct ItemInfo {
    ItemInfo(const OfflinePageItem& page,
             bool temporarily_hidden,
             bool is_suggested);
    ItemInfo(const SavePageRequest& request, bool temporarily_hidden);
    ~ItemInfo();

    std::unique_ptr<OfflineItem> ui_item;

    // Additional cached data, not exposed to UI through OfflineItem.
    // Indicates if this item wraps the completed page or in-progress request.
    bool is_request;

    // These are shared between pages and requests.
    int64_t offline_id;

    // ClientId is here to support the Delegate that can toggle temporary
    // visibility of the items in the collection.
    ClientId client_id;

    // This item is present in the collection but temporarily hidden from UI.
    // This is useful when unrelated reasons cause the UI item to be excluded
    // (filtered out) from UI. When item becomes temporarily hidden the adapter
    // issues ItemDeleted notification to observers, and ItemAdded when it
    // becomes visible again.
    bool temporarily_hidden;

   private:
    DISALLOW_COPY_AND_ASSIGN(ItemInfo);
  };

  typedef std::map<std::string, std::unique_ptr<ItemInfo>> OfflineItems;
  using VisualResultCallback = base::OnceCallback<void(
      std::unique_ptr<offline_items_collection::OfflineItemVisuals>)>;

  void LoadCache();
  void ClearCache();

  // Task callbacks.
  void CancelDownloadContinuation(
      const std::string& guid,
      std::vector<std::unique_ptr<SavePageRequest>> requests);
  void PauseDownloadContinuation(
      const std::string& guid,
      std::vector<std::unique_ptr<SavePageRequest>> requests);
  void ResumeDownloadContinuation(
      const std::string& guid,
      std::vector<std::unique_ptr<SavePageRequest>> requests);
  void OnOfflinePagesLoaded(const MultipleOfflinePageItemResult& pages);
  void OnThumbnailLoaded(VisualResultCallback callback,
                         std::unique_ptr<OfflinePageThumbnail> thumbnail);
  void OnRequestsLoaded(std::vector<std::unique_ptr<SavePageRequest>> requests);

  void OnDeletePagesDone(DeletePageResult result);

  void AddItemHelper(std::unique_ptr<ItemInfo> item_info);
  // This function is not re-entrant.  It temporarily sets |deleting_item_|
  // while it runs, so that functions such as |GetOfflineIdByGuid| will work
  // during the |ItemDeleted| callback.
  void DeleteItemHelper(const std::string& guid);

  void ReplyWithAllItems(OfflineContentProvider::MultipleItemCallback callback);

  void OpenItemByGuid(const std::string& guid);
  void RemoveItemByGuid(const std::string& guid);

  // A valid offline content aggregator, supplied at construction.
  OfflineContentAggregator* aggregator_;

  // Always valid, this class is a member of the model.
  OfflinePageModel* model_;

  // Always valid, a service.
  RequestCoordinator* request_coordinator_;

  // May be null if thumbnails are not required.
  std::unique_ptr<ThumbnailDecoder> thumbnail_decoder_;

  // A delegate, supplied at construction.
  std::unique_ptr<Delegate> delegate_;

  State state_;

  // The cache of UI items. The key is OfflineItem.guid.
  OfflineItems items_;

  // The callbacks for GetAllItems waiting for cache initialization.
  std::vector<OfflineContentProvider::MultipleItemCallback>
      postponed_callbacks_;

  // The requests for operations with items waiting for cache initialization.
  // std::vector<base::OnceCallback<const std::string&>> postponed_operations_;
  std::vector<base::OnceClosure> postponed_operations_;

  std::unique_ptr<ItemInfo> deleting_item_;

  // The observers.
  base::ObserverList<OfflineContentProvider::Observer> observers_;

  base::WeakPtrFactory<DownloadUIAdapter> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadUIAdapter);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGE_DOWNLOADS_DOWNLOAD_UI_ADAPTER_H_
