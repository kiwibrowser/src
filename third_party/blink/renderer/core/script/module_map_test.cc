// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/script/module_map.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_fetch_request.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_loader_client.h"
#include "third_party/blink/renderer/core/script/modulator.h"
#include "third_party/blink/renderer/core/script/module_script.h"
#include "third_party/blink/renderer/core/script/script_module_resolver.h"
#include "third_party/blink/renderer/core/testing/dummy_modulator.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_mock_scheduler.h"

namespace blink {

namespace {

class TestSingleModuleClient final : public SingleModuleClient {
 public:
  TestSingleModuleClient() = default;
  ~TestSingleModuleClient() override {}

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(module_script_);
    SingleModuleClient::Trace(visitor);
  }

  void NotifyModuleLoadFinished(ModuleScript* module_script) override {
    was_notify_finished_ = true;
    module_script_ = module_script;
  }

  bool WasNotifyFinished() const { return was_notify_finished_; }
  ModuleScript* GetModuleScript() { return module_script_; }

 private:
  bool was_notify_finished_ = false;
  Member<ModuleScript> module_script_;
};

class TestScriptModuleResolver final : public ScriptModuleResolver {
 public:
  TestScriptModuleResolver() {}

  int RegisterModuleScriptCallCount() const {
    return register_module_script_call_count_;
  }

  void RegisterModuleScript(ModuleScript*) override {
    register_module_script_call_count_++;
  }

  void UnregisterModuleScript(ModuleScript*) override {
    FAIL() << "UnregisterModuleScript shouldn't be called in ModuleMapTest";
  }

  ModuleScript* GetHostDefined(const ScriptModule&) const override {
    NOTREACHED();
    return nullptr;
  }

  ScriptModule Resolve(const String& specifier,
                       const ScriptModule& referrer,
                       ExceptionState&) override {
    NOTREACHED();
    return ScriptModule();
  }

 private:
  int register_module_script_call_count_ = 0;
};

}  // namespace

class ModuleMapTestModulator final : public DummyModulator {
 public:
  ModuleMapTestModulator();
  ~ModuleMapTestModulator() override {}

  void Trace(blink::Visitor*) override;

  TestScriptModuleResolver* GetTestScriptModuleResolver() {
    return resolver_.Get();
  }
  void ResolveFetches();

 private:
  // Implements Modulator:

  ScriptModuleResolver* GetScriptModuleResolver() override {
    return resolver_.Get();
  }

  base::SingleThreadTaskRunner* TaskRunner() override {
    return Platform::Current()->CurrentThread()->GetTaskRunner().get();
  };

  void FetchNewSingleModule(const ModuleScriptFetchRequest&,
                            ModuleGraphLevel,
                            ModuleScriptLoaderClient*) override;

  struct TestRequest final : public GarbageCollectedFinalized<TestRequest> {
    TestRequest(const KURL& in_url,
                const ScriptFetchOptions& in_options,
                ModuleScriptLoaderClient* in_client)
        : url(in_url), options(in_options), client(in_client) {}
    KURL url;
    ScriptFetchOptions options;
    Member<ModuleScriptLoaderClient> client;

    void Trace(blink::Visitor* visitor) { visitor->Trace(client); }
  };
  HeapVector<Member<TestRequest>> test_requests_;

  Member<TestScriptModuleResolver> resolver_;
};

ModuleMapTestModulator::ModuleMapTestModulator()
    : resolver_(new TestScriptModuleResolver) {}

void ModuleMapTestModulator::Trace(blink::Visitor* visitor) {
  visitor->Trace(test_requests_);
  visitor->Trace(resolver_);
  DummyModulator::Trace(visitor);
}

void ModuleMapTestModulator::FetchNewSingleModule(
    const ModuleScriptFetchRequest& request,
    ModuleGraphLevel,
    ModuleScriptLoaderClient* client) {
  TestRequest* test_request =
      new TestRequest(request.Url(), request.Options(), client);
  test_requests_.push_back(test_request);
}

