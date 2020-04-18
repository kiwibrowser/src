// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/complete_offline_page_upgrade_task.h"

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

CompleteUpgradeStatus CompleteOfflinePageUpgradeSync(
    int64_t offline_id,
    const base::FilePath& temporary_file_path,
    const base::FilePath& target_file_path,
    const std::string& digest,
    int64_t file_size,
    sql::Connection* db) {
  if (!db)
    return CompleteUpgradeStatus::DB_ERROR;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return CompleteUpgradeStatus::DB_ERROR;

  // We need to remember the old file path, so that we can remove that file
  // later on.
  const char kSql[] =
      "SELECT file_path FROM offlinepages_v1 WHERE offline_id = ?";
  sql::Statement select_statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  select_statement.BindInt64(0, offline_id);
  if (!select_statement.Step()) {
    return select_statement.Succeeded() ? CompleteUpgradeStatus::ITEM_MISSING
                                        : CompleteUpgradeStatus::DB_ERROR;
  }

  base::FilePath old_file_path =
      store_utils::FromDatabaseFilePath(select_statement.ColumnString(0));

  // TODO(fgorski): Verify the digest and size of the temporary file.
  // That requires moving ComputeDigest function to component and this is
  // already in progress.

  // Verify that the temporary file is there.
  if (!base::PathExists(temporary_file_path))
    return CompleteUpgradeStatus::TEMPORARY_FILE_MISSING;

  // Verify that the target file name is not in use.
  if (base::PathExists(target_file_path))
    return CompleteUpgradeStatus::TARGET_FILE_NAME_IN_USE;

  if (!base::Move(temporary_file_path, target_file_path))
    return CompleteUpgradeStatus::RENAMING_FAILED;

  // Conditions for upgrade are met here.
  // Update remaining attempts in DB and complete task.
  const char kUpdateSql[] =
      "UPDATE offlinepages_v1"
      " SET upgrade_attempt = 0, file_path = ?, file_size = ?, digest = ?"
      " WHERE offline_id = ?";
  sql::Statement update_statement(
      db->GetCachedStatement(SQL_FROM_HERE, kUpdateSql));
  update_statement.BindString(
      0, store_utils::ToDatabaseFilePath(target_file_path));
  update_statement.BindInt64(1, file_size);
  update_statement.BindString(2, digest);
  update_statement.BindInt64(3, offline_id);

  // This status might require special handling/reporting as the new file was
  // renamed to its final name, but store item was not updated accordingly.
  if (!update_statement.Run() || !transaction.Commit())
    return CompleteUpgradeStatus::DB_ERROR_POST_FILE_RENAME;

  // Make a best effort to delete the old file (will be cleaned up by
  // consistency check otherwise).
  base::DeleteFile(old_file_path, false /* recursive */);

  return CompleteUpgradeStatus::SUCCESS;
}

}  // namespace

CompleteOfflinePageUpgradeTask::CompleteOfflinePageUpgradeTask(
    OfflinePageMetadataStoreSQL* store,
    int64_t offline_id,
    const base::FilePath& temporary_file_path,
    const base::FilePath& target_file_path,
    const std::string& digest,
    int64_t file_size,
    CompleteUpgradeCallback callback)
    : store_(store),
      offline_id_(offline_id),
      temporary_file_path_(temporary_file_path),
      target_file_path_(target_file_path),
      digest_(digest),
      file_size_(file_size),
      callback_(std::move(callback)),
      weak_ptr_factory_(this) {
  DCHECK(store_);
  DCHECK(!callback_.is_null());
}

CompleteOfflinePageUpgradeTask::~CompleteOfflinePageUpgradeTask() {}

void CompleteOfflinePageUpgradeTask::Run() {
  store_->Execute(
      base::BindOnce(&CompleteOfflinePageUpgradeSync, offline_id_,
                     temporary_file_path_, target_file_path_, digest_,
                     file_size_),
      base::BindOnce(&CompleteOfflinePageUpgradeTask::InformUpgradeAttemptDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CompleteOfflinePageUpgradeTask::InformUpgradeAttemptDone(
    CompleteUpgradeStatus result) {
  std::move(callback_).Run(result);
  TaskComplete();
}

}  // namespace offline_pages
