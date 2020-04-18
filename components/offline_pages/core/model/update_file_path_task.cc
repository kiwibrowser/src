// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/update_file_path_task.h"

#include "base/bind.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/model/offline_page_model_utils.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

bool UpdateFilePathSync(const base::FilePath& new_file_path,
                        int64_t offline_id,
                        sql::Connection* db) {
  if (!db)
    return false;

  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  // Update the file_path to point to the new path.
  const char kSqlUpdate[] =
      "UPDATE OR IGNORE offlinepages_v1"
      " SET file_path = ?"
      " WHERE offline_id = ?";
  sql::Statement update_statement(
      db->GetCachedStatement(SQL_FROM_HERE, kSqlUpdate));
  update_statement.BindString(
      0, offline_pages::store_utils::ToDatabaseFilePath(new_file_path));
  update_statement.BindInt64(1, offline_id);

  if (!update_statement.Run())
    return false;

  if (!transaction.Commit())
    return false;

  return true;
}

}  // namespace

UpdateFilePathTask::UpdateFilePathTask(OfflinePageMetadataStoreSQL* store,
                                       int64_t offline_id,
                                       const base::FilePath& file_path,
                                       UpdateFilePathDoneCallback callback)
    : store_(store),
      offline_id_(offline_id),
      file_path_(file_path),
      callback_(std::move(callback)),
      weak_ptr_factory_(this) {
  DCHECK(store_);
}

UpdateFilePathTask::~UpdateFilePathTask(){};

void UpdateFilePathTask::Run() {
  store_->Execute(base::BindOnce(&UpdateFilePathSync, file_path_, offline_id_),
                  base::BindOnce(&UpdateFilePathTask::OnUpdateFilePathDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void UpdateFilePathTask::OnUpdateFilePathDone(bool result) {
  // Forward the updated offline page to the callback
  std::move(callback_).Run(result);
  TaskComplete();
}

}  // namespace offline_pages
