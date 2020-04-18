/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_RUNNER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_RUNNER_H_

#include "base/location.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/deque.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"

namespace blink {

class Document;
class PendingScript;
class ScriptLoader;

class CORE_EXPORT ScriptRunner final
    : public GarbageCollectedFinalized<ScriptRunner>,
      public TraceWrapperBase {
 public:
  static ScriptRunner* Create(Document* document) {
    return new ScriptRunner(document);
  }

  void QueueScriptForExecution(PendingScript*);
  bool HasPendingScripts() const {
    return !pending_in_order_scripts_.IsEmpty() ||
           !pending_async_scripts_.IsEmpty();
  }
  void Suspend();
  void Resume();
  void NotifyScriptReady(PendingScript*);
  void NotifyScriptStreamerFinished();

  static void MovePendingScript(Document&, Document&, ScriptLoader*);

  void Trace(blink::Visitor*);
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override { return "ScriptRunner"; }

 private:
  class Task;

  explicit ScriptRunner(Document*);

  void MovePendingScript(ScriptRunner*, PendingScript*);
  bool RemovePendingInOrderScript(PendingScript*);
  void ScheduleReadyInOrderScripts();

  void PostTask(const base::Location&);

  // Execute the first task in in_order_scripts_to_execute_soon_.
  // Returns true if task was run, and false otherwise.
  bool ExecuteInOrderTask();

  // Execute any task in async_scripts_to_execute_soon_.
  // Returns true if task was run, and false otherwise.
  bool ExecuteAsyncTask();

  void ExecuteTask();

  // Try to start streaming a specific script or any available script.
  void TryStream(PendingScript*);
  void TryStreamAny();
  bool DoTryStream(PendingScript*);  // Implementation for both Try* methods.

  Member<Document> document_;

  HeapDeque<TraceWrapperMember<PendingScript>> pending_in_order_scripts_;
  HeapHashSet<TraceWrapperMember<PendingScript>> pending_async_scripts_;

  // http://www.whatwg.org/specs/web-apps/current-work/#set-of-scripts-that-will-execute-as-soon-as-possible
  HeapDeque<TraceWrapperMember<PendingScript>> async_scripts_to_execute_soon_;
  HeapDeque<TraceWrapperMember<PendingScript>>
      in_order_scripts_to_execute_soon_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  int number_of_in_order_scripts_with_pending_notification_ = 0;

  bool is_suspended_ = false;

#ifndef NDEBUG
  // We expect to have one posted task in flight for each script in either
  // .._to_be_executed_soon_ queue. This invariant will be temporarily violated
  // when the ScriptRunner is suspended, or when we take a Script out the
  // async_scripts_to_be_executed_soon_ queue for streaming. We'll use this
  // variable to account & check this invariant for debugging.
  int number_of_extra_tasks_ = 0;
#endif
  DISALLOW_COPY_AND_ASSIGN(ScriptRunner);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_RUNNER_H_
