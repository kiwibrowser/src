// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_SCHEDULER_H_
#define CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_SCHEDULER_H_

#include <list>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "content/browser/cache_storage/cache_storage_scheduler_client.h"
#include "content/common/content_export.h"

namespace content {

class CacheStorageOperation;

// TODO(jkarlin): Support readers and writers so operations can run in parallel.
// TODO(jkarlin): Support operation identification so that ops can be checked in
// DCHECKs.

// CacheStorageScheduler runs the scheduled callbacks sequentially. Add an
// operation by calling ScheduleOperation() with your callback. Once your
// operation is done be sure to call CompleteOperationAndRunNext() to schedule
// the next operation.
class CONTENT_EXPORT CacheStorageScheduler {
 public:
  explicit CacheStorageScheduler(CacheStorageSchedulerClient client_type);
  virtual ~CacheStorageScheduler();

  // Adds the operation to the tail of the queue and starts it if the scheduler
  // is idle.
  void ScheduleOperation(base::OnceClosure closure);

  // Call this after each operation completes. It cleans up the current
  // operation and starts the next.
  void CompleteOperationAndRunNext();

  // Returns true if there are any running or pending operations.
  bool ScheduledOperations() const;

  // Wraps |callback| to also call CompleteOperationAndRunNext.
  template <typename... Args>
  base::OnceCallback<void(Args...)> WrapCallbackToRunNext(
      base::OnceCallback<void(Args...)> callback) {
    return base::BindOnce(&CacheStorageScheduler::RunNextContinuation<Args...>,
                          weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  }

 private:
  void RunOperationIfIdle();

  template <typename... Args>
  void RunNextContinuation(base::OnceCallback<void(Args...)> callback,
                           Args... args) {
    // Grab a weak ptr to guard against the scheduler being deleted during the
    // callback.
    base::WeakPtr<CacheStorageScheduler> scheduler =
        weak_ptr_factory_.GetWeakPtr();

    std::move(callback).Run(std::forward<Args>(args)...);
    if (scheduler)
      CompleteOperationAndRunNext();
  }

  std::list<std::unique_ptr<CacheStorageOperation>> pending_operations_;
  std::unique_ptr<CacheStorageOperation> running_operation_;
  CacheStorageSchedulerClient client_type_;

  base::WeakPtrFactory<CacheStorageScheduler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CacheStorageScheduler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_SCHEDULER_H_
