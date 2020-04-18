// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/delete_page_task.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "components/offline_pages/core/model/offline_page_model_utils.h"
#include "components/offline_pages/core/offline_page_client_policy.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

using DeletePageTaskResult = DeletePageTask::DeletePageTaskResult;

namespace {

#define OFFLINE_PAGES_TABLE_NAME "offlinepages_v1"

// A wrapper of DeletedPageInfo to include |file_path| in order to be used
// through the deletion process. This is implementation detail and it will be
// used to create OfflinePageModel::DeletedPageInfo that are passed through
// callback.
// Please keep WRAPPER_FIELDS, WRAPPER_FIELD_COUNT, the struct declaration of
// DeletedPageInfoWrapper and the method CreateInfoWrapper in sync.
// The WRAPPER_FIELD_COUNT is used for queries which requires more info than the
// fields of INFO_WRAPPER_FIELD, as the additional field can be added manually
// in the SQL query and the result of it can be simply fetched by calling
// statement.Column*(INFO_WRAPPER_COUNT), as it's the last column. For example,
// please take a look at GetCachedDeletedPageInfoWrappersByUrlPredicateSync.
#define INFO_WRAPPER_FIELDS                                                  \
  "offline_id, system_download_id, client_namespace, client_id, file_path, " \
  "request_origin, access_count, creation_time, online_url"
#define INFO_WRAPPER_FIELD_COUNT 8

struct DeletedPageInfoWrapper {
  DeletedPageInfoWrapper();
  DeletedPageInfoWrapper(const DeletedPageInfoWrapper& other);
  int64_t offline_id;
  int64_t system_download_id;
  ClientId client_id;
  base::FilePath file_path;
  std::string request_origin;
  // Used by metric collection only:
  int access_count;
  base::Time creation_time;
  GURL url;
};

DeletedPageInfoWrapper CreateInfoWrapper(const sql::Statement& statement) {
  DeletedPageInfoWrapper info_wrapper;
  info_wrapper.offline_id = statement.ColumnInt64(0);
  info_wrapper.system_download_id = statement.ColumnInt64(1);
  info_wrapper.client_id.name_space = statement.ColumnString(2);
  info_wrapper.client_id.id = statement.ColumnString(3);
  info_wrapper.file_path =
      store_utils::FromDatabaseFilePath(statement.ColumnString(4));
  info_wrapper.request_origin = statement.ColumnString(5);
  info_wrapper.access_count = statement.ColumnInt(6);
  info_wrapper.creation_time =
      store_utils::FromDatabaseTime(statement.ColumnInt64(7));
  info_wrapper.url = GURL(statement.ColumnString(8));
  return info_wrapper;
}

DeletedPageInfoWrapper::DeletedPageInfoWrapper() = default;
DeletedPageInfoWrapper::DeletedPageInfoWrapper(
    const DeletedPageInfoWrapper& other) = default;

void ReportDeletePageHistograms(
    const std::vector<DeletedPageInfoWrapper>& info_wrappers) {
  const int max_minutes = base::TimeDelta::FromDays(365).InMinutes();
  base::Time delete_time = base::Time::Now();
  for (const auto& info_wrapper : info_wrappers) {
    base::UmaHistogramCustomCounts(
        model_utils::AddHistogramSuffix(info_wrapper.client_id.name_space,
                                        "OfflinePages.PageLifetime"),
        (delete_time - info_wrapper.creation_time).InMinutes(), 1, max_minutes,
        100);
    base::UmaHistogramCustomCounts(
        model_utils::AddHistogramSuffix(info_wrapper.client_id.name_space,
                                        "OfflinePages.AccessCount"),
        info_wrapper.access_count, 1, 1000000, 50);
  }
}

bool DeleteArchiveSync(const base::FilePath& file_path) {
  // Delete the file only, |false| for recursive.
  return base::DeleteFile(file_path, false);
}

// Deletes a page from the store by |offline_id|.
bool DeletePageEntryByOfflineIdSync(sql::Connection* db, int64_t offline_id) {
  static const char kSql[] =
      "DELETE FROM " OFFLINE_PAGES_TABLE_NAME " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);
  return statement.Run();
}

// Deletes pages by DeletedPageInfoWrapper. This will return a
// DeletePageTaskResult which contains the infos of the deleted pages (which are
// successfully deleted from the disk and the store) and a DeletePageResult.
// For each DeletedPageInfoWrapper to be deleted, the deletion will delete the
// archive file first, then database entry, in order to avoid the potential
// issue of leaving archive files behind (and they may be imported later).
// Since the database entry will only be deleted while the associated archive
// file is deleted successfully, there will be no such issue.
DeletePageTaskResult DeletePagesByDeletedPageInfoWrappersSync(
    sql::Connection* db,
    const std::vector<DeletedPageInfoWrapper>& info_wrappers) {
  std::vector<OfflinePageModel::DeletedPageInfo> deleted_page_infos;

  // If there's no page to delete, return an empty list with SUCCESS.
  if (info_wrappers.size() == 0)
    return DeletePageTaskResult(DeletePageResult::SUCCESS, deleted_page_infos);

  ReportDeletePageHistograms(info_wrappers);

  bool any_archive_deleted = false;
  for (const auto& info_wrapper : info_wrappers) {
    if (DeleteArchiveSync(info_wrapper.file_path)) {
      any_archive_deleted = true;
      if (DeletePageEntryByOfflineIdSync(db, info_wrapper.offline_id)) {
        deleted_page_infos.emplace_back(
            info_wrapper.offline_id, info_wrapper.system_download_id,
            info_wrapper.client_id, info_wrapper.request_origin,
            info_wrapper.url);
      }
    }
  }
  // If there're no files deleted, return DEVICE_FAILURE.
  if (!any_archive_deleted)
    return DeletePageTaskResult(DeletePageResult::DEVICE_FAILURE,
                                deleted_page_infos);

  return DeletePageTaskResult(DeletePageResult::SUCCESS, deleted_page_infos);
}

// Gets the page info for |offline_id|, returning in |info_wrapper|. Returns
// false if there's no record for |offline_id|.
bool GetDeletedPageInfoWrapperByOfflineIdSync(
    sql::Connection* db,
    int64_t offline_id,
    DeletedPageInfoWrapper* info_wrapper) {
  static const char kSql[] =
      "SELECT " INFO_WRAPPER_FIELDS " FROM " OFFLINE_PAGES_TABLE_NAME
      " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);

  if (statement.Step()) {
    *info_wrapper = CreateInfoWrapper(statement);
    return true;
  }
  return false;
}

DeletePageTaskResult DeletePagesByOfflineIdsSync(
    const std::vector<int64_t>& offline_ids,
    sql::Connection* db) {
  if (!db)
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});
  if (offline_ids.empty())
    return DeletePageTaskResult(DeletePageResult::SUCCESS, {});

  // If you create a transaction but dont Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});

  std::vector<DeletedPageInfoWrapper> infos;
  for (int64_t offline_id : offline_ids) {
    DeletedPageInfoWrapper info;
    if (GetDeletedPageInfoWrapperByOfflineIdSync(db, offline_id, &info))
      infos.push_back(info);
  }
  DeletePageTaskResult result =
      DeletePagesByDeletedPageInfoWrappersSync(db, infos);

  if (!transaction.Commit())
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});
  return result;
}

