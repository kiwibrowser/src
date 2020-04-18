// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/get_thumbnail_task.h"

#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

std::unique_ptr<OfflinePageThumbnail> GetThumbnailSync(int64_t offline_id,
                                                       sql::Connection* db) {
  std::unique_ptr<OfflinePageThumbnail> result;
  static const char kSql[] =
      "SELECT offline_id, expiration, thumbnail FROM page_thumbnails"
      " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);

  if (!statement.Step()) {
    return result;
  }
  result = std::make_unique<OfflinePageThumbnail>();

  result->offline_id = statement.ColumnInt64(0);
  int64_t expiration = statement.ColumnInt64(1);
  result->expiration = store_utils::FromDatabaseTime(expiration);
  if (!statement.ColumnBlobAsString(2, &result->thumbnail))
    result.reset();

  return result;
}

}  // namespace

GetThumbnailTask::GetThumbnailTask(OfflinePageMetadataStoreSQL* store,
                                   int64_t offline_id,
                                   CompleteCallback complete_callback)
    : store_(store),
      offline_id_(offline_id),
      complete_callback_(std::move(complete_callback)),
      weak_ptr_factory_(this) {}

GetThumbnailTask::~GetThumbnailTask() = default;

void GetThumbnailTask::Run() {
  store_->Execute(base::BindOnce(GetThumbnailSync, std::move(offline_id_)),
                  base::BindOnce(&GetThumbnailTask::Complete,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void GetThumbnailTask::Complete(std::unique_ptr<OfflinePageThumbnail> result) {
  TaskComplete();
  std::move(complete_callback_).Run(std::move(result));
}

}  // namespace offline_pages