void ModuleMapTestModulator::ResolveFetches() {
  for (const auto& test_request : test_requests_) {
    auto* module_script = ModuleScript::CreateForTest(
        this, ScriptModule(), test_request->url, test_request->options);
    TaskRunner()->PostTask(
        FROM_HERE,
        WTF::Bind(&ModuleScriptLoaderClient::NotifyNewSingleModuleFinished,
                  WrapPersistent(test_request->client.Get()),
                  WrapPersistent(module_script)));
  }
  test_requests_.clear();
}

class ModuleMapTest : public testing::Test {
 public:
  void SetUp() override;

  ModuleMapTestModulator* Modulator() { return modulator_.Get(); }
  ModuleMap* Map() { return map_; }

 protected:
  Persistent<ModuleMapTestModulator> modulator_;
  Persistent<ModuleMap> map_;
};

void ModuleMapTest::SetUp() {
  modulator_ = new ModuleMapTestModulator();
  map_ = ModuleMap::Create(modulator_.Get());
}

TEST_F(ModuleMapTest, sequentialRequests) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;
  platform->AdvanceClockSeconds(1.);  // For non-zero DocumentParserTimings

  KURL url(NullURL(), "https://example.com/foo.js");

  // First request
  TestSingleModuleClient* client = new TestSingleModuleClient;
  Map()->FetchSingleModuleScript(ModuleScriptFetchRequest::CreateForTest(url),
                                 ModuleGraphLevel::kTopLevelModuleFetch,
                                 client);
  Modulator()->ResolveFetches();
  EXPECT_FALSE(client->WasNotifyFinished())
      << "fetchSingleModuleScript shouldn't complete synchronously";
  platform->RunUntilIdle();

  EXPECT_EQ(Modulator()
                ->GetTestScriptModuleResolver()
                ->RegisterModuleScriptCallCount(),
            1);
  EXPECT_TRUE(client->WasNotifyFinished());
  EXPECT_TRUE(client->GetModuleScript());

  // Secondary request
  TestSingleModuleClient* client2 = new TestSingleModuleClient;
  Map()->FetchSingleModuleScript(ModuleScriptFetchRequest::CreateForTest(url),
                                 ModuleGraphLevel::kTopLevelModuleFetch,
                                 client2);
  Modulator()->ResolveFetches();
  EXPECT_FALSE(client2->WasNotifyFinished())
      << "fetchSingleModuleScript shouldn't complete synchronously";
  platform->RunUntilIdle();

  EXPECT_EQ(Modulator()
                ->GetTestScriptModuleResolver()
                ->RegisterModuleScriptCallCount(),
            1)
      << "registerModuleScript sholudn't be called in secondary request.";
  EXPECT_TRUE(client2->WasNotifyFinished());
  EXPECT_TRUE(client2->GetModuleScript());
}

TEST_F(ModuleMapTest, concurrentRequestsShouldJoin) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;
  platform->AdvanceClockSeconds(1.);  // For non-zero DocumentParserTimings

  KURL url(NullURL(), "https://example.com/foo.js");

  // First request
  TestSingleModuleClient* client = new TestSingleModuleClient;
  Map()->FetchSingleModuleScript(ModuleScriptFetchRequest::CreateForTest(url),
                                 ModuleGraphLevel::kTopLevelModuleFetch,
                                 client);

  // Secondary request (which should join the first request)
  TestSingleModuleClient* client2 = new TestSingleModuleClient;
  Map()->FetchSingleModuleScript(ModuleScriptFetchRequest::CreateForTest(url),
                                 ModuleGraphLevel::kTopLevelModuleFetch,
                                 client2);

  Modulator()->ResolveFetches();
  EXPECT_FALSE(client->WasNotifyFinished())
      << "fetchSingleModuleScript shouldn't complete synchronously";
  EXPECT_FALSE(client2->WasNotifyFinished())
      << "fetchSingleModuleScript shouldn't complete synchronously";
  platform->RunUntilIdle();

  EXPECT_EQ(Modulator()
                ->GetTestScriptModuleResolver()
                ->RegisterModuleScriptCallCount(),
            1);

  EXPECT_TRUE(client->WasNotifyFinished());
  EXPECT_TRUE(client->GetModuleScript());
  EXPECT_TRUE(client2->WasNotifyFinished());
  EXPECT_TRUE(client2->GetModuleScript());
}

}  // namespace blink
