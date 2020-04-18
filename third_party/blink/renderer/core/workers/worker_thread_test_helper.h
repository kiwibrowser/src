// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_THREAD_TEST_HELPER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_THREAD_TEST_HELPER_H_

#include <memory>

#include "base/macros.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/blink/public/mojom/net/ip_address_space.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_cache_options.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_gc_controller.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/script/script.h"
#include "third_party/blink/renderer/core/workers/global_scope_creation_params.h"
#include "third_party/blink/renderer/core/workers/parent_execution_context_task_runners.h"
#include "third_party/blink/renderer/core/workers/worker_backing_thread.h"
#include "third_party/blink/renderer/core/workers/worker_backing_thread_startup_data.h"
#include "third_party/blink/renderer/core/workers/worker_clients.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_reporting_proxy.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/core/workers/worker_thread_lifecycle_observer.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/network/content_security_policy_parsers.h"
#include "third_party/blink/renderer/platform/waitable_event.h"
#include "third_party/blink/renderer/platform/web_thread_supporting_gc.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/time.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "v8/include/v8.h"

namespace blink {

class MockWorkerThreadLifecycleObserver final
    : public GarbageCollectedFinalized<MockWorkerThreadLifecycleObserver>,
      public WorkerThreadLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(MockWorkerThreadLifecycleObserver);

 public:
  explicit MockWorkerThreadLifecycleObserver(
      WorkerThreadLifecycleContext* context)
      : WorkerThreadLifecycleObserver(context) {}

  MOCK_METHOD1(ContextDestroyed, void(WorkerThreadLifecycleContext*));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWorkerThreadLifecycleObserver);
};

class FakeWorkerGlobalScope : public WorkerGlobalScope {
 public:
  FakeWorkerGlobalScope(
      std::unique_ptr<GlobalScopeCreationParams> creation_params,
      WorkerThread* thread)
      : WorkerGlobalScope(std::move(creation_params),
                          thread,
                          CurrentTimeTicksInSeconds()) {}

  ~FakeWorkerGlobalScope() override = default;

  // EventTarget
  const AtomicString& InterfaceName() const override {
    return EventTargetNames::DedicatedWorkerGlobalScope;
  }

  // WorkerGlobalScope
  void ImportModuleScript(const KURL& module_url_record,
                          network::mojom::FetchCredentialsMode) override {
    NOTREACHED();
  }

  void ExceptionThrown(ErrorEvent*) override {}
};

class WorkerThreadForTest : public WorkerThread {
 public:
  WorkerThreadForTest(ThreadableLoadingContext* loading_context,
                      WorkerReportingProxy& mock_worker_reporting_proxy)
      : WorkerThread(loading_context, mock_worker_reporting_proxy),
        worker_backing_thread_(WorkerBackingThread::CreateForTest(
            WebThreadCreationParams(WebThreadType::kTestThread))) {}

  ~WorkerThreadForTest() override = default;

  WorkerBackingThread& GetWorkerBackingThread() override {
    return *worker_backing_thread_;
  }
  void ClearWorkerBackingThread() override { worker_backing_thread_.reset(); }

  void StartWithSourceCode(
      const SecurityOrigin* security_origin,
      const String& source,
      ParentExecutionContextTaskRunners* parent_execution_context_task_runners,
      const KURL& script_url = KURL("http://fake.url/"),
      WorkerClients* worker_clients = nullptr) {
    auto headers = std::make_unique<Vector<CSPHeaderAndType>>();
    CSPHeaderAndType header_and_type("contentSecurityPolicy",
                                     kContentSecurityPolicyHeaderTypeReport);
    headers->push_back(header_and_type);

    auto creation_params = std::make_unique<GlobalScopeCreationParams>(
        script_url, ScriptType::kClassic, "fake user agent", headers.get(),
        kReferrerPolicyDefault, security_origin,
        false /* starter_secure_context */, worker_clients,
        mojom::IPAddressSpace::kLocal, nullptr,
        base::UnguessableToken::Create(),
        std::make_unique<WorkerSettings>(Settings::Create().get()),
        kV8CacheOptionsDefault, nullptr /* worklet_module_responses_map */);

    Start(std::move(creation_params),
          WorkerBackingThreadStartupData::CreateDefault(),
          WorkerInspectorProxy::PauseOnWorkerStart::kDontPause,
          parent_execution_context_task_runners);
    EvaluateClassicScript(script_url, source, nullptr /* cached_meta_data */,
                          v8_inspector::V8StackTraceId());
  }

  void WaitForInit() {
    WaitableEvent completion_event;
    GetWorkerBackingThread().BackingThread().PostTask(
        FROM_HERE, CrossThreadBind(&WaitableEvent::Signal,
                                   CrossThreadUnretained(&completion_event)));
    completion_event.Wait();
  }

 protected:
  WorkerOrWorkletGlobalScope* CreateWorkerGlobalScope(
      std::unique_ptr<GlobalScopeCreationParams> creation_params) override {
    return new FakeWorkerGlobalScope(std::move(creation_params), this);
  }

 private:
  WebThreadType GetThreadType() const override {
    return WebThreadType::kUnspecifiedWorkerThread;
  }

  std::unique_ptr<WorkerBackingThread> worker_backing_thread_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_THREAD_TEST_HELPER_H_
