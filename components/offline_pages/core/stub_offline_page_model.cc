// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/stub_offline_page_model.h"

#include "base/files/file_path.h"

namespace offline_pages {

StubOfflinePageModel::StubOfflinePageModel()
    : archive_directory_(base::FilePath(FILE_PATH_LITERAL("/archive_dir/"))) {}
StubOfflinePageModel::~StubOfflinePageModel() {}

void StubOfflinePageModel::SetArchiveDirectory(const base::FilePath& path) {
  archive_directory_ = path;
}

void StubOfflinePageModel::AddObserver(Observer* observer) {}
void StubOfflinePageModel::RemoveObserver(Observer* observer) {}
void StubOfflinePageModel::SavePage(
    const SavePageParams& save_page_params,
    std::unique_ptr<OfflinePageArchiver> archiver,
    content::WebContents* web_contents,
    const SavePageCallback& callback) {}
void StubOfflinePageModel::AddPage(const OfflinePageItem& page,
                                   const AddPageCallback& callback) {}
void StubOfflinePageModel::MarkPageAccessed(int64_t offline_id) {}
void StubOfflinePageModel::DeletePagesByOfflineId(
    const std::vector<int64_t>& offline_ids,
    const DeletePageCallback& callback) {}
void StubOfflinePageModel::DeletePagesByClientIds(
    const std::vector<ClientId>& client_ids,
    const DeletePageCallback& callback) {}
void StubOfflinePageModel::DeletePagesByClientIdsAndOrigin(
    const std::vector<ClientId>& client_ids,
    const std::string& origin,
    const DeletePageCallback& callback) {}
void StubOfflinePageModel::GetPagesByClientIds(
    const std::vector<ClientId>& client_ids,
    MultipleOfflinePageItemCallback callback) {}
void StubOfflinePageModel::DeleteCachedPagesByURLPredicate(
    const UrlPredicate& predicate,
    const DeletePageCallback& callback) {}
void StubOfflinePageModel::GetAllPages(
    MultipleOfflinePageItemCallback callback) {}
void StubOfflinePageModel::GetOfflineIdsForClientId(
    const ClientId& client_id,
    const MultipleOfflineIdCallback& callback) {}
void StubOfflinePageModel::GetPageByOfflineId(
    int64_t offline_id,
    SingleOfflinePageItemCallback callback) {}
void StubOfflinePageModel::GetPageByGuid(
    const std::string& guid,
    SingleOfflinePageItemCallback callback) {}
void StubOfflinePageModel::GetPagesByURL(
    const GURL& url,
    URLSearchMode url_search_mode,
    MultipleOfflinePageItemCallback callback) {}
void StubOfflinePageModel::GetPagesByRequestOrigin(
    const std::string& origin,
    MultipleOfflinePageItemCallback callback) {}
void StubOfflinePageModel::GetPageBySizeAndDigest(
    int64_t file_size,
    const std::string& digest,
    SingleOfflinePageItemCallback callback) {}
void StubOfflinePageModel::GetPagesRemovedOnCacheReset(
    MultipleOfflinePageItemCallback callback) {}
void StubOfflinePageModel::GetPagesByNamespace(
    const std::string& name_space,
    MultipleOfflinePageItemCallback callback) {}
void StubOfflinePageModel::GetPagesSupportedByDownloads(
    MultipleOfflinePageItemCallback callback) {}
void StubOfflinePageModel::StoreThumbnail(const OfflinePageThumbnail& thumb) {}
void StubOfflinePageModel::GetThumbnailByOfflineId(
    int64_t offline_id,
    GetThumbnailCallback callback) {}
void StubOfflinePageModel::HasThumbnailForOfflineId(
    int64_t offline_id,
    base::OnceCallback<void(bool)> callback) {}
void StubOfflinePageModel::PublishInternalArchive(
    const OfflinePageItem& offline_page,
    std::unique_ptr<OfflinePageArchiver> archiver,
    PublishPageCallback publish_done_callback){};
const base::FilePath& StubOfflinePageModel::GetInternalArchiveDirectory(
    const std::string& name_space) const {
  return archive_directory_;
}
bool StubOfflinePageModel::IsArchiveInInternalDir(
    const base::FilePath& file_path) const {
  return archive_directory_.IsParent(file_path);
}

ClientPolicyController* StubOfflinePageModel::GetPolicyController() {
  return &policy_controller_;
}
OfflineEventLogger* StubOfflinePageModel::GetLogger() {
  return nullptr;
}
}  // namespace offline_pages
