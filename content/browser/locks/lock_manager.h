// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOCKS_LOCK_CONTEXT_H_
#define CONTENT_BROWSER_LOCKS_LOCK_CONTEXT_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/blink/public/platform/modules/locks/lock_manager.mojom.h"
#include "url/origin.h"

namespace content {

// One instance of this exists per StoragePartition, and services multiple
// child processes/origins. An instance must only be used on the sequence
// it was created on.
class LockManager : public base::RefCountedThreadSafe<LockManager>,
                    public blink::mojom::LockManager {
 public:
  LockManager();

  void CreateService(blink::mojom::LockManagerRequest request,
                     const url::Origin& origin);

  // Request a lock. When the lock is acquired, |callback| will be invoked with
  // a LockHandle.
  void RequestLock(const std::string& name,
                   blink::mojom::LockMode mode,
                   WaitMode wait,
                   blink::mojom::LockRequestPtr request) override;

  // Called by a LockHandle's implementation when destructed.
  void ReleaseLock(const url::Origin& origin, int64_t lock_id);

  // Called to request a snapshot of the current lock state for an origin.
  void QueryState(QueryStateCallback callback) override;

 protected:
  friend class base::RefCountedThreadSafe<LockManager>;
  ~LockManager() override;

 private:
  // Internal representation of a lock request or held lock.
  class Lock;

  // State for a particular origin.
  class OriginState;

  // State for each client held in |bindings_|.
  struct BindingState {
    url::Origin origin;
    std::string client_id;
  };

  bool IsGrantable(const url::Origin& origin,
                   const std::string& name,
                   blink::mojom::LockMode mode) const;

  // Mints a monotonically increasing identifier. Used both for lock requests
  // and granted locks as keys in ordered maps.
  int64_t NextLockId();

  void Break(const url::Origin& origin, const std::string& name);

  // Called when a lock is requested and optionally when a lock is released,
  // to process outstanding requests within the origin.
  void ProcessRequests(const url::Origin& origin);

  mojo::BindingSet<blink::mojom::LockManager, BindingState> bindings_;

  int64_t next_lock_id_ = 0;
  std::map<url::Origin, OriginState> origins_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<LockManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LockManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOCKS_LOCK_CONTEXT_H
