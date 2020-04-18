// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_QUEUE_STORE_SQL_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_QUEUE_STORE_SQL_H_

#include <stdint.h>
#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/background/request_queue_store.h"

namespace base {
class SequencedTaskRunner;
}

namespace sql {
class Connection;
}

namespace offline_pages {

// SQLite implementation of RequestQueueStore.
//
// This store has a history of schema updates.
// Original schema was delivered in M57. Since then the following changes
// happened:
// * In M58 original_url was added.
// * In M61 request_origin was added.
// * In M67 fail_state was added.
//
// TODO(romax): remove all activation_time related code the next we change the
// schema.
//
// Looking for procedure to update the schema, please refer to
// offline_page_metadata_store_sql.h
class RequestQueueStoreSQL : public RequestQueueStore {
 public:
  RequestQueueStoreSQL(
      scoped_refptr<base::SequencedTaskRunner> background_task_runner,
      const base::FilePath& database_dir);
  ~RequestQueueStoreSQL() override;

  // RequestQueueStore implementation.
  void Initialize(InitializeCallback callback) override;
  void GetRequests(GetRequestsCallback callback) override;
  // Note: current implementation of this method makes a SQL query per ID. This
  // is OK as long as number of IDs stays low, which is a typical case.
  // Implementation should be revisited in case that presumption changes.
  void GetRequestsByIds(const std::vector<int64_t>& request_ids,
                        UpdateCallback callback) override;
  void AddRequest(const SavePageRequest& offline_page,
                  AddCallback callback) override;
  void UpdateRequests(const std::vector<SavePageRequest>& requests,
                      UpdateCallback callback) override;
  void RemoveRequests(const std::vector<int64_t>& request_ids,
                      UpdateCallback callback) override;
  void Reset(ResetCallback callback) override;
  StoreState state() const override;

 private:
  // Used to finalize DB connection initialization.
  void OnOpenConnectionDone(InitializeCallback callback, bool success);

  // Used to finalize DB connection reset.
  void OnResetDone(ResetCallback callback, bool success);

  // Helper function to return immediately if no database is found.
  bool CheckDb() const;

  // Background thread where all SQL access should be run.
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  // Path to the database on disk.
  base::FilePath db_file_path_;

  // Database connection.
  std::unique_ptr<sql::Connection> db_;

  // State of the store.
  StoreState state_;

  base::WeakPtrFactory<RequestQueueStoreSQL> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(RequestQueueStoreSQL);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_REQUEST_QUEUE_STORE_SQL_H_
