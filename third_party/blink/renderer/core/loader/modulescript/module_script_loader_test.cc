// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/modulescript/module_script_loader.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/worker_or_worklet_script_controller.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/loader/modulescript/document_module_script_fetcher.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_fetch_request.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_loader_client.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_loader_registry.h"
#include "third_party/blink/renderer/core/loader/modulescript/worker_or_worklet_module_script_fetcher.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trial_context.h"
#include "third_party/blink/renderer/core/script/modulator.h"
#include "third_party/blink/renderer/core/script/module_script.h"
#include "third_party/blink/renderer/core/script/script.h"
#include "third_party/blink/renderer/core/testing/dummy_modulator.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/core/workers/global_scope_creation_params.h"
#include "third_party/blink/renderer/core/workers/main_thread_worklet_global_scope.h"
#include "third_party/blink/renderer/core/workers/main_thread_worklet_reporting_proxy.h"
#include "third_party/blink/renderer/core/workers/worklet_module_responses_map.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/testing/fetch_testing_platform_support.h"
#include "third_party/blink/renderer/platform/loader/testing/mock_fetch_context.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"

namespace blink {

namespace {

class TestModuleScriptLoaderClient final
    : public GarbageCollectedFinalized<TestModuleScriptLoaderClient>,
      public ModuleScriptLoaderClient {
  USING_GARBAGE_COLLECTED_MIXIN(TestModuleScriptLoaderClient);

 public:
  TestModuleScriptLoaderClient() = default;
  ~TestModuleScriptLoaderClient() override = default;

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(module_script_);
  }

  void NotifyNewSingleModuleFinished(ModuleScript* module_script) override {
    was_notify_finished_ = true;
    module_script_ = module_script;
  }

  bool WasNotifyFinished() const { return was_notify_finished_; }
  ModuleScript* GetModuleScript() { return module_script_; }

 private:
  bool was_notify_finished_ = false;
  Member<ModuleScript> module_script_;
};

class ModuleScriptLoaderTestModulator final : public DummyModulator {
 public:
  ModuleScriptLoaderTestModulator(
      scoped_refptr<ScriptState> script_state,
      scoped_refptr<const SecurityOrigin> security_origin,
      ResourceFetcher* fetcher)
      : script_state_(std::move(script_state)),
        security_origin_(std::move(security_origin)),
        fetcher_(fetcher) {}

  ~ModuleScriptLoaderTestModulator() override = default;

  const SecurityOrigin* GetSecurityOriginForFetch() override {
    return security_origin_.get();
  }

  ScriptState* GetScriptState() override { return script_state_.get(); }

  ScriptModule CompileModule(const String& script,
                             const KURL& source_url,
                             const KURL& base_url,
                             const ScriptFetchOptions& options,
                             AccessControlStatus access_control_status,
                             const TextPosition& position,
                             ExceptionState& exception_state) override {
    ScriptState::Scope scope(script_state_.get());
    return ScriptModule::Compile(
        script_state_->GetIsolate(), script, source_url, base_url, options,
        access_control_status, position, exception_state);
  }

  void SetModuleRequests(const Vector<String>& requests) {
    requests_.clear();
    for (const String& request : requests) {
      requests_.emplace_back(request, TextPosition::MinimumPosition());
    }
  }
  Vector<ModuleRequest> ModuleRequestsFromScriptModule(ScriptModule) override {
    return requests_;
  }

  ModuleScriptFetcher* CreateModuleScriptFetcher() override {
    auto* execution_context = ExecutionContext::From(script_state_.get());
    if (execution_context->IsDocument())
      return new DocumentModuleScriptFetcher(Fetcher());
    auto* global_scope = ToWorkletGlobalScope(execution_context);
    return new WorkerOrWorkletModuleScriptFetcher(
        global_scope->ModuleFetchCoordinatorProxy());
  }

  ResourceFetcher* Fetcher() const { return fetcher_.Get(); }

  void Trace(blink::Visitor*) override;

 private:
  scoped_refptr<ScriptState> script_state_;
  scoped_refptr<const SecurityOrigin> security_origin_;
  Member<ResourceFetcher> fetcher_;
  Vector<ModuleRequest> requests_;
};

void ModuleScriptLoaderTestModulator::Trace(blink::Visitor* visitor) {
  visitor->Trace(fetcher_);
  DummyModulator::Trace(visitor);
}

}  // namespace

class ModuleScriptLoaderTest : public PageTestBase {
  DISALLOW_COPY_AND_ASSIGN(ModuleScriptLoaderTest);

 public:
  ModuleScriptLoaderTest() = default;
  void SetUp() override;

  void InitializeForDocument();
  void InitializeForWorklet();

  void TestFetchDataURL(TestModuleScriptLoaderClient*);
  void TestInvalidSpecifier(TestModuleScriptLoaderClient*);
  void TestFetchInvalidURL(TestModuleScriptLoaderClient*);
  void TestFetchURL(TestModuleScriptLoaderClient*);

  ModuleScriptLoaderTestModulator* GetModulator() { return modulator_.Get(); }

