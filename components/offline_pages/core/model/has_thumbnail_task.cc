// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/has_thumbnail_task.h"

#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

namespace {

bool ThumbnailExistsSync(int64_t offline_id, sql::Connection* db) {
  static const char kSql[] =
      "SELECT 1 FROM page_thumbnails WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);
  return statement.Step();
}

}  // namespace

HasThumbnailTask::HasThumbnailTask(OfflinePageMetadataStoreSQL* store,
                                   int64_t offline_id,
                                   ThumbnailExistsCallback exists_callback)
    : store_(store),
      offline_id_(offline_id),
      exists_callback_(std::move(exists_callback)),
      weak_ptr_factory_(this) {}

HasThumbnailTask::~HasThumbnailTask() = default;

void HasThumbnailTask::Run() {
  store_->Execute(base::BindOnce(ThumbnailExistsSync, std::move(offline_id_)),
                  base::BindOnce(&HasThumbnailTask::OnThumbnailExists,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void HasThumbnailTask::OnThumbnailExists(bool exists) {
  TaskComplete();
  std::move(exists_callback_).Run(exists);
}

}  // namespace offline_pages
