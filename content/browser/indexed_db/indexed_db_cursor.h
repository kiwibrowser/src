// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CURSOR_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CURSOR_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/indexed_db/indexed_db_backing_store.h"
#include "content/browser/indexed_db/indexed_db_database.h"
#include "content/browser/indexed_db/indexed_db_transaction.h"
#include "content/common/indexed_db/indexed_db_key_range.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"

namespace content {

class CONTENT_EXPORT IndexedDBCursor {
 public:
  IndexedDBCursor(std::unique_ptr<IndexedDBBackingStore::Cursor> cursor,
                  indexed_db::CursorType cursor_type,
                  blink::WebIDBTaskType task_type,
                  IndexedDBTransaction* transaction);
  ~IndexedDBCursor();

  void Advance(uint32_t count, scoped_refptr<IndexedDBCallbacks> callbacks);
  void Continue(std::unique_ptr<IndexedDBKey> key,
                std::unique_ptr<IndexedDBKey> primary_key,
                scoped_refptr<IndexedDBCallbacks> callbacks);
  void PrefetchContinue(int number_to_fetch,
                        scoped_refptr<IndexedDBCallbacks> callbacks);
  leveldb::Status PrefetchReset(int used_prefetches, int unused_prefetches);

  const IndexedDBKey& key() const { return cursor_->key(); }
  const IndexedDBKey& primary_key() const { return cursor_->primary_key(); }
  IndexedDBValue* Value() const {
    return (cursor_type_ == indexed_db::CURSOR_KEY_ONLY) ? NULL
                                                         : cursor_->value();
  }

  void Close();

  leveldb::Status CursorIterationOperation(
      std::unique_ptr<IndexedDBKey> key,
      std::unique_ptr<IndexedDBKey> primary_key,
      scoped_refptr<IndexedDBCallbacks> callbacks,
      IndexedDBTransaction* transaction);
  leveldb::Status CursorAdvanceOperation(
      uint32_t count,
      scoped_refptr<IndexedDBCallbacks> callbacks,
      IndexedDBTransaction* transaction);
  leveldb::Status CursorPrefetchIterationOperation(
      int number_to_fetch,
      scoped_refptr<IndexedDBCallbacks> callbacks,
      IndexedDBTransaction* transaction);

 private:
  blink::WebIDBTaskType task_type_;
  indexed_db::CursorType cursor_type_;

  // We rely on the transaction calling Close() to clear this.
  IndexedDBTransaction* transaction_;

  // Must be destroyed before transaction_.
  std::unique_ptr<IndexedDBBackingStore::Cursor> cursor_;
  // Must be destroyed before transaction_.
  std::unique_ptr<IndexedDBBackingStore::Cursor> saved_cursor_;

  bool closed_;

  base::WeakPtrFactory<IndexedDBCursor> ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(IndexedDBCursor);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CURSOR_H_
