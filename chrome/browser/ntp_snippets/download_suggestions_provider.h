// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NTP_SNIPPETS_DOWNLOAD_SUGGESTIONS_PROVIDER_H_
#define CHROME_BROWSER_NTP_SNIPPETS_DOWNLOAD_SUGGESTIONS_PROVIDER_H_

#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/download/download_history.h"
#include "components/ntp_snippets/callbacks.h"
#include "components/ntp_snippets/category.h"
#include "components/ntp_snippets/category_status.h"
#include "components/ntp_snippets/content_suggestion.h"
#include "components/ntp_snippets/content_suggestions_provider.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "content/public/browser/download_manager.h"

class PrefRegistrySimple;
class PrefService;

namespace offline_pages {
struct OfflinePageItem;
}

namespace base {
class Clock;
}

// Provides download content suggestions from the offline pages model and the
// download manager (obtaining the data through DownloadManager and each
// DownloadItem). Offline page related downloads are referred to as offline page
// downloads, while the remaining downloads (e.g. images, music, books) are
// called asset downloads. In case either of the data sources is |nullptr|, it
// is ignored.
class DownloadSuggestionsProvider
    : public ntp_snippets::ContentSuggestionsProvider,
      public offline_pages::OfflinePageModel::Observer,
      public content::DownloadManager::Observer,
      public download::DownloadItem::Observer,
      public DownloadHistory::Observer {
 public:
  DownloadSuggestionsProvider(
      ContentSuggestionsProvider::Observer* observer,
      offline_pages::OfflinePageModel* offline_page_model,
      content::DownloadManager* download_manager,
      DownloadHistory* download_history,
      PrefService* pref_service,
      base::Clock* clock);
  ~DownloadSuggestionsProvider() override;

  // ContentSuggestionsProvider implementation.
  ntp_snippets::CategoryStatus GetCategoryStatus(
      ntp_snippets::Category category) override;
  ntp_snippets::CategoryInfo GetCategoryInfo(
      ntp_snippets::Category category) override;
  void DismissSuggestion(
      const ntp_snippets::ContentSuggestion::ID& suggestion_id) override;
  void FetchSuggestionImage(
      const ntp_snippets::ContentSuggestion::ID& suggestion_id,
      ntp_snippets::ImageFetchedCallback callback) override;
  void FetchSuggestionImageData(
      const ntp_snippets::ContentSuggestion::ID& suggestion_id,
      ntp_snippets::ImageDataFetchedCallback callback) override;
  void Fetch(const ntp_snippets::Category& category,
             const std::set<std::string>& known_suggestion_ids,
             ntp_snippets::FetchDoneCallback callback) override;
  void ClearHistory(
      base::Time begin,
      base::Time end,
      const base::Callback<bool(const GURL& url)>& filter) override;
  void ClearCachedSuggestions() override;
  void GetDismissedSuggestionsForDebugging(
      ntp_snippets::Category category,
      ntp_snippets::DismissedSuggestionsCallback callback) override;
  void ClearDismissedSuggestionsForDebugging(
      ntp_snippets::Category category) override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class DownloadSuggestionsProviderTest;

  void GetPagesMatchingQueryCallbackForGetDismissedSuggestions(
      ntp_snippets::DismissedSuggestionsCallback callback,
      const std::vector<offline_pages::OfflinePageItem>& offline_pages) const;

  // OfflinePageModel::Observer implementation.
  void OfflinePageModelLoaded(offline_pages::OfflinePageModel* model) override;
  void OfflinePageAdded(
      offline_pages::OfflinePageModel* model,
      const offline_pages::OfflinePageItem& added_page) override;
  void OfflinePageDeleted(
      const offline_pages::OfflinePageModel::DeletedPageInfo& page_info)
      override;

  // content::DownloadManager::Observer implementation.
  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;
  void ManagerGoingDown(content::DownloadManager* manager) override;

  // download::DownloadItem::Observer implementation.
  void OnDownloadUpdated(download::DownloadItem* item) override;
  void OnDownloadOpened(download::DownloadItem* item) override;
  void OnDownloadRemoved(download::DownloadItem* item) override;
  void OnDownloadDestroyed(download::DownloadItem* item) override;

  // DownloadHistory::Observer implementation.
  void OnHistoryQueryComplete() override;
  void OnDownloadHistoryDestroyed() override;

  // Updates the |category_status_| of the |provided_category_| and notifies the
  // |observer_|, if necessary.
  void NotifyStatusChanged(ntp_snippets::CategoryStatus new_status);

  // Requests all offline pages and after asynchronously obtaining the result,
  // prunes dismissed IDs and caches some most recent items. If |notify| is
  // true, notifies |ContentSuggestionsProvider::Observer| about them.
  void AsynchronouslyFetchOfflinePagesDownloads(bool notify);

  // Retrieves all asset downloads, prunes dismissed IDs and caches some most
  // recent items, but does not notify |ContentSuggestionsProvider::Observer|
  // about them.
  void FetchAssetsDownloads();

  // Retrieves both offline page and asset downloads, updates the internal cache
  // and notifies |ContentSuggestionsProvider::Observer|.
  void AsynchronouslyFetchAllDownloadsAndSubmitSuggestions();

  // Takes |kMaxSuggestionsCount| the most recent cached suggestions and
  // notifies |ContentSuggestionsProvider::Observer| about them.
  void SubmitContentSuggestions();

  // Converts an OfflinePageItem to a ContentSuggestion for the
  // |provided_category_|.
  ntp_snippets::ContentSuggestion ConvertOfflinePage(
      const offline_pages::OfflinePageItem& offline_page) const;

  // Converts DownloadItem to a ContentSuggestion for the |provided_category_|.
  ntp_snippets::ContentSuggestion ConvertDownloadItem(
      const download::DownloadItem& download_item) const;

  // Returns true if a download published and last visited times are considered
  // too old for the download to be shown.
  bool IsDownloadOutdated(const base::Time& published_time,
                          const base::Time& last_visited_time);

  // Adds |item| to the internal asset download cache if all of the following
  // holds:
  // - the download is completed;
  // - its suggestion has not been dismissed;
  // - there are less than |kMaxSuggestionsCount| items cached or the oldest
  //     cached item is older than the current item (the oldest item is removed
  //     then);
  // - the item is not present in the cache yet.
  // Returns |true| if the item has been added.
  bool CacheAssetDownloadIfNeeded(const download::DownloadItem* item);

  // Removes item corresponding to |suggestion_id| either from offline pages or
  // asset download cache (depends on the |suggestion_id|). Returns |false| if
  // there is no corresponding item in the cache and so nothing has been
  // removed.
  bool RemoveSuggestionFromCacheIfPresent(
      const ntp_snippets::ContentSuggestion::ID& suggestion_id);

  // Removes item corresponding to |suggestion_id| from cache and fetches all
  // corresponding downloads to update the cache if there may be any not in
  // cache.
  void RemoveSuggestionFromCacheAndRetrieveMoreIfNeeded(
      const ntp_snippets::ContentSuggestion::ID& suggestion_id);

  // Processes a list of offline pages (assuming that these are all the download
  // offline pages that currently exist), prunes dismissed IDs and updates
  // internal cache. If |notify| is true, notifies
  // |ContentSuggestionsProvider::Observer|.
  void UpdateOfflinePagesCache(
      bool notify,
      const std::vector<offline_pages::OfflinePageItem>&
          all_download_offline_pages);

  // Fires the |OnSuggestionInvalidated| event for the suggestion corresponding
  // to the given |id_within_category| and clears it from the dismissed IDs
  // list, if necessary.
  void InvalidateSuggestion(const std::string& id_within_category);

  // Reads dismissed IDs related to asset downloads from prefs.
  std::set<std::string> ReadAssetDismissedIDsFromPrefs() const;

  // Writes |dismissed_ids| into prefs for asset downloads.
  void StoreAssetDismissedIDsToPrefs(
      const std::set<std::string>& dismissed_ids);

  // Reads dismissed IDs related to offline page downloads from prefs.
  std::set<std::string> ReadOfflinePageDismissedIDsFromPrefs() const;

  // Writes |dismissed_ids| into prefs for offline page downloads.
  void StoreOfflinePageDismissedIDsToPrefs(
      const std::set<std::string>& dismissed_ids);

  // Reads from prefs dismissed IDs related to either offline page or asset
  // downloads (given by |for_offline_page_downloads|).
  std::set<std::string> ReadDismissedIDsFromPrefs(
      bool for_offline_page_downloads) const;

  // Writes |dismissed_ids| into prefs for either offline page or asset
  // downloads (given by |for_offline_page_downloads|).
  void StoreDismissedIDsToPrefs(bool for_offline_page_downloads,
                                const std::set<std::string>& dismissed_ids);

  void UnregisterDownloadItemObservers();

  ntp_snippets::CategoryStatus category_status_;
  const ntp_snippets::Category provided_category_;
  offline_pages::OfflinePageModel* offline_page_model_;
  content::DownloadManager* download_manager_;
  DownloadHistory* download_history_;
  PrefService* pref_service_;
  base::Clock* clock_;

  // Cached offline page downloads. If there are not enough asset downloads, all
  // of these could be shown (they are the most recently visited, not dismissed
  // and not invalidated). Order is undefined. If the model has less than
  // |kMaxSuggestionsCount| offline pages, then all of them which satisfy the
  // criteria above are cached, otherwise only |kMaxSuggestionsCount|.
  std::vector<offline_pages::OfflinePageItem> cached_offline_page_downloads_;
  // Cached asset downloads. If there are not enough offline page downloads, all
  // of these could be shown (they are the most recently downloaded, not
  // dismissed and not invalidated). Order is undefined. If the model has less
  // than |kMaxSuggestionsCount| asset downloads, then all of them which satisfy
  // the criteria above are cached, otherwise only |kMaxSuggestionsCount|.
  std::vector<const download::DownloadItem*> cached_asset_downloads_;

  bool is_asset_downloads_initialization_complete_;

  base::WeakPtrFactory<DownloadSuggestionsProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadSuggestionsProvider);
};

#endif  // CHROME_BROWSER_NTP_SNIPPETS_DOWNLOAD_SUGGESTIONS_PROVIDER_H_
