// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_disk_cache.h"

#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/cache_type.h"
#include "net/base/net_errors.h"

namespace content {

// A callback shim that provides storage for the 'backend_ptr' value
// and will delete a resulting ptr if completion occurs after its
// been canceled.
class AppCacheDiskCache::CreateBackendCallbackShim
    : public base::RefCounted<CreateBackendCallbackShim> {
 public:
  explicit CreateBackendCallbackShim(AppCacheDiskCache* object)
      : appcache_diskcache_(object) {
  }

  void Cancel() { appcache_diskcache_ = nullptr; }

  void Callback(int rv) {
    if (appcache_diskcache_)
      appcache_diskcache_->OnCreateBackendComplete(rv);
  }

  std::unique_ptr<disk_cache::Backend> backend_ptr_;  // Accessed directly.

 private:
  friend class base::RefCounted<CreateBackendCallbackShim>;

  ~CreateBackendCallbackShim() {
  }

  AppCacheDiskCache* appcache_diskcache_;  // Unowned pointer.
};

// An implementation of AppCacheDiskCacheInterface::Entry that's a thin
// wrapper around disk_cache::Entry.
class AppCacheDiskCache::EntryImpl : public Entry {
 public:
  EntryImpl(disk_cache::Entry* disk_cache_entry,
            AppCacheDiskCache* owner)
      : disk_cache_entry_(disk_cache_entry), owner_(owner) {
    DCHECK(disk_cache_entry);
    DCHECK(owner);
    owner_->AddOpenEntry(this);
  }

  // Entry implementation.
  int Read(int index,
           int64_t offset,
           net::IOBuffer* buf,
           int buf_len,
           const net::CompletionCallback& callback) override {
    if (offset < 0 || offset > std::numeric_limits<int32_t>::max())
      return net::ERR_INVALID_ARGUMENT;
    if (!disk_cache_entry_)
      return net::ERR_ABORTED;
    return disk_cache_entry_->ReadData(
        index, static_cast<int>(offset), buf, buf_len, callback);
  }

  int Write(int index,
            int64_t offset,
            net::IOBuffer* buf,
            int buf_len,
            const net::CompletionCallback& callback) override {
    if (offset < 0 || offset > std::numeric_limits<int32_t>::max())
      return net::ERR_INVALID_ARGUMENT;
    if (!disk_cache_entry_)
      return net::ERR_ABORTED;
    const bool kTruncate = true;
    return disk_cache_entry_->WriteData(
        index, static_cast<int>(offset), buf, buf_len, callback, kTruncate);
  }

  int64_t GetSize(int index) override {
    return disk_cache_entry_ ? disk_cache_entry_->GetDataSize(index) : 0L;
  }

  void Close() override {
    if (disk_cache_entry_)
      disk_cache_entry_->Close();
    delete this;
  }

  void Abandon() {
    owner_ = nullptr;
    disk_cache_entry_->Close();
    disk_cache_entry_ = nullptr;
  }

 private:
  ~EntryImpl() override {
    if (owner_)
      owner_->RemoveOpenEntry(this);
  }

