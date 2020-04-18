// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_DISK_CACHE_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_DISK_CACHE_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "content/browser/appcache/appcache_response.h"
#include "content/common/content_export.h"
#include "net/disk_cache/disk_cache.h"

namespace content {

// An implementation of AppCacheDiskCacheInterface that
// uses net::DiskCache as the backing store.
class CONTENT_EXPORT AppCacheDiskCache
    : public AppCacheDiskCacheInterface {
 public:
  AppCacheDiskCache();
  ~AppCacheDiskCache() override;

  // Initializes the object to use disk backed storage.
  int InitWithDiskBackend(const base::FilePath& disk_cache_directory,
                          int disk_cache_size,
                          bool force,
                          base::OnceClosure post_cleanup_callback,
                          const net::CompletionCallback& callback);

  // Initializes the object to use memory only storage.
  // This is used for Chrome's incognito browsing.
  int InitWithMemBackend(int disk_cache_size,
                         const net::CompletionCallback& callback);

  void Disable();
  bool is_disabled() const { return is_disabled_; }

  int CreateEntry(int64_t key,
                  Entry** entry,
                  const net::CompletionCallback& callback) override;
  int OpenEntry(int64_t key,
                Entry** entry,
                const net::CompletionCallback& callback) override;
  int DoomEntry(int64_t key, const net::CompletionCallback& callback) override;

  void set_is_waiting_to_initialize(bool is_waiting_to_initialize) {
    is_waiting_to_initialize_ = is_waiting_to_initialize;
  }

 protected:
  explicit AppCacheDiskCache(bool use_simple_cache);
  disk_cache::Backend* disk_cache() { return disk_cache_.get(); }

 private:
  class CreateBackendCallbackShim;
  class EntryImpl;

  // PendingCalls allow CreateEntry, OpenEntry, and DoomEntry to be called
  // immediately after construction, without waiting for the
  // underlying disk_cache::Backend to be fully constructed. Early
  // calls are queued up and serviced once the disk_cache::Backend is
  // really ready to go.
  enum PendingCallType {
    CREATE,
    OPEN,
    DOOM
  };
  struct PendingCall {
    PendingCallType call_type;
    int64_t key;
    Entry** entry;
    net::CompletionCallback callback;

    PendingCall();

    PendingCall(PendingCallType call_type,
                int64_t key,
                Entry** entry,
                const net::CompletionCallback& callback);

    PendingCall(const PendingCall& other);

    ~PendingCall();
  };
  using PendingCalls = std::vector<PendingCall>;

  class ActiveCall;
  using ActiveCalls = std::set<ActiveCall*>;
  using OpenEntries = std::set<EntryImpl*>;

  bool is_initializing_or_waiting_to_initialize() const {
    return create_backend_callback_.get() != NULL || is_waiting_to_initialize_;
  }

  int Init(net::CacheType cache_type,
           const base::FilePath& directory,
           int cache_size,
           bool force,
           base::OnceClosure post_cleanup_callback,
           const net::CompletionCallback& callback);
  void OnCreateBackendComplete(int rv);
  void AddOpenEntry(EntryImpl* entry) { open_entries_.insert(entry); }
  void RemoveOpenEntry(EntryImpl* entry) { open_entries_.erase(entry); }

  bool use_simple_cache_;
  bool is_disabled_;
  bool is_waiting_to_initialize_;
  net::CompletionCallback init_callback_;
  scoped_refptr<CreateBackendCallbackShim> create_backend_callback_;
  PendingCalls pending_calls_;
  OpenEntries open_entries_;
  std::unique_ptr<disk_cache::Backend> disk_cache_;

  base::WeakPtrFactory<AppCacheDiskCache> weak_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_DISK_CACHE_H_
