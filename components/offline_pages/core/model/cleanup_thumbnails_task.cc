// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/cleanup_thumbnails_task.h"

#include "base/metrics/histogram_macros.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {
typedef base::OnceCallback<void(CleanupThumbnailsTask::Result)> ResultCallback;

CleanupThumbnailsTask::Result CleanupThumbnailsSync(base::Time now,
                                                    sql::Connection* db) {
  const char kSql[] =
      "DELETE FROM page_thumbnails "
      "WHERE offline_id IN ("
      "  SELECT pt.offline_id from page_thumbnails pt"
      "  LEFT OUTER JOIN offlinepages_v1 op"
      "  ON pt.offline_id = op.offline_id "
      "  WHERE op.offline_id IS NULL "
      "  AND pt.expiration < ?"
      ")";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, store_utils::ToDatabaseTime(now));
  if (!statement.Run())
    return CleanupThumbnailsTask::Result();

  return CleanupThumbnailsTask::Result{true, db->GetLastChangeCount()};
}

}  // namespace

CleanupThumbnailsTask::CleanupThumbnailsTask(
    OfflinePageMetadataStoreSQL* store,
    base::Time now,
    CleanupThumbnailsCallback complete_callback)
    : store_(store),
      now_(now),
      complete_callback_(std::move(complete_callback)),
      weak_ptr_factory_(this) {}

CleanupThumbnailsTask::~CleanupThumbnailsTask() = default;

void CleanupThumbnailsTask::Run() {
  store_->Execute(base::BindOnce(CleanupThumbnailsSync, now_),
                  base::BindOnce(&CleanupThumbnailsTask::Complete,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void CleanupThumbnailsTask::Complete(Result result) {
  TaskComplete();
  UMA_HISTOGRAM_COUNTS_1000("OfflinePages.CleanupThumbnails.Count",
                            result.removed_thumbnails);
  std::move(complete_callback_).Run(result.success);
}

}  // namespace offline_pages