// Gets page infos for |client_id|, returning a vector of
// DeletedPageInfoWrappers because ClientId can refer to multiple pages.
std::vector<DeletedPageInfoWrapper> GetDeletedPageInfoWrappersByClientIdSync(
    sql::Connection* db,
    ClientId client_id) {
  std::vector<DeletedPageInfoWrapper> info_wrappers;
  static const char kSql[] =
      "SELECT " INFO_WRAPPER_FIELDS " FROM " OFFLINE_PAGES_TABLE_NAME
      " WHERE client_namespace = ? AND client_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, client_id.name_space);
  statement.BindString(1, client_id.id);

  while (statement.Step())
    info_wrappers.emplace_back(CreateInfoWrapper(statement));

  return info_wrappers;
}

DeletePageTaskResult DeletePagesByClientIdsSync(
    const std::vector<ClientId> client_ids,
    sql::Connection* db) {
  std::vector<DeletedPageInfoWrapper> infos;

  if (!db)
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});
  if (client_ids.empty())
    return DeletePageTaskResult(DeletePageResult::SUCCESS, {});

  // If you create a transaction but dont Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});

  for (ClientId client_id : client_ids) {
    std::vector<DeletedPageInfoWrapper> temp_infos =
        GetDeletedPageInfoWrappersByClientIdSync(db, client_id);
    infos.insert(infos.end(), temp_infos.begin(), temp_infos.end());
  }

  DeletePageTaskResult result =
      DeletePagesByDeletedPageInfoWrappersSync(db, infos);

  if (!transaction.Commit())
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});
  return result;
}

