// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/start_offline_page_upgrade_task.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/sys_info.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

StartUpgradeResult StartOfflinePageUpgradeSync(
    int64_t offline_id,
    const base::FilePath& target_directory,
    sql::Connection* db) {
  if (!db)
    return StartUpgradeResult(StartUpgradeStatus::DB_ERROR);

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return StartUpgradeResult(StartUpgradeStatus::DB_ERROR);

  const char kSql[] =
      "SELECT file_path, file_size, digest"
      " FROM offlinepages_v1 WHERE offline_id = ?";
  sql::Statement select_statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  select_statement.BindInt64(0, offline_id);
  if (!select_statement.Step()) {
    return StartUpgradeResult(select_statement.Succeeded()
                                  ? StartUpgradeStatus::ITEM_MISSING
                                  : StartUpgradeStatus::DB_ERROR);
  }

  base::FilePath file_path =
      store_utils::FromDatabaseFilePath(select_statement.ColumnString(0));
  if (!base::PathExists(file_path))
    return StartUpgradeResult(StartUpgradeStatus::FILE_MISSING);

  int64_t free_disk_space_on_target =
      base::SysInfo::AmountOfFreeDiskSpace(target_directory);

  int64_t file_size = select_statement.ColumnInt64(1);
  if (free_disk_space_on_target < 2 * file_size)
    return StartUpgradeResult(StartUpgradeStatus::NOT_ENOUGH_STORAGE);

  // Digest will be consumed when returning.
  std::string digest = select_statement.ColumnString(2);

  // Conditions for upgrade are met here.
  // Update remaining attempts in DB and complete task.
  const char kUpdateSql[] =
      "UPDATE offlinepages_v1 SET upgrade_attempt = upgrade_attempt - 1 "
      " WHERE offline_id = ?";
  sql::Statement update_statement(
      db->GetCachedStatement(SQL_FROM_HERE, kUpdateSql));
  update_statement.BindInt64(0, offline_id);

  if (!update_statement.Run() || !transaction.Commit())
    return StartUpgradeResult(StartUpgradeStatus::DB_ERROR);

  return StartUpgradeResult(StartUpgradeStatus::SUCCESS, std::move(digest),
                            std::move(file_path));
}

}  // namespace

StartOfflinePageUpgradeTask::StartOfflinePageUpgradeTask(
    OfflinePageMetadataStoreSQL* store,
    int64_t offline_id,
    const base::FilePath& target_directory,
    StartUpgradeCallback callback)
    : store_(store),
      offline_id_(offline_id),
      target_directory_(target_directory),
      callback_(std::move(callback)),
      weak_ptr_factory_(this) {
  DCHECK(store_);
  DCHECK(!callback_.is_null());
}

StartOfflinePageUpgradeTask::~StartOfflinePageUpgradeTask() {}

void StartOfflinePageUpgradeTask::Run() {
  store_->Execute(
      base::BindOnce(&StartOfflinePageUpgradeSync, offline_id_,
                     target_directory_),
      base::BindOnce(&StartOfflinePageUpgradeTask::InformUpgradeAttemptDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void StartOfflinePageUpgradeTask::InformUpgradeAttemptDone(
    StartUpgradeResult result) {
  std::move(callback_).Run(std::move(result));
  TaskComplete();
}

}  // namespace offline_pages
