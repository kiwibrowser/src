// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/script/script_module_resolver_impl.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/script/modulator.h"
#include "third_party/blink/renderer/core/script/module_script.h"
#include "third_party/blink/renderer/core/testing/dummy_modulator.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_throw_exception.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_mock_scheduler.h"
#include "v8/include/v8.h"

namespace blink {

namespace {

class ScriptModuleResolverImplTestModulator final : public DummyModulator {
 public:
  ScriptModuleResolverImplTestModulator() {}
  ~ScriptModuleResolverImplTestModulator() override {}

  void Trace(blink::Visitor*) override;

  void SetScriptState(ScriptState* script_state) {
    script_state_ = script_state;
  }

  int GetFetchedModuleScriptCalled() const {
    return get_fetched_module_script_called_;
  }
  void SetModuleScript(ModuleScript* module_script) {
    module_script_ = module_script;
  }
  const KURL& FetchedUrl() const { return fetched_url_; }

 private:
  // Implements Modulator:
  ScriptState* GetScriptState() override { return script_state_.get(); }

  ModuleScript* GetFetchedModuleScript(const KURL&) override;

  scoped_refptr<ScriptState> script_state_;
  int get_fetched_module_script_called_ = 0;
  KURL fetched_url_;
  Member<ModuleScript> module_script_;
};

void ScriptModuleResolverImplTestModulator::Trace(blink::Visitor* visitor) {
  visitor->Trace(module_script_);
  DummyModulator::Trace(visitor);
}

ModuleScript* ScriptModuleResolverImplTestModulator::GetFetchedModuleScript(
    const KURL& url) {
  get_fetched_module_script_called_++;
  fetched_url_ = url;
  return module_script_.Get();
}

ModuleScript* CreateReferrerModuleScript(Modulator* modulator,
                                         V8TestingScope& scope) {
  KURL js_url("https://example.com/referrer.js");
  ScriptModule referrer_record = ScriptModule::Compile(
      scope.GetIsolate(), "import './target.js'; export const a = 42;", js_url,
      js_url, ScriptFetchOptions(), kSharableCrossOrigin,
      TextPosition::MinimumPosition(), ASSERT_NO_EXCEPTION);
  KURL referrer_url("https://example.com/referrer.js");
  auto* referrer_module_script =
      ModuleScript::CreateForTest(modulator, referrer_record, referrer_url);
  return referrer_module_script;
}

ModuleScript* CreateTargetModuleScript(Modulator* modulator,
                                       V8TestingScope& scope,
                                       bool has_parse_error = false) {
  KURL js_url("https://example.com/target.js");
  ScriptModule record = ScriptModule::Compile(
      scope.GetIsolate(), "export const pi = 3.14;", js_url, js_url,
      ScriptFetchOptions(), kSharableCrossOrigin,
      TextPosition::MinimumPosition(), ASSERT_NO_EXCEPTION);
  KURL url("https://example.com/target.js");
  auto* module_script = ModuleScript::CreateForTest(modulator, record, url);
  if (has_parse_error) {
    v8::Local<v8::Value> error =
        V8ThrowException::CreateError(scope.GetIsolate(), "hoge");
    module_script->SetParseErrorAndClearRecord(
        ScriptValue(scope.GetScriptState(), error));
  }
  return module_script;
}

}  // namespace

class ScriptModuleResolverImplTest : public testing::Test {
 public:
  void SetUp() override;

  ScriptModuleResolverImplTestModulator* Modulator() {
    return modulator_.Get();
  }

 protected:
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;
  Persistent<ScriptModuleResolverImplTestModulator> modulator_;
};

void ScriptModuleResolverImplTest::SetUp() {
  platform_->AdvanceClockSeconds(1.);  // For non-zero DocumentParserTimings
  modulator_ = new ScriptModuleResolverImplTestModulator();
}

TEST_F(ScriptModuleResolverImplTest, RegisterResolveSuccess) {
  V8TestingScope scope;
  ScriptModuleResolver* resolver = ScriptModuleResolverImpl::Create(
      Modulator(), scope.GetExecutionContext());
  Modulator()->SetScriptState(scope.GetScriptState());

  ModuleScript* referrer_module_script =
      CreateReferrerModuleScript(modulator_, scope);
  resolver->RegisterModuleScript(referrer_module_script);

  ModuleScript* target_module_script =
      CreateTargetModuleScript(modulator_, scope);
  Modulator()->SetModuleScript(target_module_script);

  ScriptModule resolved =
      resolver->Resolve("./target.js", referrer_module_script->Record(),
                        scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  EXPECT_EQ(resolved, target_module_script->Record());
  EXPECT_EQ(1, modulator_->GetFetchedModuleScriptCalled());
  EXPECT_EQ(modulator_->FetchedUrl(), target_module_script->BaseURL())
      << "Unexpectedly fetched URL: " << modulator_->FetchedUrl().GetString();
}

}  // namespace blink
