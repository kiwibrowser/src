// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_EMBEDDED_WORKER_REGISTRY_H_
#define CONTENT_BROWSER_SERVICE_WORKER_EMBEDDED_WORKER_REGISTRY_H_

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "content/browser/service_worker/service_worker_lifetime_tracker.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_status_code.h"

namespace content {

class EmbeddedWorkerInstance;
class ServiceWorkerContextCore;
class ServiceWorkerVersion;

// Acts as a thin stub between MessageFilter and each EmbeddedWorkerInstance,
// which sends/receives messages to/from each EmbeddedWorker in child process.
//
// Hangs off ServiceWorkerContextCore (its reference is also held by each
// EmbeddedWorkerInstance).  Operated only on IO thread.
class CONTENT_EXPORT EmbeddedWorkerRegistry
    : public base::RefCounted<EmbeddedWorkerRegistry> {
 public:
  static scoped_refptr<EmbeddedWorkerRegistry> Create(
      const base::WeakPtr<ServiceWorkerContextCore>& context);

  // Used for DeleteAndStartOver. Creates a new registry which takes over
  // |next_embedded_worker_id_| and |process_sender_map_| from |old_registry|.
  static scoped_refptr<EmbeddedWorkerRegistry> Create(
      const base::WeakPtr<ServiceWorkerContextCore>& context,
      EmbeddedWorkerRegistry* old_registry);

  // Creates and removes a new worker instance entry for bookkeeping.
  // This doesn't actually start or stop the worker.
  std::unique_ptr<EmbeddedWorkerInstance> CreateWorker(
      ServiceWorkerVersion* owner_version);

  // Stop all running workers, even if they're handling events.
  void Shutdown();

  // Called by EmbeddedWorkerInstance when it starts or stops. This registry
  // keeps track of running workers.
  bool OnWorkerStarted(int process_id, int embedded_worker_id);
  void OnWorkerStopped(int process_id, int embedded_worker_id);

  // Called by EmbeddedWorkerInstance when it learns DevTools has attached to
  // it.
  void OnDevToolsAttached(int embedded_worker_id);

  // Returns an embedded worker instance for given |embedded_worker_id|.
  EmbeddedWorkerInstance* GetWorker(int embedded_worker_id);

  // Returns true if |embedded_worker_id| is managed by this registry.
  bool CanHandle(int embedded_worker_id) const;

 private:
  friend class base::RefCounted<EmbeddedWorkerRegistry>;
  friend class MojoEmbeddedWorkerInstanceTest;
  friend class EmbeddedWorkerInstance;
  friend class EmbeddedWorkerInstanceTest;
  FRIEND_TEST_ALL_PREFIXES(EmbeddedWorkerInstanceTest,
                           RemoveWorkerInSharedProcess);

  using WorkerInstanceMap = std::map<int, EmbeddedWorkerInstance*>;

  EmbeddedWorkerRegistry(
      const base::WeakPtr<ServiceWorkerContextCore>& context,
      int initial_embedded_worker_id);
  ~EmbeddedWorkerRegistry();

  // Called when EmbeddedWorkerInstance is ready for IPC. This function
  // prepares a route to the child worker thread.
  // TODO(shimazu): Remove this function once mojofication is completed.
  void BindWorkerToProcess(int process_id, int embedded_worker_id);

  // RemoveWorker is called when EmbeddedWorkerInstance is destructed.
  // |process_id| could be invalid (i.e. ChildProcessHost::kInvalidUniqueID)
  // if it's not running.
  void RemoveWorker(int process_id, int embedded_worker_id);

  // Removes the bookkeeping that binds the worker to the process.  This is
  // called instead of WorkerStopped() in cases when the worker could not be
  // cleanly stopped, e.g., because connection with the renderer was lost.
  void DetachWorker(int process_id, int embedded_worker_id);

  base::WeakPtr<ServiceWorkerContextCore> context_;

  WorkerInstanceMap worker_map_;

  // Map from process_id to embedded_worker_id.
  // This map only contains starting and running workers.
  std::map<int, std::set<int> > worker_process_map_;

  int next_embedded_worker_id_;
  const int initial_embedded_worker_id_;
  ServiceWorkerLifetimeTracker lifetime_tracker_;

  DISALLOW_COPY_AND_ASSIGN(EmbeddedWorkerRegistry);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_EMBEDDED_WORKER_REGISTRY_H_
