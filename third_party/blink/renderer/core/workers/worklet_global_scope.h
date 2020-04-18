// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKLET_GLOBAL_SCOPE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKLET_GLOBAL_SCOPE_H_

#include <memory>
#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/workers/worker_or_worklet_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_or_worklet_module_fetch_coordinator_proxy.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class WorkletPendingTasks;
class WorkerReportingProxy;
struct GlobalScopeCreationParams;

class CORE_EXPORT WorkletGlobalScope
    : public WorkerOrWorkletGlobalScope,
      public ActiveScriptWrappable<WorkletGlobalScope> {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(WorkletGlobalScope);

 public:
  ~WorkletGlobalScope() override;

  bool IsWorkletGlobalScope() const final { return true; }

  // Always returns false here as PaintWorkletGlobalScope and
  // AnimationWorkletGlobalScope don't have a #close() method on the global.
  // Note that AudioWorkletGlobal overrides this behavior.
  bool IsClosing() const override { return false; }

  ExecutionContext* GetExecutionContext() const override;

  // ExecutionContext
  const KURL& Url() const final { return url_; }
  const KURL& BaseURL() const final { return url_; }
  KURL CompleteURL(const String&) const final;
  String UserAgent() const final { return user_agent_; }
  SecurityContext& GetSecurityContext() final { return *this; }
  bool IsSecureContext(String& error_message) const final;

  DOMTimerCoordinator* Timers() final {
    // WorkletGlobalScopes don't have timers.
    NOTREACHED();
    return nullptr;
  }

  // Implementation of the "fetch and invoke a worklet script" algorithm:
  // https://drafts.css-houdini.org/worklets/#fetch-and-invoke-a-worklet-script
  // When script evaluation is done or any exception happens, it's notified to
  // the given WorkletPendingTasks via |outside_settings_task_runner| (i.e., the
  // parent frame's task runner).
  void FetchAndInvokeScript(
      const KURL& module_url_record,
      network::mojom::FetchCredentialsMode,
      scoped_refptr<base::SingleThreadTaskRunner> outside_settings_task_runner,
      WorkletPendingTasks*);

  WorkerOrWorkletModuleFetchCoordinatorProxy* ModuleFetchCoordinatorProxy()
      const;

  const SecurityOrigin* DocumentSecurityOrigin() const {
    return document_security_origin_.get();
  }

  // Customize the security context used for origin trials.
  // Origin trials are only enabled in secure contexts, but WorkletGlobalScopes
  // are defined to have a unique, opaque origin, so are not secure:
  // https://drafts.css-houdini.org/worklets/#script-settings-for-worklets
  // For origin trials, instead consider the context of the document which
  // created the worklet, since the origin trial tokens are inherited from the
  // document.
  bool DocumentSecureContext() const { return document_secure_context_; }

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;

 protected:
  // Partial implementation of the "set up a worklet environment settings
  // object" algorithm:
  // https://drafts.css-houdini.org/worklets/#script-settings-for-worklets
  WorkletGlobalScope(
      std::unique_ptr<GlobalScopeCreationParams>,
      v8::Isolate*,
      WorkerReportingProxy&,
      scoped_refptr<base::SingleThreadTaskRunner> document_loading_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> worklet_loading_task_runner);

 private:
  EventTarget* ErrorEventTarget() final { return nullptr; }

  // The |url_| and |user_agent_| are inherited from the parent Document.
  const KURL url_;
  const String user_agent_;

  // Used for module fetch and origin trials, inherited from the parent
  // Document.
  const scoped_refptr<const SecurityOrigin> document_security_origin_;

  // Used for origin trials, inherited from the parent Document.
  const bool document_secure_context_;

  Member<WorkerOrWorkletModuleFetchCoordinatorProxy> fetch_coordinator_proxy_;
};

DEFINE_TYPE_CASTS(WorkletGlobalScope,
                  ExecutionContext,
                  context,
                  context->IsWorkletGlobalScope(),
                  context.IsWorkletGlobalScope());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKLET_GLOBAL_SCOPE_H_
