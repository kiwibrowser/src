// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_pending_connection.h"

namespace content {

IndexedDBPendingConnection::IndexedDBPendingConnection(
    scoped_refptr<IndexedDBCallbacks> callbacks,
    scoped_refptr<IndexedDBDatabaseCallbacks> database_callbacks,
    int child_process_id,
    int64_t transaction_id,
    int64_t version)
    : callbacks(callbacks),
      database_callbacks(database_callbacks),
      child_process_id(child_process_id),
      transaction_id(transaction_id),
      version(version) {}

IndexedDBPendingConnection::IndexedDBPendingConnection(
    const IndexedDBPendingConnection& other) = default;

IndexedDBPendingConnection::~IndexedDBPendingConnection() {}

}  // namespace content
