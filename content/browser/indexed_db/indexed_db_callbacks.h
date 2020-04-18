// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CALLBACKS_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CALLBACKS_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/strings/string16.h"
#include "content/browser/indexed_db/indexed_db_database_error.h"
#include "content/browser/indexed_db/indexed_db_dispatcher_host.h"
#include "content/common/indexed_db/indexed_db.mojom.h"
#include "content/common/indexed_db/indexed_db_key.h"
#include "content/common/indexed_db/indexed_db_key_path.h"
#include "content/public/browser/browser_thread.h"
#include "url/origin.h"

namespace base {
class SequencedTaskRunner;
}

namespace content {
class IndexedDBConnection;
class IndexedDBCursor;
class IndexedDBDatabase;
struct IndexedDBDataLossInfo;
struct IndexedDBDatabaseMetadata;
struct IndexedDBReturnValue;
struct IndexedDBValue;

// Expected to be constructed on IO thread and called/deleted from IDB sequence.
class CONTENT_EXPORT IndexedDBCallbacks
    : public base::RefCounted<IndexedDBCallbacks> {
 public:
  // Destructively converts an IndexedDBValue to a Mojo Value.
  static ::indexed_db::mojom::ValuePtr ConvertAndEraseValue(
      IndexedDBValue* value);

  IndexedDBCallbacks(
      base::WeakPtr<IndexedDBDispatcherHost> dispatcher_host,
      const url::Origin& origin,
      ::indexed_db::mojom::CallbacksAssociatedPtrInfo callbacks_info,
      scoped_refptr<base::SequencedTaskRunner> idb_runner);

  virtual void OnError(const IndexedDBDatabaseError& error);

  // IndexedDBFactory::GetDatabaseNames
  virtual void OnSuccess(const std::vector<base::string16>& string);

  // IndexedDBFactory::Open / DeleteDatabase
  virtual void OnBlocked(int64_t existing_version);

  // IndexedDBFactory::Open
  virtual void OnUpgradeNeeded(
      int64_t old_version,
      std::unique_ptr<IndexedDBConnection> connection,
      const content::IndexedDBDatabaseMetadata& metadata,
      const IndexedDBDataLossInfo& data_loss_info);
  virtual void OnSuccess(std::unique_ptr<IndexedDBConnection> connection,
                         const content::IndexedDBDatabaseMetadata& metadata);

  // IndexedDBDatabase::OpenCursor
  virtual void OnSuccess(std::unique_ptr<IndexedDBCursor> cursor,
                         const IndexedDBKey& key,
                         const IndexedDBKey& primary_key,
                         IndexedDBValue* value);

  // IndexedDBCursor::Continue / Advance
  virtual void OnSuccess(const IndexedDBKey& key,
                         const IndexedDBKey& primary_key,
                         IndexedDBValue* value);

  // IndexedDBCursor::PrefetchContinue
  virtual void OnSuccessWithPrefetch(
      const std::vector<IndexedDBKey>& keys,
      const std::vector<IndexedDBKey>& primary_keys,
      std::vector<IndexedDBValue>* values);

  // IndexedDBDatabase::Get
  // IndexedDBCursor::Advance
  virtual void OnSuccess(IndexedDBReturnValue* value);

  // IndexedDBDatabase::GetAll
  virtual void OnSuccessArray(std::vector<IndexedDBReturnValue>* values);

  // IndexedDBDatabase::Put / IndexedDBCursor::Update
  virtual void OnSuccess(const IndexedDBKey& key);

  // IndexedDBDatabase::Count
  // IndexedDBFactory::DeleteDatabase
  // IndexedDBDatabase::DeleteRange
  virtual void OnSuccess(int64_t value);

  // IndexedDBCursor::Continue / Advance (when complete)
  virtual void OnSuccess();

  void SetConnectionOpenStartTime(const base::TimeTicks& start_time);

 protected:
  virtual ~IndexedDBCallbacks();

 private:
  friend class base::RefCounted<IndexedDBCallbacks>;

  class IOThreadHelper;

  // Stores if this callbacks object is complete and should not be called again.
  bool complete_ = false;

  // Depending on whether the database needs upgrading, we create connections in
  // different spots. This stores if we've already created the connection so
  // OnSuccess(Connection) doesn't create an extra one.
  bool connection_created_ = false;

  // Used to assert that OnSuccess is only called if there was no data loss.
  blink::WebIDBDataLoss data_loss_;

  // The "blocked" event should be sent at most once per request.
  bool sent_blocked_ = false;
  base::TimeTicks connection_open_start_time_;

  std::unique_ptr<IOThreadHelper, BrowserThread::DeleteOnIOThread> io_helper_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(IndexedDBCallbacks);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CALLBACKS_H_