  disk_cache::Entry* disk_cache_entry_;
  AppCacheDiskCache* owner_;
};

// Separate object to hold state for each Create, Delete, or Doom call
// while the call is in-flight and to produce an EntryImpl upon completion.
class AppCacheDiskCache::ActiveCall
    : public base::RefCounted<AppCacheDiskCache::ActiveCall> {
 public:
  static int CreateEntry(const base::WeakPtr<AppCacheDiskCache>& owner,
                         int64_t key,
                         Entry** entry,
                         const net::CompletionCallback& callback) {
    scoped_refptr<ActiveCall> active_call(
        new ActiveCall(owner, entry, callback));
    int rv = owner->disk_cache()->CreateEntry(
        base::Int64ToString(key), net::HIGHEST, &active_call->entry_ptr_,
        base::Bind(&ActiveCall::OnAsyncCompletion, active_call));
    return active_call->HandleImmediateReturnValue(rv);
  }

  static int OpenEntry(const base::WeakPtr<AppCacheDiskCache>& owner,
                       int64_t key,
                       Entry** entry,
                       const net::CompletionCallback& callback) {
    scoped_refptr<ActiveCall> active_call(
        new ActiveCall(owner, entry, callback));
    int rv = owner->disk_cache()->OpenEntry(
        base::Int64ToString(key), net::HIGHEST, &active_call->entry_ptr_,
        base::Bind(&ActiveCall::OnAsyncCompletion, active_call));
    return active_call->HandleImmediateReturnValue(rv);
  }

  static int DoomEntry(const base::WeakPtr<AppCacheDiskCache>& owner,
                       int64_t key,
                       const net::CompletionCallback& callback) {
    scoped_refptr<ActiveCall> active_call(
        new ActiveCall(owner, nullptr, callback));
    int rv = owner->disk_cache()->DoomEntry(
        base::Int64ToString(key), net::HIGHEST,
        base::Bind(&ActiveCall::OnAsyncCompletion, active_call));
    return active_call->HandleImmediateReturnValue(rv);
  }

 private:
  friend class base::RefCounted<AppCacheDiskCache::ActiveCall>;

  ActiveCall(const base::WeakPtr<AppCacheDiskCache>& owner,
             Entry** entry,
             const net::CompletionCallback& callback)
      : owner_(owner),
        entry_(entry),
        callback_(callback),
        entry_ptr_(nullptr) {
    DCHECK(owner_);
  }

  ~ActiveCall() {}

  int HandleImmediateReturnValue(int rv) {
    if (rv == net::ERR_IO_PENDING) {
      // OnAsyncCompletion will be called later.
      return rv;
    }

    if (rv == net::OK && entry_) {
      DCHECK(entry_ptr_);
      *entry_ = new EntryImpl(entry_ptr_, owner_.get());
    }
    return rv;
  }

  void OnAsyncCompletion(int rv) {
    if (rv == net::OK && entry_) {
      DCHECK(entry_ptr_);
      if (owner_) {
        *entry_ = new EntryImpl(entry_ptr_, owner_.get());
      } else {
        entry_ptr_->Close();
        rv = net::ERR_ABORTED;
      }
    }
    callback_.Run(rv);
  }

  base::WeakPtr<AppCacheDiskCache> owner_;
  Entry** entry_;
  net::CompletionCallback callback_;
  disk_cache::Entry* entry_ptr_;
};

AppCacheDiskCache::AppCacheDiskCache()
#if defined(APPCACHE_USE_SIMPLE_CACHE)
    : AppCacheDiskCache(true)
#else
    : AppCacheDiskCache(false)
#endif
{
}

AppCacheDiskCache::~AppCacheDiskCache() {
  Disable();
}

int AppCacheDiskCache::InitWithDiskBackend(
    const base::FilePath& disk_cache_directory,
    int disk_cache_size,
    bool force,
    base::OnceClosure post_cleanup_callback,
    const net::CompletionCallback& callback) {
  return Init(net::APP_CACHE, disk_cache_directory, disk_cache_size, force,
              std::move(post_cleanup_callback), callback);
}

int AppCacheDiskCache::InitWithMemBackend(
    int mem_cache_size, const net::CompletionCallback& callback) {
  return Init(net::MEMORY_CACHE, base::FilePath(), mem_cache_size, false,
              base::OnceClosure(), callback);
}

void AppCacheDiskCache::Disable() {
  if (is_disabled_)
    return;

  is_disabled_ = true;

  if (create_backend_callback_.get()) {
    create_backend_callback_->Cancel();
    create_backend_callback_ = nullptr;
    OnCreateBackendComplete(net::ERR_ABORTED);
  }

  // We need to close open file handles in order to reinitalize the
  // appcache system on the fly. File handles held in both entries and in
  // the main disk_cache::Backend class need to be released.
  for (EntryImpl* entry : open_entries_) {
    entry->Abandon();
  }
  open_entries_.clear();
  disk_cache_.reset();
}

int AppCacheDiskCache::CreateEntry(int64_t key,
                                   Entry** entry,
                                   const net::CompletionCallback& callback) {
  DCHECK(entry);
  DCHECK(!callback.is_null());
  if (is_disabled_)
    return net::ERR_ABORTED;

  if (is_initializing_or_waiting_to_initialize()) {
    pending_calls_.push_back(PendingCall(CREATE, key, entry, callback));
    return net::ERR_IO_PENDING;
  }

  if (!disk_cache_)
    return net::ERR_FAILED;

  return ActiveCall::CreateEntry(
      weak_factory_.GetWeakPtr(), key, entry, callback);
}

int AppCacheDiskCache::OpenEntry(int64_t key,
                                 Entry** entry,
                                 const net::CompletionCallback& callback) {
  DCHECK(entry);
  DCHECK(!callback.is_null());
  if (is_disabled_)
    return net::ERR_ABORTED;

  if (is_initializing_or_waiting_to_initialize()) {
    pending_calls_.push_back(PendingCall(OPEN, key, entry, callback));
    return net::ERR_IO_PENDING;
  }

  if (!disk_cache_)
    return net::ERR_FAILED;

  return ActiveCall::OpenEntry(
      weak_factory_.GetWeakPtr(), key, entry, callback);
}

int AppCacheDiskCache::DoomEntry(int64_t key,
                                 const net::CompletionCallback& callback) {
  DCHECK(!callback.is_null());
  if (is_disabled_)
    return net::ERR_ABORTED;

  if (is_initializing_or_waiting_to_initialize()) {
    pending_calls_.push_back(PendingCall(DOOM, key, nullptr, callback));
    return net::ERR_IO_PENDING;
  }

  if (!disk_cache_)
    return net::ERR_FAILED;

  return ActiveCall::DoomEntry(weak_factory_.GetWeakPtr(), key, callback);
}

AppCacheDiskCache::AppCacheDiskCache(bool use_simple_cache)
    : AppCacheDiskCacheInterface("DiskCache.AppCache"),
      use_simple_cache_(use_simple_cache),
      is_disabled_(false),
      is_waiting_to_initialize_(false),
      weak_factory_(this) {}

AppCacheDiskCache::PendingCall::PendingCall()
    : call_type(CREATE), key(0), entry(nullptr) {}

AppCacheDiskCache::PendingCall::PendingCall(
    PendingCallType call_type,
    int64_t key,
    Entry** entry,
    const net::CompletionCallback& callback)
    : call_type(call_type), key(key), entry(entry), callback(callback) {}

AppCacheDiskCache::PendingCall::PendingCall(const PendingCall& other) = default;

AppCacheDiskCache::PendingCall::~PendingCall() {}

int AppCacheDiskCache::Init(net::CacheType cache_type,
                            const base::FilePath& cache_directory,
                            int cache_size,
                            bool force,
                            base::OnceClosure post_cleanup_callback,
                            const net::CompletionCallback& callback) {
  DCHECK(!is_initializing_or_waiting_to_initialize() && !disk_cache_.get());
  is_disabled_ = false;
  create_backend_callback_ = new CreateBackendCallbackShim(this);

  int rv = disk_cache::CreateCacheBackend(
      cache_type,
      use_simple_cache_ ? net::CACHE_BACKEND_SIMPLE
                        : net::CACHE_BACKEND_DEFAULT,
      cache_directory, cache_size, force, nullptr,
      &(create_backend_callback_->backend_ptr_),
      std::move(post_cleanup_callback),
      base::Bind(&CreateBackendCallbackShim::Callback,
                 create_backend_callback_));
  if (rv == net::ERR_IO_PENDING)
    init_callback_ = callback;
  else
    OnCreateBackendComplete(rv);
  return rv;
}

void AppCacheDiskCache::OnCreateBackendComplete(int rv) {
  if (rv == net::OK) {
    disk_cache_ = std::move(create_backend_callback_->backend_ptr_);
  }
  create_backend_callback_ = nullptr;

  // Invoke our clients callback function.
  if (!init_callback_.is_null()) {
    init_callback_.Run(rv);
    init_callback_.Reset();
  }

  // Service pending calls that were queued up while we were initializing.
  for (const auto& call : pending_calls_) {
    rv = net::ERR_FAILED;
    switch (call.call_type) {
      case CREATE:
        rv = CreateEntry(call.key, call.entry, call.callback);
        break;
      case OPEN:
        rv = OpenEntry(call.key, call.entry, call.callback);
        break;
      case DOOM:
        rv = DoomEntry(call.key, call.callback);
        break;
      default:
        NOTREACHED();
        break;
    }
    if (rv != net::ERR_IO_PENDING)
      call.callback.Run(rv);
  }
  pending_calls_.clear();
}

}  // namespace content
