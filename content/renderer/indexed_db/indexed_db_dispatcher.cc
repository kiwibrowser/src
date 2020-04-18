// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/indexed_db/indexed_db_dispatcher.h"

#include <utility>

#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_local.h"
#include "content/renderer/indexed_db/indexed_db_key_builders.h"
#include "content/renderer/indexed_db/webidbcursor_impl.h"
#include "ipc/ipc_channel.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_database_callbacks.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_observation.h"

using blink::WebIDBKey;
using blink::WebIDBObservation;
using base::ThreadLocalPointer;

namespace content {
static base::LazyInstance<ThreadLocalPointer<IndexedDBDispatcher>>::Leaky
    g_idb_dispatcher_tls = LAZY_INSTANCE_INITIALIZER;

namespace {

IndexedDBDispatcher* const kDeletedIndexedDBDispatcherMarker =
    reinterpret_cast<IndexedDBDispatcher*>(0x1);

}  // unnamed namespace

IndexedDBDispatcher::IndexedDBDispatcher() {
  g_idb_dispatcher_tls.Pointer()->Set(this);
}

IndexedDBDispatcher::~IndexedDBDispatcher() {
  in_destructor_ = true;
  mojo_owned_callback_state_.clear();
  mojo_owned_database_callback_state_.clear();

  g_idb_dispatcher_tls.Pointer()->Set(kDeletedIndexedDBDispatcherMarker);
}

IndexedDBDispatcher* IndexedDBDispatcher::ThreadSpecificInstance() {
  if (g_idb_dispatcher_tls.Pointer()->Get() ==
      kDeletedIndexedDBDispatcherMarker) {
    NOTREACHED() << "Re-instantiating TLS IndexedDBDispatcher.";
    g_idb_dispatcher_tls.Pointer()->Set(nullptr);
  }
  if (g_idb_dispatcher_tls.Pointer()->Get())
    return g_idb_dispatcher_tls.Pointer()->Get();

  IndexedDBDispatcher* dispatcher = new IndexedDBDispatcher();
  if (WorkerThread::GetCurrentId())
    WorkerThread::AddObserver(dispatcher);
  return dispatcher;
}

void IndexedDBDispatcher::WillStopCurrentWorkerThread() {
  delete this;
}

void IndexedDBDispatcher::RegisterMojoOwnedCallbacks(
    IndexedDBCallbacksImpl::InternalState* callbacks) {
  mojo_owned_callback_state_[callbacks] = base::WrapUnique(callbacks);
}

void IndexedDBDispatcher::UnregisterMojoOwnedCallbacks(
    IndexedDBCallbacksImpl::InternalState* callbacks) {
  if (in_destructor_)
    return;

  auto it = mojo_owned_callback_state_.find(callbacks);
  DCHECK(it != mojo_owned_callback_state_.end());
  it->second.release();
  mojo_owned_callback_state_.erase(it);
}

void IndexedDBDispatcher::RegisterMojoOwnedDatabaseCallbacks(
    blink::WebIDBDatabaseCallbacks* callbacks) {
  mojo_owned_database_callback_state_[callbacks] = base::WrapUnique(callbacks);
}

void IndexedDBDispatcher::UnregisterMojoOwnedDatabaseCallbacks(
    blink::WebIDBDatabaseCallbacks* callbacks) {
  if (in_destructor_)
    return;

  auto it = mojo_owned_database_callback_state_.find(callbacks);
  DCHECK(it != mojo_owned_database_callback_state_.end());
  it->second.release();
  mojo_owned_database_callback_state_.erase(it);
}

void IndexedDBDispatcher::RegisterCursor(WebIDBCursorImpl* cursor) {
  DCHECK(!base::ContainsKey(cursors_, cursor));
  cursors_.insert(cursor);
}

void IndexedDBDispatcher::UnregisterCursor(WebIDBCursorImpl* cursor) {
  DCHECK(base::ContainsKey(cursors_, cursor));
  cursors_.erase(cursor);
}

void IndexedDBDispatcher::ResetCursorPrefetchCaches(
    int64_t transaction_id,
    WebIDBCursorImpl* exception_cursor) {
  for (WebIDBCursorImpl* cursor : cursors_) {
    if (cursor != exception_cursor &&
        cursor->transaction_id() == transaction_id)
      cursor->ResetPrefetchCache();
  }
}

}  // namespace content
