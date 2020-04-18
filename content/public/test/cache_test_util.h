// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BROWSING_DATA_CACHE_TEST_UTIL_H_
#define CONTENT_BROWSER_BROWSING_DATA_CACHE_TEST_UTIL_H_

#include <set>
#include <vector>

#include "base/run_loop.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/disk_cache/disk_cache.h"

namespace base {
class WaitableEvent;
}

namespace content {

// A util class that can be used to create and retreive cache entries.
class CacheTestUtil {
 public:
  explicit CacheTestUtil(StoragePartition* partition_);

  ~CacheTestUtil();

  void CreateCacheEntries(const std::set<std::string>& keys);

  std::vector<std::string> GetEntryKeys();

  StoragePartition* partition() { return partition_; }
  disk_cache::Backend* backend() { return backend_; }

 private:
  void SetUpOnIOThread();
  void TearDownOnIOThread();

  void CreateCacheEntriesOnIOThread(const std::set<std::string>& keys);

  void GetEntryKeysOnIOThread();
  void GetNextKey(int error);

  void WaitForTasksOnIOThread();
  void WaitForCompletion(int value);
  void SetNumberOfWaitedTasks(int count);

  void DoneCallback(int value);

  base::Callback<void(int)> done_callback_;

  StoragePartition* partition_;
  disk_cache::Backend* backend_ = nullptr;
  std::vector<disk_cache::Entry*> entries_;
  std::unique_ptr<disk_cache::Backend::Iterator> iterator_;

  disk_cache::Entry* current_entry_;
  std::vector<std::string> keys_;

  std::unique_ptr<base::WaitableEvent> waitable_event_;
  int remaining_tasks_;
};

}  // content

#endif  // CONTENT_BROWSER_BROWSING_DATA_CACHE_TEST_UTIL_H_
