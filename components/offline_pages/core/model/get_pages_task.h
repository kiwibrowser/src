// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_GET_PAGES_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_GET_PAGES_TASK_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/client_id.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/task.h"

class GURL;

namespace offline_pages {
class ClientPolicyController;

// Gets offline pages that match the list of client IDs.
class GetPagesTask : public Task {
 public:
  // Structure defining and intermediate read result.
  struct ReadResult {
    ReadResult();
    ReadResult(const ReadResult& other);
    ~ReadResult();

    bool success;
    std::vector<OfflinePageItem> pages;
  };

  using DbWorkCallback = OfflinePageMetadataStoreSQL::RunCallback<ReadResult>;

  // Creates |GetPagesTask| reading all pages from DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingAllPages(
      OfflinePageMetadataStoreSQL* store,
      MultipleOfflinePageItemCallback callback);

  // Creates |GetPagesTask| reading pages matching provided |client_ids| from
  // DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingClientIds(
      OfflinePageMetadataStoreSQL* store,
      MultipleOfflinePageItemCallback callback,
      const std::vector<ClientId>& client_ids);

  // Creates |GetPagesTask| reading pages belonging to provided |name_space|
  // from DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingNamespace(
      OfflinePageMetadataStoreSQL* store,
      MultipleOfflinePageItemCallback callback,
      const std::string& name_space);

  // Creates |GetPagesTask| reading pages removed on cache reset from DB.
  static std::unique_ptr<GetPagesTask>
  CreateTaskMatchingPagesRemovedOnCacheReset(
      OfflinePageMetadataStoreSQL* store,
      MultipleOfflinePageItemCallback callback,
      ClientPolicyController* policy_controller);

  // Creates |GetPagesTask| reading pages in namespaces supported by downloads
  // from DB.
  static std::unique_ptr<GetPagesTask>
  CreateTaskMatchingPagesSupportedByDownloads(
      OfflinePageMetadataStoreSQL* store,
      MultipleOfflinePageItemCallback callback,
      ClientPolicyController* policy_controller);

  // Creates |GetPagesTask| reading pages matching provided |request_origin|
  // from DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingRequestOrigin(
      OfflinePageMetadataStoreSQL* store,
      MultipleOfflinePageItemCallback callback,
      const std::string& request_origin);

  // Creates |GetPagesTask| reading pages matching provided |url| from DB.
  // The url will be matched against original URL and final URL. Fragments will
  // be removed from all URLs prior to matching. Only a match on a single field
  // is necessary.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingUrl(
      OfflinePageMetadataStoreSQL* store,
      MultipleOfflinePageItemCallback callback,
      const GURL& url);

  // Creates |GetPagesTask| reading a single page matching provided |offline_id|
  // from DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingOfflineId(
      OfflinePageMetadataStoreSQL* store,
      SingleOfflinePageItemCallback callback,
      int64_t offline_id);

  // Creates |GetPagesTask| reading a single page matching provided |guid| from
  // DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingGuid(
      OfflinePageMetadataStoreSQL* store,
      SingleOfflinePageItemCallback callback,
      const std::string& guid);

  // Creates |GetPagesTask| reading a single page matching provided |file_size|
  // and |digest| from DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingSizeAndDigest(
      OfflinePageMetadataStoreSQL* store,
      SingleOfflinePageItemCallback callback,
      int64_t file_size,
      const std::string& digest);

  // Creates |GetPagesTask| selecting persistent items having a non-zero
  // remaining upgrade attempts.
  // Order of items is determined by number of remaining attempts (descending)
  // and creation time (descending).
  static std::unique_ptr<GetPagesTask> CreateTaskSelectingItemsMarkedForUpgrade(
      OfflinePageMetadataStoreSQL* store,
      MultipleOfflinePageItemCallback callback);

  ~GetPagesTask() override;

  // Task implementation:
  void Run() override;

 private:
  GetPagesTask(OfflinePageMetadataStoreSQL* store,
               DbWorkCallback db_work_callback,
               MultipleOfflinePageItemCallback callback);

  void ReadRequests();
  void CompleteWithResult(ReadResult result);

  OfflinePageMetadataStoreSQL* store_;
  DbWorkCallback db_work_callback_;
  MultipleOfflinePageItemCallback callback_;

  base::WeakPtrFactory<GetPagesTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(GetPagesTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_GET_PAGES_TASK_H_