// Gets page infos for |client_id|, returning a vector of
// DeletedPageInfoWrappers because ClientId can refer to multiple pages.
std::vector<DeletedPageInfoWrapper>
GetDeletedPageInfoWrappersByClientIdAndOriginSync(sql::Connection* db,
                                                  ClientId client_id,
                                                  const std::string& origin) {
  std::vector<DeletedPageInfoWrapper> info_wrappers;
  static const char kSql[] =
      "SELECT " INFO_WRAPPER_FIELDS " FROM " OFFLINE_PAGES_TABLE_NAME
      " WHERE client_namespace = ? AND client_id = ? AND request_origin = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, client_id.name_space);
  statement.BindString(1, client_id.id);
  statement.BindString(2, origin);

  while (statement.Step())
    info_wrappers.emplace_back(CreateInfoWrapper(statement));

  return info_wrappers;
}

DeletePageTaskResult DeletePagesByClientIdsAndOriginSync(
    const std::vector<ClientId> client_ids,
    const std::string& origin,
    sql::Connection* db) {
  std::vector<DeletedPageInfoWrapper> infos;

  if (!db)
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});
  if (client_ids.empty())
    return DeletePageTaskResult(DeletePageResult::SUCCESS, {});

  // If you create a transaction but dont Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});

  for (ClientId client_id : client_ids) {
    std::vector<DeletedPageInfoWrapper> temp_infos =
        GetDeletedPageInfoWrappersByClientIdAndOriginSync(db, client_id,
                                                          origin);
    infos.insert(infos.end(), temp_infos.begin(), temp_infos.end());
  }

  DeletePageTaskResult result =
      DeletePagesByDeletedPageInfoWrappersSync(db, infos);

  if (!transaction.Commit())
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});
  return result;
}

// Gets the page information of pages that are within the provided temporary
// namespaces and satisfy the provided URL predicate.
std::vector<DeletedPageInfoWrapper>
GetCachedDeletedPageInfoWrappersByUrlPredicateSync(
    sql::Connection* db,
    const std::vector<std::string>& temp_namespaces,
    const UrlPredicate& url_predicate) {
  std::vector<DeletedPageInfoWrapper> info_wrappers;
  static const char kSql[] =
      "SELECT " INFO_WRAPPER_FIELDS
      ", online_url"
      " FROM " OFFLINE_PAGES_TABLE_NAME " WHERE client_namespace = ?";

  for (const auto& temp_namespace : temp_namespaces) {
    sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
    statement.BindString(0, temp_namespace);

    while (statement.Step()) {
      if (!url_predicate.Run(
              GURL(statement.ColumnString(INFO_WRAPPER_FIELD_COUNT))))
        continue;
      DeletedPageInfoWrapper info_wrapper = CreateInfoWrapper(statement);
      info_wrappers.push_back(info_wrapper);
    }
  }
  return info_wrappers;
}

DeletePageTaskResult DeleteCachedPagesByUrlPredicateSync(
    const std::vector<std::string>& namespaces,
    const UrlPredicate& predicate,
    sql::Connection* db) {
  if (!db)
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});

  // If you create a transaction but dont Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});

  const std::vector<DeletedPageInfoWrapper>& infos =
      GetCachedDeletedPageInfoWrappersByUrlPredicateSync(db, namespaces,
                                                         predicate);
  DeletePageTaskResult result =
      DeletePagesByDeletedPageInfoWrappersSync(db, infos);

  if (!transaction.Commit())
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});
  return result;
}

// Gets the page information of pages whose url and name_space equal to |url|
// and |name_space|. The pages will be deleted from old to new (by last access
// time) until there are limit - 1 pages left.
// TODO(romax): This might be affected by https://crbug.com/753609 for url
// matching.
std::vector<DeletedPageInfoWrapper>
GetDeletedPageInfoWrappersForPageLimitDeletion(sql::Connection* db,
                                               const GURL& url,
                                               std::string name_space,
                                               size_t limit) {
  std::vector<DeletedPageInfoWrapper> info_wrappers;
  static const char kSql[] =
      "SELECT " INFO_WRAPPER_FIELDS " FROM " OFFLINE_PAGES_TABLE_NAME
      " WHERE client_namespace = ? AND online_url = ?"
      " ORDER BY last_access_time ASC";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindString(0, name_space);
  statement.BindString(1, url.spec());

  while (statement.Step()) {
    DeletedPageInfoWrapper info_wrapper = CreateInfoWrapper(statement);
    info_wrappers.push_back(info_wrapper);
  }

  // Since the page information was selected by ascending order of last access
  // time, only the first |size - limit| pages needs to be deleted.
  int page_to_delete = info_wrappers.size() - limit;
  if (page_to_delete < 0)
    page_to_delete = 0;
  info_wrappers.resize(page_to_delete);
  return info_wrappers;
}

