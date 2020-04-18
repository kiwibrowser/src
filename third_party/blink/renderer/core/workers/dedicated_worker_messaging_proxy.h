// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_DEDICATED_WORKER_MESSAGING_PROXY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_DEDICATED_WORKER_MESSAGING_PROXY_H_

#include <memory>
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/messaging/message_port.h"
#include "third_party/blink/renderer/core/workers/global_scope_creation_params.h"
#include "third_party/blink/renderer/core/workers/threaded_messaging_proxy_base.h"
#include "third_party/blink/renderer/core/workers/worker_backing_thread_startup_data.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/weborigin/referrer_policy.h"

namespace v8_inspector {
struct V8StackTraceId;
}

namespace blink {

class DedicatedWorker;
class DedicatedWorkerObjectProxy;
class SerializedScriptValue;
class WorkerOptions;

// A proxy class to talk to the DedicatedWorkerGlobalScope on a worker thread
// via the DedicatedWorkerMessagingProxy from the main thread. See class
// comments on ThreadedMessagingProxyBase for the lifetime and thread affinity.
class CORE_EXPORT DedicatedWorkerMessagingProxy
    : public ThreadedMessagingProxyBase {
 public:
  DedicatedWorkerMessagingProxy(ExecutionContext*, DedicatedWorker*);
  ~DedicatedWorkerMessagingProxy() override;

  // These methods should only be used on the parent context thread.
  void StartWorkerGlobalScope(std::unique_ptr<GlobalScopeCreationParams>,
                              const WorkerOptions&,
                              const KURL& script_url,
                              const v8_inspector::V8StackTraceId&,
                              const String& source_code);
  void PostMessageToWorkerGlobalScope(scoped_refptr<SerializedScriptValue>,
                                      Vector<MessagePortChannel>,
                                      const v8_inspector::V8StackTraceId&);

  bool HasPendingActivity() const;

  // These methods come from worker context thread via
  // DedicatedWorkerObjectProxy and are called on the parent context thread.
  void DidEvaluateScript(bool success);
  void PostMessageToWorkerObject(scoped_refptr<SerializedScriptValue>,
                                 Vector<MessagePortChannel>,
                                 const v8_inspector::V8StackTraceId&);
  void DispatchErrorEvent(const String& error_message,
                          std::unique_ptr<SourceLocation>,
                          int exception_id);

  DedicatedWorkerObjectProxy& WorkerObjectProxy() {
    return *worker_object_proxy_.get();
  }

  void Trace(blink::Visitor*) override;

 private:
  friend class DedicatedWorkerMessagingProxyForTest;

  base::Optional<WorkerBackingThreadStartupData> CreateBackingThreadStartupData(
      v8::Isolate*);

  std::unique_ptr<WorkerThread> CreateWorkerThread() override;

  std::unique_ptr<DedicatedWorkerObjectProxy> worker_object_proxy_;

  // This must be weak. The base class (i.e., ThreadedMessagingProxyBase) has a
  // strong persistent reference to itself via SelfKeepAlive (see class-level
  // comments on ThreadedMessagingProxyBase.h for details). To cut the
  // persistent reference, this worker object needs to call a cleanup function
  // in its dtor. If this is a strong reference, the dtor is never called
  // because the worker object is reachable from the persistent reference.
  WeakMember<DedicatedWorker> worker_object_;

  // Set once the initial script evaluation has been completed and it's ready
  // to dispatch events (e.g., Message events) on the worker global scope.
  bool was_script_evaluated_ = false;

  // Tasks are queued here until worker scripts are evaluated on the worker
  // global scope.
  struct QueuedTask;
  Vector<QueuedTask> queued_early_tasks_;
  DISALLOW_COPY_AND_ASSIGN(DedicatedWorkerMessagingProxy);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_DEDICATED_WORKER_MESSAGING_PROXY_H_