 protected:
  ScopedTestingPlatformSupport<FetchTestingPlatformSupport> platform_;
  std::unique_ptr<MainThreadWorkletReportingProxy> reporting_proxy_;
  Persistent<ModuleScriptLoaderTestModulator> modulator_;
  Persistent<MainThreadWorkletGlobalScope> global_scope_;
};

void ModuleScriptLoaderTest::SetUp() {
  platform_->AdvanceClockSeconds(1.);  // For non-zero DocumentParserTimings
  PageTestBase::SetUp(IntSize(500, 500));
  GetDocument().SetURL(KURL("https://example.test"));
  GetDocument().SetSecurityOrigin(SecurityOrigin::Create(GetDocument().Url()));
}

void ModuleScriptLoaderTest::InitializeForDocument() {
  auto* fetch_context =
      MockFetchContext::Create(MockFetchContext::kShouldLoadNewResource);
  auto* fetcher = ResourceFetcher::Create(fetch_context);
  modulator_ = new ModuleScriptLoaderTestModulator(
      ToScriptStateForMainWorld(&GetFrame()), GetDocument().GetSecurityOrigin(),
      fetcher);
}

void ModuleScriptLoaderTest::InitializeForWorklet() {
  auto* fetch_context =
      MockFetchContext::Create(MockFetchContext::kShouldLoadNewResource);
  auto* fetcher = ResourceFetcher::Create(fetch_context);
  reporting_proxy_ =
      std::make_unique<MainThreadWorkletReportingProxy>(&GetDocument());
  auto creation_params = std::make_unique<GlobalScopeCreationParams>(
      GetDocument().Url(), ScriptType::kModule, GetDocument().UserAgent(),
      nullptr /* content_security_policy_parsed_headers */,
      GetDocument().GetReferrerPolicy(), GetDocument().GetSecurityOrigin(),
      GetDocument().IsSecureContext(), nullptr /* worker_clients */,
      GetDocument().AddressSpace(),
      OriginTrialContext::GetTokens(&GetDocument()).get(),
      base::UnguessableToken::Create(), nullptr /* worker_settings */,
      kV8CacheOptionsDefault, new WorkletModuleResponsesMap(fetcher));
  global_scope_ = new MainThreadWorkletGlobalScope(
      &GetFrame(), std::move(creation_params), *reporting_proxy_);
  global_scope_->ScriptController()->InitializeContextIfNeeded("Dummy Context");
  modulator_ = new ModuleScriptLoaderTestModulator(
      global_scope_->ScriptController()->GetScriptState(),
      GetDocument().GetSecurityOrigin(), fetcher);
}

void ModuleScriptLoaderTest::TestFetchDataURL(
    TestModuleScriptLoaderClient* client) {
  ModuleScriptLoaderRegistry* registry = ModuleScriptLoaderRegistry::Create();
  KURL url("data:text/javascript,export default 'grapes';");
  registry->Fetch(ModuleScriptFetchRequest::CreateForTest(url),
                  ModuleGraphLevel::kTopLevelModuleFetch, GetModulator(),
                  client);
}

TEST_F(ModuleScriptLoaderTest, FetchDataURL) {
  InitializeForDocument();
  TestModuleScriptLoaderClient* client = new TestModuleScriptLoaderClient;
  TestFetchDataURL(client);

  EXPECT_TRUE(client->WasNotifyFinished())
      << "ModuleScriptLoader should finish synchronously.";
  ASSERT_TRUE(client->GetModuleScript());
  EXPECT_FALSE(client->GetModuleScript()->HasEmptyRecord());
  EXPECT_FALSE(client->GetModuleScript()->HasParseError());
}

TEST_F(ModuleScriptLoaderTest, FetchDataURL_OnWorklet) {
  InitializeForWorklet();
  TestModuleScriptLoaderClient* client1 = new TestModuleScriptLoaderClient;
  TestFetchDataURL(client1);

  EXPECT_FALSE(client1->WasNotifyFinished())
      << "ModuleScriptLoader should finish asynchronously.";
  platform_->RunUntilIdle();

  EXPECT_TRUE(client1->WasNotifyFinished());
  ASSERT_TRUE(client1->GetModuleScript());
  EXPECT_FALSE(client1->GetModuleScript()->HasEmptyRecord());
  EXPECT_FALSE(client1->GetModuleScript()->HasParseError());

  // Try to fetch the same URL again in order to verify the case where
  // WorkletModuleResponsesMap serves a cache.
  TestModuleScriptLoaderClient* client2 = new TestModuleScriptLoaderClient;
  TestFetchDataURL(client2);

  EXPECT_FALSE(client2->WasNotifyFinished())
      << "ModuleScriptLoader should finish asynchronously.";
  platform_->RunUntilIdle();

  EXPECT_TRUE(client2->WasNotifyFinished());
  ASSERT_TRUE(client2->GetModuleScript());
  EXPECT_FALSE(client2->GetModuleScript()->HasEmptyRecord());
  EXPECT_FALSE(client2->GetModuleScript()->HasParseError());
}

