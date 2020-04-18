// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/store_thumbnail_task.h"

#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

bool StoreThumbnailSync(const OfflinePageThumbnail& thumbnail,
                        sql::Connection* db) {
  static const char kSql[] =
      "INSERT OR REPLACE INTO page_thumbnails (offline_id, expiration, "
      "thumbnail) VALUES (?, ?, ?)";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, thumbnail.offline_id);
  statement.BindInt64(1, store_utils::ToDatabaseTime(thumbnail.expiration));
  statement.BindString(2, thumbnail.thumbnail);
  return statement.Run();
}

}  // namespace

StoreThumbnailTask::StoreThumbnailTask(
    OfflinePageMetadataStoreSQL* store,
    OfflinePageThumbnail thumbnail,
    base::OnceCallback<void(bool)> complete_callback)
    : store_(store),
      thumbnail_(std::move(thumbnail)),
      complete_callback_(std::move(complete_callback)),
      weak_ptr_factory_(this) {}

StoreThumbnailTask::~StoreThumbnailTask() = default;

void StoreThumbnailTask::Run() {
  store_->Execute(base::BindOnce(StoreThumbnailSync, std::move(thumbnail_)),
                  base::BindOnce(&StoreThumbnailTask::Complete,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void StoreThumbnailTask::Complete(bool success) {
  TaskComplete();
  std::move(complete_callback_).Run(success);
}

}  // namespace offline_pages
