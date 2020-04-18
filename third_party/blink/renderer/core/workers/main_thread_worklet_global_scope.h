// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_MAIN_THREAD_WORKLET_GLOBAL_SCOPE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_MAIN_THREAD_WORKLET_GLOBAL_SCOPE_H_

#include "base/single_thread_task_runner.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/workers/worklet_global_scope.h"

namespace blink {

class ConsoleMessage;
class LocalFrame;
class WorkerReportingProxy;

class CORE_EXPORT MainThreadWorkletGlobalScope
    : public WorkletGlobalScope,
      public ContextClient {
  USING_GARBAGE_COLLECTED_MIXIN(MainThreadWorkletGlobalScope);

 public:
  MainThreadWorkletGlobalScope(LocalFrame*,
                               std::unique_ptr<GlobalScopeCreationParams>,
                               WorkerReportingProxy&);
  ~MainThreadWorkletGlobalScope() override;

  bool IsMainThreadWorkletGlobalScope() const final { return true; }

  // WorkerOrWorkletGlobalScope
  WorkerThread* GetThread() const final;
  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner(TaskType) override;

  void Terminate();

  // ExecutionContext
  void AddConsoleMessage(ConsoleMessage*) final;
  void ExceptionThrown(ErrorEvent*) final;
  CoreProbeSink* GetProbeSink() final;

  void Trace(blink::Visitor*) override;
};

DEFINE_TYPE_CASTS(MainThreadWorkletGlobalScope,
                  ExecutionContext,
                  context,
                  context->IsMainThreadWorkletGlobalScope(),
                  context.IsMainThreadWorkletGlobalScope());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_MAIN_THREAD_WORKLET_GLOBAL_SCOPE_H_
