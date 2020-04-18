// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/indexed_db/webidbcursor_impl.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/single_thread_task_runner.h"
#include "content/renderer/indexed_db/indexed_db_dispatcher.h"
#include "content/renderer/indexed_db/indexed_db_key_builders.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_value.h"

using blink::WebBlobInfo;
using blink::WebData;
using blink::WebIDBCallbacks;
using blink::WebIDBKey;
using blink::WebIDBKeyView;
using blink::WebIDBValue;
using indexed_db::mojom::CallbacksAssociatedPtrInfo;
using indexed_db::mojom::CursorAssociatedPtrInfo;

namespace content {

class WebIDBCursorImpl::IOThreadHelper {
 public:
  explicit IOThreadHelper(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~IOThreadHelper();

  void Bind(CursorAssociatedPtrInfo cursor_info);
  void Advance(uint32_t count,
               std::unique_ptr<IndexedDBCallbacksImpl> callbacks);
  void Continue(const IndexedDBKey& key,
                const IndexedDBKey& primary_key,
                std::unique_ptr<IndexedDBCallbacksImpl> callbacks);
  void Prefetch(int32_t count,
                std::unique_ptr<IndexedDBCallbacksImpl> callbacks);
  void PrefetchReset(int32_t used_prefetches, int32_t unused_prefetches);

 private:
  CallbacksAssociatedPtrInfo GetCallbacksProxy(
      std::unique_ptr<IndexedDBCallbacksImpl> callbacks);

  indexed_db::mojom::CursorAssociatedPtr cursor_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(IOThreadHelper);
};

WebIDBCursorImpl::WebIDBCursorImpl(
    indexed_db::mojom::CursorAssociatedPtrInfo cursor_info,
    int64_t transaction_id,
    scoped_refptr<base::SingleThreadTaskRunner> io_runner,
    scoped_refptr<base::SingleThreadTaskRunner> callback_runner)
    : transaction_id_(transaction_id),
      helper_(new IOThreadHelper(io_runner)),
      io_runner_(std::move(io_runner)),
      callback_runner_(std::move(callback_runner)),
      continue_count_(0),
      used_prefetches_(0),
      pending_onsuccess_callbacks_(0),
      prefetch_amount_(kMinPrefetchAmount),
      weak_factory_(this) {
  IndexedDBDispatcher::ThreadSpecificInstance()->RegisterCursor(this);
  io_runner_->PostTask(FROM_HERE, base::BindOnce(&IOThreadHelper::Bind,
                                                 base::Unretained(helper_),
                                                 std::move(cursor_info)));
}

WebIDBCursorImpl::~WebIDBCursorImpl() {
  // It's not possible for there to be pending callbacks that address this
  // object since inside WebKit, they hold a reference to the object which owns
  // this object. But, if that ever changed, then we'd need to invalidate
  // any such pointers.
  IndexedDBDispatcher::ThreadSpecificInstance()->UnregisterCursor(this);
  io_runner_->DeleteSoon(FROM_HERE, helper_);
}

void WebIDBCursorImpl::Advance(unsigned long count,
                               WebIDBCallbacks* callbacks_ptr) {
  std::unique_ptr<WebIDBCallbacks> callbacks(callbacks_ptr);
  if (count <= prefetch_keys_.size()) {
    CachedAdvance(count, callbacks.get());
    return;
  }
  ResetPrefetchCache();

  // Reset all cursor prefetch caches except for this cursor.
  IndexedDBDispatcher::ThreadSpecificInstance()->ResetCursorPrefetchCaches(
      transaction_id_, this);

  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      std::move(callbacks), transaction_id_, weak_factory_.GetWeakPtr(),
      io_runner_, callback_runner_);
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::Advance, base::Unretained(helper_), count,
                     std::move(callbacks_impl)));
}

void WebIDBCursorImpl::Continue(WebIDBKeyView key,
                                WebIDBKeyView primary_key,
                                WebIDBCallbacks* callbacks_ptr) {
  std::unique_ptr<WebIDBCallbacks> callbacks(callbacks_ptr);

  if (key.KeyType() == blink::kWebIDBKeyTypeNull &&
      primary_key.KeyType() == blink::kWebIDBKeyTypeNull) {
    // No key(s), so this would qualify for a prefetch.
    ++continue_count_;

    if (!prefetch_keys_.empty()) {
      // We have a prefetch cache, so serve the result from that.
      CachedContinue(callbacks.get());
      return;
    }

    if (continue_count_ > kPrefetchContinueThreshold) {
      // Request pre-fetch.
      ++pending_onsuccess_callbacks_;

      auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
          std::move(callbacks), transaction_id_, weak_factory_.GetWeakPtr(),
          io_runner_, callback_runner_);
      io_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(&IOThreadHelper::Prefetch, base::Unretained(helper_),
                         prefetch_amount_, std::move(callbacks_impl)));

      // Increase prefetch_amount_ exponentially.
      prefetch_amount_ *= 2;
      if (prefetch_amount_ > kMaxPrefetchAmount)
        prefetch_amount_ = kMaxPrefetchAmount;

      return;
    }
  } else {
    // Key argument supplied. We couldn't prefetch this.
    ResetPrefetchCache();
  }

  // Reset all cursor prefetch caches except for this cursor.
  IndexedDBDispatcher::ThreadSpecificInstance()->ResetCursorPrefetchCaches(
      transaction_id_, this);

  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      std::move(callbacks), transaction_id_, weak_factory_.GetWeakPtr(),
      io_runner_, callback_runner_);
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::Continue, base::Unretained(helper_),
                     IndexedDBKeyBuilder::Build(key),
                     IndexedDBKeyBuilder::Build(primary_key),
                     std::move(callbacks_impl)));
}

