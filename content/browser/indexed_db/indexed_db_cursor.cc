// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_cursor.h"

#include <stddef.h>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "content/browser/indexed_db/indexed_db_callbacks.h"
#include "content/browser/indexed_db/indexed_db_database_error.h"
#include "content/browser/indexed_db/indexed_db_tracing.h"
#include "content/browser/indexed_db/indexed_db_transaction.h"
#include "content/browser/indexed_db/indexed_db_value.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_database_exception.h"

namespace content {
namespace {
// This should never be script visible: the cursor should either be closed when
// it hits the end of the range (and script throws an error before the call
// could be made), if the transaction has finished (ditto), or if there's an
// incoming request from the front end but the transaction has aborted on the
// back end; in that case the tx will already have sent an abort to the request
// so this would be ignored.
IndexedDBDatabaseError CreateCursorClosedError() {
  return IndexedDBDatabaseError(blink::kWebIDBDatabaseExceptionUnknownError,
                                "The cursor has been closed.");
}

leveldb::Status InvokeOrSucceed(base::WeakPtr<IndexedDBCursor> weak_cursor,
                                IndexedDBTransaction::Operation operation,
                                IndexedDBTransaction* transaction) {
  if (weak_cursor)
    return std::move(operation).Run(transaction);
  return leveldb::Status::OK();
}

// This allows us to bind a function with a return value to a weak ptr, and if
// the weak pointer is invalidated then we just return a default (success).
template <typename Functor, typename... Args>
IndexedDBTransaction::Operation BindWeakOperation(
    Functor&& functor,
    base::WeakPtr<IndexedDBCursor> weak_cursor,
    Args&&... args) {
  DCHECK(weak_cursor);
  IndexedDBCursor* cursor_ptr = weak_cursor.get();
  return base::BindOnce(&InvokeOrSucceed, std::move(weak_cursor),
                        base::BindOnce(std::forward<Functor>(functor),
                                       base::Unretained(cursor_ptr),
                                       std::forward<Args>(args)...));
}

}  // namespace

IndexedDBCursor::IndexedDBCursor(
    std::unique_ptr<IndexedDBBackingStore::Cursor> cursor,
    indexed_db::CursorType cursor_type,
    blink::WebIDBTaskType task_type,
    IndexedDBTransaction* transaction)
    : task_type_(task_type),
      cursor_type_(cursor_type),
      transaction_(transaction),
      cursor_(std::move(cursor)),
      closed_(false),
      ptr_factory_(this) {
  IDB_ASYNC_TRACE_BEGIN("IndexedDBCursor::open", this);
}

IndexedDBCursor::~IndexedDBCursor() {
  if (transaction_)
    transaction_->UnregisterOpenCursor(this);
  // Call to make sure we complete our lifetime trace.
  Close();
}

void IndexedDBCursor::Continue(std::unique_ptr<IndexedDBKey> key,
                               std::unique_ptr<IndexedDBKey> primary_key,
                               scoped_refptr<IndexedDBCallbacks> callbacks) {
  IDB_TRACE("IndexedDBCursor::Continue");

  if (closed_) {
    callbacks->OnError(CreateCursorClosedError());
    return;
  }

  transaction_->ScheduleTask(
      task_type_,
      BindWeakOperation(&IndexedDBCursor::CursorIterationOperation,
                        ptr_factory_.GetWeakPtr(), base::Passed(&key),
                        base::Passed(&primary_key), callbacks));
}

void IndexedDBCursor::Advance(uint32_t count,
                              scoped_refptr<IndexedDBCallbacks> callbacks) {
  IDB_TRACE("IndexedDBCursor::Advance");

  if (closed_) {
    callbacks->OnError(CreateCursorClosedError());
    return;
  }

  transaction_->ScheduleTask(
      task_type_,
      BindWeakOperation(&IndexedDBCursor::CursorAdvanceOperation,
                        ptr_factory_.GetWeakPtr(), count, callbacks));
}

leveldb::Status IndexedDBCursor::CursorAdvanceOperation(
    uint32_t count,
    scoped_refptr<IndexedDBCallbacks> callbacks,
    IndexedDBTransaction* /*transaction*/) {
  IDB_TRACE("IndexedDBCursor::CursorAdvanceOperation");
  leveldb::Status s = leveldb::Status::OK();

  if (!cursor_ || !cursor_->Advance(count, &s)) {
    cursor_.reset();
    if (s.ok()) {
      callbacks->OnSuccess(nullptr);
      return s;
    }
    Close();
    callbacks->OnError(IndexedDBDatabaseError(
        blink::kWebIDBDatabaseExceptionUnknownError, "Error advancing cursor"));
    return s;
  }

  callbacks->OnSuccess(key(), primary_key(), Value());
  return s;
}

leveldb::Status IndexedDBCursor::CursorIterationOperation(
    std::unique_ptr<IndexedDBKey> key,
    std::unique_ptr<IndexedDBKey> primary_key,
    scoped_refptr<IndexedDBCallbacks> callbacks,
    IndexedDBTransaction* /*transaction*/) {
  IDB_TRACE("IndexedDBCursor::CursorIterationOperation");
  leveldb::Status s = leveldb::Status::OK();

  if (!cursor_ ||
      !cursor_->Continue(key.get(), primary_key.get(),
                         IndexedDBBackingStore::Cursor::SEEK, &s)) {
    cursor_.reset();
    if (s.ok()) {
      // This happens if we reach the end of the iterator and can't continue.
      callbacks->OnSuccess(nullptr);
      return s;
    }
    Close();
    callbacks->OnError(
        IndexedDBDatabaseError(blink::kWebIDBDatabaseExceptionUnknownError,
                               "Error continuing cursor."));
    return s;
  }

  callbacks->OnSuccess(this->key(), this->primary_key(), Value());
  return s;
}

void IndexedDBCursor::PrefetchContinue(
    int number_to_fetch,
    scoped_refptr<IndexedDBCallbacks> callbacks) {
  IDB_TRACE("IndexedDBCursor::PrefetchContinue");

  if (closed_) {
    callbacks->OnError(CreateCursorClosedError());
    return;
  }

  transaction_->ScheduleTask(
      task_type_,
      BindWeakOperation(&IndexedDBCursor::CursorPrefetchIterationOperation,
                        ptr_factory_.GetWeakPtr(), number_to_fetch, callbacks));
}

leveldb::Status IndexedDBCursor::CursorPrefetchIterationOperation(
    int number_to_fetch,
    scoped_refptr<IndexedDBCallbacks> callbacks,
    IndexedDBTransaction* /*transaction*/) {
  IDB_TRACE("IndexedDBCursor::CursorPrefetchIterationOperation");
  leveldb::Status s = leveldb::Status::OK();

  std::vector<IndexedDBKey> found_keys;
  std::vector<IndexedDBKey> found_primary_keys;
  std::vector<IndexedDBValue> found_values;

  saved_cursor_.reset();
  // TODO(cmumford): Use IPC::Channel::kMaximumMessageSize
  const size_t max_size_estimate = 10 * 1024 * 1024;
  size_t size_estimate = 0;

  // TODO(cmumford): Handle this error (crbug.com/363397). Although this will
  //                 properly fail, caller will not know why, and any corruption
  //                 will be ignored.
  for (int i = 0; i < number_to_fetch; ++i) {
    if (!cursor_ || !cursor_->Continue(&s)) {
      cursor_.reset();
      if (s.ok()) {
        // We've reached the end, so just return what we have.
        break;
      }
      Close();
      callbacks->OnError(
          IndexedDBDatabaseError(blink::kWebIDBDatabaseExceptionUnknownError,
                                 "Error continuing cursor."));
      return s;
    }

    if (i == 0) {
      // First prefetched result is always used, so that's the position
      // a cursor should be reset to if the prefetch is invalidated.
      saved_cursor_ = cursor_->Clone();
    }

    found_keys.push_back(cursor_->key());
    found_primary_keys.push_back(cursor_->primary_key());

    switch (cursor_type_) {
      case indexed_db::CURSOR_KEY_ONLY:
        found_values.push_back(IndexedDBValue());
        break;
      case indexed_db::CURSOR_KEY_AND_VALUE: {
        IndexedDBValue value;
        value.swap(*cursor_->value());
        size_estimate += value.SizeEstimate();
        found_values.push_back(value);
        break;
      }
      default:
        NOTREACHED();
    }
    size_estimate += cursor_->key().size_estimate();
    size_estimate += cursor_->primary_key().size_estimate();

    if (size_estimate > max_size_estimate)
      break;
  }

  if (found_keys.empty()) {
    callbacks->OnSuccess(nullptr);
    return s;
  }

  callbacks->OnSuccessWithPrefetch(
      found_keys, found_primary_keys, &found_values);
  return s;
}

leveldb::Status IndexedDBCursor::PrefetchReset(int used_prefetches,
                                               int /* unused_prefetches */) {
  IDB_TRACE("IndexedDBCursor::PrefetchReset");
  cursor_.swap(saved_cursor_);
  saved_cursor_.reset();
  leveldb::Status s;

  if (closed_)
    return s;
  // First prefetched result is always used.
  if (cursor_){
    DCHECK_GT(used_prefetches, 0);
    for (int i = 0; i < used_prefetches - 1; ++i) {
      bool ok = cursor_->Continue(&s);
      DCHECK(ok);
    }
  }

  return s;
}

void IndexedDBCursor::Close() {
  if (closed_)
    return;
  IDB_ASYNC_TRACE_END("IndexedDBCursor::open", this);
  IDB_TRACE("IndexedDBCursor::Close");
  closed_ = true;
  cursor_.reset();
  saved_cursor_.reset();
  transaction_ = nullptr;
}

}  // namespace content