void ModuleScriptLoaderTest::TestInvalidSpecifier(
    TestModuleScriptLoaderClient* client) {
  ModuleScriptLoaderRegistry* registry = ModuleScriptLoaderRegistry::Create();
  KURL url("data:text/javascript,import 'invalid';export default 'grapes';");
  GetModulator()->SetModuleRequests({"invalid"});
  registry->Fetch(ModuleScriptFetchRequest::CreateForTest(url),
                  ModuleGraphLevel::kTopLevelModuleFetch, GetModulator(),
                  client);
}

TEST_F(ModuleScriptLoaderTest, InvalidSpecifier) {
  InitializeForDocument();
  TestModuleScriptLoaderClient* client = new TestModuleScriptLoaderClient;
  TestInvalidSpecifier(client);

  EXPECT_TRUE(client->WasNotifyFinished())
      << "ModuleScriptLoader should finish synchronously.";
  ASSERT_TRUE(client->GetModuleScript());
  EXPECT_TRUE(client->GetModuleScript()->HasEmptyRecord());
  EXPECT_TRUE(client->GetModuleScript()->HasParseError());
}

TEST_F(ModuleScriptLoaderTest, InvalidSpecifier_OnWorklet) {
  InitializeForWorklet();
  TestModuleScriptLoaderClient* client = new TestModuleScriptLoaderClient;
  TestInvalidSpecifier(client);

  EXPECT_FALSE(client->WasNotifyFinished())
      << "ModuleScriptLoader should finish asynchronously.";
  platform_->RunUntilIdle();

  EXPECT_TRUE(client->WasNotifyFinished());
  ASSERT_TRUE(client->GetModuleScript());
  EXPECT_TRUE(client->GetModuleScript()->HasEmptyRecord());
  EXPECT_TRUE(client->GetModuleScript()->HasParseError());
}

void ModuleScriptLoaderTest::TestFetchInvalidURL(
    TestModuleScriptLoaderClient* client) {
  ModuleScriptLoaderRegistry* registry = ModuleScriptLoaderRegistry::Create();
  KURL url;
  EXPECT_FALSE(url.IsValid());
  registry->Fetch(ModuleScriptFetchRequest::CreateForTest(url),
                  ModuleGraphLevel::kTopLevelModuleFetch, GetModulator(),
                  client);
}

TEST_F(ModuleScriptLoaderTest, FetchInvalidURL) {
  InitializeForDocument();
  TestModuleScriptLoaderClient* client = new TestModuleScriptLoaderClient;
  TestFetchInvalidURL(client);

  EXPECT_TRUE(client->WasNotifyFinished())
      << "ModuleScriptLoader should finish synchronously.";
  EXPECT_FALSE(client->GetModuleScript());
}

TEST_F(ModuleScriptLoaderTest, FetchInvalidURL_OnWorklet) {
  InitializeForWorklet();
  TestModuleScriptLoaderClient* client = new TestModuleScriptLoaderClient;
  TestFetchInvalidURL(client);

  EXPECT_FALSE(client->WasNotifyFinished())
      << "ModuleScriptLoader should finish asynchronously.";
  platform_->RunUntilIdle();

  EXPECT_TRUE(client->WasNotifyFinished());
  EXPECT_FALSE(client->GetModuleScript());
}

void ModuleScriptLoaderTest::TestFetchURL(
    TestModuleScriptLoaderClient* client) {
  KURL url("https://example.test/module.js");
  URLTestHelpers::RegisterMockedURLLoad(
      url, test::CoreTestDataPath("module.js"), "text/javascript");

  ModuleScriptLoaderRegistry* registry = ModuleScriptLoaderRegistry::Create();
  registry->Fetch(ModuleScriptFetchRequest::CreateForTest(url),
                  ModuleGraphLevel::kTopLevelModuleFetch, GetModulator(),
                  client);
}

TEST_F(ModuleScriptLoaderTest, FetchURL) {
  InitializeForDocument();
  TestModuleScriptLoaderClient* client = new TestModuleScriptLoaderClient;
  TestFetchURL(client);

  EXPECT_FALSE(client->WasNotifyFinished())
      << "ModuleScriptLoader unexpectedly finished synchronously.";
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();

  EXPECT_TRUE(client->WasNotifyFinished());
  EXPECT_TRUE(client->GetModuleScript());
}

TEST_F(ModuleScriptLoaderTest, FetchURL_OnWorklet) {
  InitializeForWorklet();
  TestModuleScriptLoaderClient* client = new TestModuleScriptLoaderClient;
  TestFetchURL(client);

  EXPECT_FALSE(client->WasNotifyFinished())
      << "ModuleScriptLoader unexpectedly finished synchronously.";

  // Advance until WorkerOrWorkletModuleScriptFetcher finishes looking up a
  // cache in WorkletModuleResponsesMap and issues a fetch request so that
  // ServeAsynchronousRequests() can serve for the pending request.
  platform_->RunUntilIdle();
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();

  EXPECT_TRUE(client->WasNotifyFinished());
  EXPECT_TRUE(client->GetModuleScript());
}

}  // namespace blink
