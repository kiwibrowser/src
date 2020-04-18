// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_INSPECTOR_PROXY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_INSPECTOR_PROXY_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/thread_debugger.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"

namespace blink {

class ExecutionContext;
class KURL;
class WorkerThread;

// A proxy for talking to the worker inspector on the worker thread.
// All of these methods should be called on the main thread.
class CORE_EXPORT WorkerInspectorProxy final
    : public GarbageCollectedFinalized<WorkerInspectorProxy> {
 public:
  enum class PauseOnWorkerStart { kPause, kDontPause };

  static WorkerInspectorProxy* Create();

  ~WorkerInspectorProxy();
  void Trace(blink::Visitor*);

  class CORE_EXPORT PageInspector {
   public:
    virtual ~PageInspector() = default;
    virtual void DispatchMessageFromWorker(WorkerInspectorProxy*,
                                           int session_id,
                                           const String& message) = 0;
  };

  // Returns whether WorkerThread should pause to run debugger tasks on its
  // startup.
  PauseOnWorkerStart ShouldPauseOnWorkerStart(ExecutionContext*);

  void WorkerThreadCreated(ExecutionContext*, WorkerThread*, const KURL&);
  void WorkerThreadTerminated();
  void DispatchMessageFromWorker(int session_id, const String&);

  void ConnectToInspector(int session_id,
                          PageInspector*);
  void DisconnectFromInspector(int session_id, PageInspector*);
  void SendMessageToInspector(int session_id, const String& message);

  const String& Url() { return url_; }
  ExecutionContext* GetExecutionContext() { return execution_context_; }
  const String& InspectorId();
  WorkerThread* GetWorkerThread() { return worker_thread_; }

 private:
  WorkerInspectorProxy();

  WorkerThread* worker_thread_;
  Member<ExecutionContext> execution_context_;
  HashMap<int, PageInspector*> page_inspectors_;
  String url_;
  String inspector_id_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_INSPECTOR_PROXY_H_