void WebIDBCursorImpl::PostSuccessHandlerCallback() {
  pending_onsuccess_callbacks_--;

  // If the onsuccess callback called continue()/advance() on the cursor
  // again, and that request was served by the prefetch cache, then
  // pending_onsuccess_callbacks_ would be incremented. If not, it means the
  // callback did something else, or nothing at all, in which case we need to
  // reset the cache.

  if (pending_onsuccess_callbacks_ == 0)
    ResetPrefetchCache();
}

void WebIDBCursorImpl::SetPrefetchData(
    const std::vector<IndexedDBKey>& keys,
    const std::vector<IndexedDBKey>& primary_keys,
    std::vector<WebIDBValue> values) {
  prefetch_keys_.assign(keys.begin(), keys.end());
  prefetch_primary_keys_.assign(primary_keys.begin(), primary_keys.end());
  prefetch_values_.assign(std::make_move_iterator(values.begin()),
                          std::make_move_iterator(values.end()));

  used_prefetches_ = 0;
  pending_onsuccess_callbacks_ = 0;
}

void WebIDBCursorImpl::CachedAdvance(unsigned long count,
                                     WebIDBCallbacks* callbacks) {
  DCHECK_GE(prefetch_keys_.size(), count);
  DCHECK_EQ(prefetch_primary_keys_.size(), prefetch_keys_.size());
  DCHECK_EQ(prefetch_values_.size(), prefetch_keys_.size());

  while (count > 1) {
    prefetch_keys_.pop_front();
    prefetch_primary_keys_.pop_front();
    prefetch_values_.pop_front();
    ++used_prefetches_;
    --count;
  }

  CachedContinue(callbacks);
}

void WebIDBCursorImpl::CachedContinue(WebIDBCallbacks* callbacks) {
  DCHECK_GT(prefetch_keys_.size(), 0ul);
  DCHECK_EQ(prefetch_primary_keys_.size(), prefetch_keys_.size());
  DCHECK_EQ(prefetch_values_.size(), prefetch_keys_.size());

  IndexedDBKey key = prefetch_keys_.front();
  IndexedDBKey primary_key = prefetch_primary_keys_.front();
  WebIDBValue value = std::move(prefetch_values_.front());

  prefetch_keys_.pop_front();
  prefetch_primary_keys_.pop_front();
  prefetch_values_.pop_front();
  ++used_prefetches_;

  ++pending_onsuccess_callbacks_;

  if (!continue_count_) {
    // The cache was invalidated by a call to ResetPrefetchCache()
    // after the RequestIDBCursorPrefetch() was made. Now that the
    // initiating continue() call has been satisfied, discard
    // the rest of the cache.
    ResetPrefetchCache();
  }

  callbacks->OnSuccess(WebIDBKeyBuilder::Build(key),
                       WebIDBKeyBuilder::Build(primary_key), std::move(value));
}

void WebIDBCursorImpl::ResetPrefetchCache() {
  continue_count_ = 0;
  prefetch_amount_ = kMinPrefetchAmount;

  if (prefetch_keys_.empty()) {
    // No prefetch cache, so no need to reset the cursor in the back-end.
    return;
  }

  // Reset the back-end cursor.
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::PrefetchReset, base::Unretained(helper_),
                     used_prefetches_, prefetch_keys_.size()));

  // Reset the prefetch cache.
  prefetch_keys_.clear();
  prefetch_primary_keys_.clear();
  prefetch_values_.clear();

  pending_onsuccess_callbacks_ = 0;
}

WebIDBCursorImpl::IOThreadHelper::IOThreadHelper(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(std::move(task_runner)) {}

WebIDBCursorImpl::IOThreadHelper::~IOThreadHelper() {}

void WebIDBCursorImpl::IOThreadHelper::Bind(
    CursorAssociatedPtrInfo cursor_info) {
  cursor_.Bind(std::move(cursor_info), task_runner_);
}

void WebIDBCursorImpl::IOThreadHelper::Advance(
    uint32_t count,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  cursor_->Advance(count, GetCallbacksProxy(std::move(callbacks)));
}

void WebIDBCursorImpl::IOThreadHelper::Continue(
    const IndexedDBKey& key,
    const IndexedDBKey& primary_key,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  cursor_->Continue(key, primary_key, GetCallbacksProxy(std::move(callbacks)));
}

void WebIDBCursorImpl::IOThreadHelper::Prefetch(
    int32_t count,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  cursor_->Prefetch(count, GetCallbacksProxy(std::move(callbacks)));
}

void WebIDBCursorImpl::IOThreadHelper::PrefetchReset(
    int32_t used_prefetches,
    int32_t unused_prefetches) {
  cursor_->PrefetchReset(used_prefetches, unused_prefetches);
}

CallbacksAssociatedPtrInfo WebIDBCursorImpl::IOThreadHelper::GetCallbacksProxy(
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  CallbacksAssociatedPtrInfo ptr_info;
  auto request = mojo::MakeRequest(&ptr_info);
  mojo::MakeStrongAssociatedBinding(std::move(callbacks), std::move(request));
  return ptr_info;
}

}  // namespace content
