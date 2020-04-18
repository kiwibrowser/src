/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/memory/discardable_memory_allocator.h"
#include "base/run_loop.h"
#include "base/test/icu_test_util.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/blink/public/platform/interface_provider.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#include "third_party/blink/renderer/platform/font_family_names.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/instrumentation/resource_coordinator/blink_resource_coordinator_base.h"
#include "third_party/blink/renderer/platform/instrumentation/resource_coordinator/renderer_resource_coordinator.h"
#include "third_party/blink/renderer/platform/language.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_type_names.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/network/mime/mock_mime_registry.h"
#include "third_party/blink/renderer/platform/scheduler/base/real_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_manager.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/wtf.h"

namespace blink {

class TestingPlatformSupport::TestingInterfaceProvider
    : public blink::InterfaceProvider {
 public:
  TestingInterfaceProvider() = default;
  virtual ~TestingInterfaceProvider() = default;

  void GetInterface(const char* name,
                    mojo::ScopedMessagePipeHandle handle) override {
    if (std::string(name) == mojom::blink::MimeRegistry::Name_) {
      mojo::MakeStrongBinding(
          std::make_unique<MockMimeRegistry>(),
          mojom::blink::MimeRegistryRequest(std::move(handle)));
      return;
    }
  }
};

namespace {

class DummyThread final : public blink::WebThread {
 public:
  bool IsCurrentThread() const override { return true; }
  blink::ThreadScheduler* Scheduler() const override { return nullptr; }
};

}  // namespace

TestingPlatformSupport::TestingPlatformSupport()
    : old_platform_(Platform::Current()),
      interface_provider_(new TestingInterfaceProvider) {
  DCHECK(old_platform_);
}

TestingPlatformSupport::~TestingPlatformSupport() {
  DCHECK_EQ(this, Platform::Current());
}

WebString TestingPlatformSupport::DefaultLocale() {
  return WebString::FromUTF8("en-US");
}

WebThread* TestingPlatformSupport::CurrentThread() {
  return old_platform_ ? old_platform_->CurrentThread() : nullptr;
}

WebBlobRegistry* TestingPlatformSupport::GetBlobRegistry() {
  return old_platform_ ? old_platform_->GetBlobRegistry() : nullptr;
}

WebIDBFactory* TestingPlatformSupport::IdbFactory() {
  return old_platform_ ? old_platform_->IdbFactory() : nullptr;
}

WebURLLoaderMockFactory* TestingPlatformSupport::GetURLLoaderMockFactory() {
  return old_platform_ ? old_platform_->GetURLLoaderMockFactory() : nullptr;
}

std::unique_ptr<WebURLLoaderFactory>
TestingPlatformSupport::CreateDefaultURLLoaderFactory() {
  return old_platform_ ? old_platform_->CreateDefaultURLLoaderFactory()
                       : nullptr;
}

WebData TestingPlatformSupport::GetDataResource(const char* name) {
  return old_platform_ ? old_platform_->GetDataResource(name) : WebData();
}

InterfaceProvider* TestingPlatformSupport::GetInterfaceProvider() {
  return interface_provider_.get();
}

void TestingPlatformSupport::RunUntilIdle() {
  base::RunLoop().RunUntilIdle();
}

class ScopedUnittestsEnvironmentSetup::DummyPlatform final
    : public blink::Platform {
 public:
  DummyPlatform() = default;

  blink::WebThread* CurrentThread() override {
    static DummyThread dummy_thread;
    return &dummy_thread;
  };
};

class ScopedUnittestsEnvironmentSetup::DummyRendererResourceCoordinator final
    : public blink::RendererResourceCoordinator {};

ScopedUnittestsEnvironmentSetup::ScopedUnittestsEnvironmentSetup(int argc,
                                                                 char** argv) {
  base::CommandLine::Init(argc, argv);

  base::test::InitializeICUForTesting();

  discardable_memory_allocator_ =
      std::make_unique<base::TestDiscardableMemoryAllocator>();
  base::DiscardableMemoryAllocator::SetInstance(
      discardable_memory_allocator_.get());

  dummy_platform_ = std::make_unique<DummyPlatform>();
  Platform::SetCurrentPlatformForTesting(dummy_platform_.get());

  WTF::Partitions::Initialize(nullptr);
  WTF::Initialize(nullptr);

  testing_platform_support_ = std::make_unique<TestingPlatformSupport>();
  Platform::SetCurrentPlatformForTesting(testing_platform_support_.get());

  if (BlinkResourceCoordinatorBase::IsEnabled()) {
    dummy_renderer_resource_coordinator_ =
        std::make_unique<DummyRendererResourceCoordinator>();
    RendererResourceCoordinator::
        SetCurrentRendererResourceCoordinatorForTesting(
            dummy_renderer_resource_coordinator_.get());
  }

  ProcessHeap::Init();
  ThreadState::AttachMainThread();
  ThreadState::Current()->RegisterTraceDOMWrappers(nullptr, nullptr, nullptr,
                                                   nullptr);
  HTTPNames::init();
  FetchInitiatorTypeNames::init();

  InitializePlatformLanguage();
  FontFamilyNames::init();
  WebRuntimeFeatures::EnableExperimentalFeatures(true);
  WebRuntimeFeatures::EnableTestOnlyFeatures(true);
}

ScopedUnittestsEnvironmentSetup::~ScopedUnittestsEnvironmentSetup() = default;

}  // namespace blink