DeletePageTaskResult DeletePagesForPageLimit(const GURL& url,
                                             std::string name_space,
                                             size_t limit,
                                             sql::Connection* db) {
  if (!db)
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});

  // If the namespace can have unlimited pages per url, just return success.
  if (limit == kUnlimitedPages)
    return DeletePageTaskResult(DeletePageResult::SUCCESS, {});

  // If you create a transaction but dont Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});

  const std::vector<DeletedPageInfoWrapper>& infos =
      GetDeletedPageInfoWrappersForPageLimitDeletion(db, url, name_space,
                                                     limit);
  DeletePageTaskResult result =
      DeletePagesByDeletedPageInfoWrappersSync(db, infos);

  if (!transaction.Commit())
    return DeletePageTaskResult(DeletePageResult::STORE_FAILURE, {});
  return result;
}

}  // namespace

// DeletePageTaskResult implementations.
DeletePageTaskResult::DeletePageTaskResult() = default;
DeletePageTaskResult::DeletePageTaskResult(
    DeletePageResult result,
    const std::vector<OfflinePageModel::DeletedPageInfo>& infos)
    : result(result), infos(infos) {}
DeletePageTaskResult::DeletePageTaskResult(const DeletePageTaskResult& other) =
    default;
DeletePageTaskResult::~DeletePageTaskResult() = default;

// static
std::unique_ptr<DeletePageTask> DeletePageTask::CreateTaskMatchingOfflineIds(
    OfflinePageMetadataStoreSQL* store,
    DeletePageTask::DeletePageTaskCallback callback,
    const std::vector<int64_t>& offline_ids) {
  return std::unique_ptr<DeletePageTask>(new DeletePageTask(
      store, base::BindOnce(&DeletePagesByOfflineIdsSync, offline_ids),
      std::move(callback)));
}

// static
std::unique_ptr<DeletePageTask> DeletePageTask::CreateTaskMatchingClientIds(
    OfflinePageMetadataStoreSQL* store,
    DeletePageTask::DeletePageTaskCallback callback,
    const std::vector<ClientId>& client_ids) {
  return std::unique_ptr<DeletePageTask>(new DeletePageTask(
      store, base::BindOnce(&DeletePagesByClientIdsSync, client_ids),
      std::move(callback)));
}

// static
std::unique_ptr<DeletePageTask>
DeletePageTask::CreateTaskMatchingClientIdsAndOrigin(
    OfflinePageMetadataStoreSQL* store,
    DeletePageTask::DeletePageTaskCallback callback,
    const std::vector<ClientId>& client_ids,
    const std::string& origin) {
  return std::unique_ptr<DeletePageTask>(new DeletePageTask(
      store,
      base::BindOnce(&DeletePagesByClientIdsAndOriginSync, client_ids, origin),
      std::move(callback)));
}

// static
std::unique_ptr<DeletePageTask>
DeletePageTask::CreateTaskMatchingUrlPredicateForCachedPages(
    OfflinePageMetadataStoreSQL* store,
    DeletePageTask::DeletePageTaskCallback callback,
    ClientPolicyController* policy_controller,
    const UrlPredicate& predicate) {
  std::vector<std::string> temp_namespaces =
      policy_controller->GetNamespacesRemovedOnCacheReset();
  return std::unique_ptr<DeletePageTask>(
      new DeletePageTask(store,
                         base::BindOnce(&DeleteCachedPagesByUrlPredicateSync,
                                        temp_namespaces, predicate),
                         std::move(callback)));
}

// static
std::unique_ptr<DeletePageTask> DeletePageTask::CreateTaskDeletingForPageLimit(
    OfflinePageMetadataStoreSQL* store,
    DeletePageTask::DeletePageTaskCallback callback,
    ClientPolicyController* policy_controller,
    const OfflinePageItem& page) {
  std::string name_space = page.client_id.name_space;
  size_t limit = policy_controller->GetPolicy(name_space).pages_allowed_per_url;
  return std::unique_ptr<DeletePageTask>(new DeletePageTask(
      store,
      base::BindOnce(&DeletePagesForPageLimit, page.url, name_space, limit),
      std::move(callback)));
}

DeletePageTask::DeletePageTask(OfflinePageMetadataStoreSQL* store,
                               DeleteFunction func,
                               DeletePageTaskCallback callback)
    : store_(store),
      func_(std::move(func)),
      callback_(std::move(callback)),
      weak_ptr_factory_(this) {
  DCHECK(store_);
  DCHECK(!callback_.is_null());
}

DeletePageTask::~DeletePageTask() {}

void DeletePageTask::Run() {
  store_->Execute(std::move(func_),
                  base::BindOnce(&DeletePageTask::OnDeletePageDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void DeletePageTask::OnDeletePageDone(DeletePageTaskResult result) {
  std::move(callback_).Run(result.result, result.infos);
  TaskComplete();
}

}  // namespace offline_pages
