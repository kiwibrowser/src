// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/clear_digest_task.h"

#include "base/bind.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "sql/connection.h"
#include "sql/statement.h"

namespace offline_pages {

namespace {

bool ClearDigestSync(int64_t offline_id, sql::Connection* db) {
  if (!db)
    return false;

  const char kSql[] =
      "UPDATE OR IGNORE offlinepages_v1"
      " SET digest = '' "
      " WHERE offline_id = ?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, offline_id);
  return statement.Run();
}

}  // namespace

ClearDigestTask::ClearDigestTask(OfflinePageMetadataStoreSQL* store,
                                 int64_t offline_id)
    : store_(store), offline_id_(offline_id), weak_ptr_factory_(this) {
  DCHECK(store_);
}

ClearDigestTask::~ClearDigestTask(){};

void ClearDigestTask::Run() {
  store_->Execute(base::BindOnce(&ClearDigestSync, offline_id_),
                  base::BindOnce(&ClearDigestTask::OnClearDigestDone,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void ClearDigestTask::OnClearDigestDone(bool result) {
  // The result is currently omitted, since there's no callback needed for this
  // task.
  // TODO(romax): Add a UMA collection here, recording the result of db
  // operation.
  TaskComplete();
}

}  // namespace offline_pages
