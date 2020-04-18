// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATABASE_CALLBACKS_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATABASE_CALLBACKS_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "content/common/content_export.h"
#include "content/common/indexed_db/indexed_db.mojom.h"
#include "content/public/browser/browser_thread.h"

namespace content {
class IndexedDBContextImpl;
class IndexedDBDatabaseError;
class IndexedDBTransaction;

// Expected to be constructed on IO thread and called/deleted from IDB sequence.
class CONTENT_EXPORT IndexedDBDatabaseCallbacks
    : public base::RefCounted<IndexedDBDatabaseCallbacks> {
 public:
  IndexedDBDatabaseCallbacks(
      scoped_refptr<IndexedDBContextImpl> context,
      ::indexed_db::mojom::DatabaseCallbacksAssociatedPtrInfo callbacks_info);

  virtual void OnForcedClose();
  virtual void OnVersionChange(int64_t old_version, int64_t new_version);

  virtual void OnAbort(const IndexedDBTransaction& transaction,
                       const IndexedDBDatabaseError& error);
  virtual void OnComplete(const IndexedDBTransaction& transaction);
  virtual void OnDatabaseChange(
      ::indexed_db::mojom::ObserverChangesPtr changes);

 protected:
  virtual ~IndexedDBDatabaseCallbacks();

 private:
  friend class base::RefCounted<IndexedDBDatabaseCallbacks>;

  class IOThreadHelper;

  bool complete_ = false;
  scoped_refptr<IndexedDBContextImpl> indexed_db_context_;
  std::unique_ptr<IOThreadHelper, BrowserThread::DeleteOnIOThread> io_helper_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(IndexedDBDatabaseCallbacks);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATABASE_CALLBACKS_H_
