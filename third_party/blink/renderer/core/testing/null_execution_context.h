// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_NULL_EXECUTION_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_NULL_EXECUTION_CONTEXT_H_

#include <memory>
#include "base/single_thread_task_runner.h"
#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/core/dom/events/event_queue.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/execution_context/security_context.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

class NullExecutionContext
    : public GarbageCollectedFinalized<NullExecutionContext>,
      public SecurityContext,
      public ExecutionContext {
  USING_GARBAGE_COLLECTED_MIXIN(NullExecutionContext);

 public:
  NullExecutionContext();

  void SetURL(const KURL& url) { url_ = url; }

  const KURL& Url() const override { return url_; }
  const KURL& BaseURL() const override { return url_; }
  KURL CompleteURL(const String&) const override { return url_; }

  void DisableEval(const String&) override {}
  String UserAgent() const override { return String(); }

  EventTarget* ErrorEventTarget() override { return nullptr; }
  EventQueue* GetEventQueue() const override { return queue_.Get(); }

  bool TasksNeedPause() override { return tasks_need_pause_; }
  void SetTasksNeedPause(bool flag) { tasks_need_pause_ = flag; }

  void DidUpdateSecurityOrigin() override {}
  SecurityContext& GetSecurityContext() override { return *this; }
  DOMTimerCoordinator* Timers() override { return nullptr; }

  void AddConsoleMessage(ConsoleMessage*) override {}
  void ExceptionThrown(ErrorEvent*) override {}

  void SetIsSecureContext(bool);
  bool IsSecureContext(String& error_message) const override;

  void SetUpSecurityContext();

  ResourceFetcher* Fetcher() const override { return nullptr; }

  FrameOrWorkerScheduler* GetScheduler() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner(TaskType) override;

  using SecurityContext::GetSecurityOrigin;
  using SecurityContext::GetContentSecurityPolicy;

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(queue_);
    SecurityContext::Trace(visitor);
    ExecutionContext::Trace(visitor);
  }

 private:
  bool tasks_need_pause_;
  bool is_secure_context_;
  Member<EventQueue> queue_;

  KURL url_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_NULL_EXECUTION_CONTEXT_H_
