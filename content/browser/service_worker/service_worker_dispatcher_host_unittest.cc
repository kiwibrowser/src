// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_dispatcher_host.h"

#include <stdint.h>

#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "content/browser/service_worker/embedded_worker_instance.h"
#include "content/browser/service_worker/embedded_worker_registry.h"
#include "content/browser/service_worker/embedded_worker_status.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_handle.h"
#include "content/browser/service_worker/service_worker_navigation_handle_core.h"
#include "content/browser/service_worker/service_worker_test_utils.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/service_worker/service_worker.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_provider_type.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"

namespace content {
namespace service_worker_dispatcher_host_unittest {

static void SaveStatusCallback(bool* called,
                               ServiceWorkerStatusCode* out,
                               ServiceWorkerStatusCode status) {
  *called = true;
  *out = status;
}

struct RemoteProviderInfo {
  mojom::ServiceWorkerContainerHostAssociatedPtr host_ptr;
  mojom::ServiceWorkerContainerAssociatedRequest client_request;
};

RemoteProviderInfo SetupProviderHostInfoPtrs(
    ServiceWorkerProviderHostInfo* host_info) {
  RemoteProviderInfo remote_info;
  mojom::ServiceWorkerContainerAssociatedPtr browser_side_client_ptr;
  remote_info.client_request =
      mojo::MakeRequestAssociatedWithDedicatedPipe(&browser_side_client_ptr);
  host_info->host_request =
      mojo::MakeRequestAssociatedWithDedicatedPipe(&remote_info.host_ptr);
  host_info->client_ptr_info = browser_side_client_ptr.PassInterface();
  EXPECT_TRUE(host_info->host_request.is_pending());
  EXPECT_TRUE(host_info->client_ptr_info.is_valid());
  EXPECT_TRUE(remote_info.host_ptr.is_bound());
  EXPECT_TRUE(remote_info.client_request.is_pending());
  return remote_info;
}

std::unique_ptr<ServiceWorkerNavigationHandleCore> CreateNavigationHandleCore(
    ServiceWorkerContextWrapper* context_wrapper) {
  std::unique_ptr<ServiceWorkerNavigationHandleCore> navigation_handle_core;
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::UI, FROM_HERE,
      base::Bind(
          [](ServiceWorkerContextWrapper* wrapper) {
            return std::make_unique<ServiceWorkerNavigationHandleCore>(nullptr,
                                                                       wrapper);
          },
          base::RetainedRef(context_wrapper)),
      base::Bind(
          [](std::unique_ptr<ServiceWorkerNavigationHandleCore>* dest,
             std::unique_ptr<ServiceWorkerNavigationHandleCore> src) {
            *dest = std::move(src);
          },
          &navigation_handle_core));
  base::RunLoop().RunUntilIdle();
  return navigation_handle_core;
}

class TestingServiceWorkerDispatcherHost : public ServiceWorkerDispatcherHost {
 public:
  TestingServiceWorkerDispatcherHost(int process_id,
                                     EmbeddedWorkerTestHelper* helper)
      : ServiceWorkerDispatcherHost(process_id),
        bad_messages_received_count_(0),
        helper_(helper) {}

  bool Send(IPC::Message* message) override { return helper_->Send(message); }

  IPC::TestSink* ipc_sink() { return helper_->ipc_sink(); }

  void ShutdownForBadMessage() override { ++bad_messages_received_count_; }

  int bad_messages_received_count_;

 protected:
  EmbeddedWorkerTestHelper* helper_;
  ~TestingServiceWorkerDispatcherHost() override {}
};

class ServiceWorkerDispatcherHostTest : public testing::Test {
 protected:
  ServiceWorkerDispatcherHostTest()
      : browser_thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    Initialize(std::make_unique<EmbeddedWorkerTestHelper>(base::FilePath()));
  }

  void TearDown() override {
    version_ = nullptr;
    registration_ = nullptr;
    helper_.reset();
  }

  ServiceWorkerContextCore* context() { return helper_->context(); }
  ServiceWorkerContextWrapper* context_wrapper() {
    return helper_->context_wrapper();
  }

  void Initialize(std::unique_ptr<EmbeddedWorkerTestHelper> helper) {
    helper_.reset(helper.release());
    // Replace the default dispatcher host.
    int process_id = helper_->mock_render_process_id();
    dispatcher_host_ =
        new TestingServiceWorkerDispatcherHost(process_id, helper_.get());
    helper_->RegisterDispatcherHost(process_id, nullptr);
    dispatcher_host_->Init(context_wrapper());
  }

  void SetUpRegistration(const GURL& scope, const GURL& script_url) {
    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = scope;
    registration_ =
        new ServiceWorkerRegistration(options, 1L, context()->AsWeakPtr());
    version_ = new ServiceWorkerVersion(registration_.get(), script_url, 1L,
                                        context()->AsWeakPtr());
    std::vector<ServiceWorkerDatabase::ResourceRecord> records;
    records.push_back(
        ServiceWorkerDatabase::ResourceRecord(10, version_->script_url(), 100));
    version_->script_cache_map()->SetResources(records);
    version_->SetMainScriptHttpResponseInfo(
        EmbeddedWorkerTestHelper::CreateHttpResponseInfo());
    version_->set_fetch_handler_existence(
        ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
    version_->SetStatus(ServiceWorkerVersion::INSTALLING);

    // Make the registration findable via storage functions.
    context()->storage()->LazyInitializeForTest(base::DoNothing());
    base::RunLoop().RunUntilIdle();
    bool called = false;
    ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_MAX_VALUE;
    context()->storage()->StoreRegistration(
        registration_.get(), version_.get(),
        base::BindOnce(&SaveStatusCallback, &called, &status));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(called);
    EXPECT_EQ(SERVICE_WORKER_OK, status);
  }

  void SendProviderCreated(blink::mojom::ServiceWorkerProviderType type,
                           const GURL& pattern) {
    const int64_t kProviderId = 99;
    ServiceWorkerProviderHostInfo info(kProviderId, MSG_ROUTING_NONE, type,
                                       true /* is_parent_frame_secure */);
    remote_endpoint_.BindWithProviderHostInfo(&info);

    dispatcher_host_->OnProviderCreated(std::move(info));
    provider_host_ = context()->GetProviderHost(
        helper_->mock_render_process_id(), kProviderId);
  }

  TestBrowserThreadBundle browser_thread_bundle_;
  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
  scoped_refptr<TestingServiceWorkerDispatcherHost> dispatcher_host_;
  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;
  ServiceWorkerProviderHost* provider_host_;
  ServiceWorkerRemoteProviderEndpoint remote_endpoint_;
};

TEST_F(ServiceWorkerDispatcherHostTest, ProviderCreatedAndDestroyed) {
  int process_id = helper_->mock_render_process_id();

  std::unique_ptr<ServiceWorkerNavigationHandleCore> navigation_handle_core;

  // Prepare the first navigation handle to create provider host.
  const int kProviderId1 = -2;
  navigation_handle_core =
      CreateNavigationHandleCore(helper_->context_wrapper());
  ASSERT_TRUE(navigation_handle_core);
  base::WeakPtr<ServiceWorkerProviderHost> host1 =
      ServiceWorkerProviderHost::PreCreateNavigationHost(
          context()->AsWeakPtr(), true /* are_ancestors_secure */,
          base::RepeatingCallback<WebContents*(void)>());
  EXPECT_EQ(kProviderId1, host1->provider_id());
  ServiceWorkerProviderHostInfo host_info_1(
      host1->provider_id(), 1 /* route_id */, host1->provider_type(),
      host1->is_parent_frame_secure());
  ServiceWorkerProviderHostInfo host_info_1_copy(
      host1->provider_id(), 1 /* route_id */, host1->provider_type(),
      host1->is_parent_frame_secure());
  RemoteProviderInfo remote_info_1 = SetupProviderHostInfoPtrs(&host_info_1);
  navigation_handle_core->DidPreCreateProviderHost(host1->provider_id());

  dispatcher_host_->OnProviderCreated(std::move(host_info_1));
  EXPECT_TRUE(context()->GetProviderHost(process_id, kProviderId1));

  // Two with the same ID should be seen as a bad message.
  dispatcher_host_->OnProviderCreated(std::move(host_info_1_copy));
  EXPECT_EQ(1, dispatcher_host_->bad_messages_received_count_);

  // Releasing the interface pointer destroys the counterpart.
  remote_info_1.host_ptr.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(context()->GetProviderHost(process_id, kProviderId1));

  // Prepare another navigation handle to create another provider host.
  const int kProviderId2 = -3;
  navigation_handle_core =
      CreateNavigationHandleCore(helper_->context_wrapper());
  ASSERT_TRUE(navigation_handle_core);
  base::WeakPtr<ServiceWorkerProviderHost> host2 =
      ServiceWorkerProviderHost::PreCreateNavigationHost(
          context()->AsWeakPtr(), true /* are_ancestors_secure */,
          base::RepeatingCallback<WebContents*(void)>());
  EXPECT_EQ(kProviderId2, host2->provider_id());
  ServiceWorkerProviderHostInfo host_info_2(
      host2->provider_id(), 2 /* route_id */, host2->provider_type(),
      host2->is_parent_frame_secure());
  RemoteProviderInfo remote_info_2 = SetupProviderHostInfoPtrs(&host_info_2);
  navigation_handle_core->DidPreCreateProviderHost(host2->provider_id());

  // Deletion of the dispatcher_host should cause providers for that
  // process to get deleted as well.
  dispatcher_host_->OnProviderCreated(std::move(host_info_2));
  EXPECT_TRUE(context()->GetProviderHost(process_id, kProviderId2));
  EXPECT_TRUE(dispatcher_host_->HasOneRef());
  dispatcher_host_ = nullptr;
  EXPECT_FALSE(context()->GetProviderHost(process_id, kProviderId2));
}

TEST_F(ServiceWorkerDispatcherHostTest, CleanupOnRendererCrash) {
  GURL pattern = GURL("https://www.example.com/");
  GURL script_url = GURL("https://www.example.com/service_worker.js");
  int process_id = helper_->mock_render_process_id();

  SendProviderCreated(blink::mojom::ServiceWorkerProviderType::kForWindow,
                      pattern);
  SetUpRegistration(pattern, script_url);
  int64_t provider_id = provider_host_->provider_id();

  // Start up the worker.
  bool called = false;
  ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_ABORT;
  version_->StartWorker(ServiceWorkerMetrics::EventType::UNKNOWN,
                        base::BindOnce(&SaveStatusCallback, &called, &status));
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(called);
  EXPECT_EQ(SERVICE_WORKER_OK, status);

  EXPECT_TRUE(context()->GetProviderHost(process_id, provider_id));
  EXPECT_EQ(EmbeddedWorkerStatus::RUNNING, version_->running_status());

  // Simulate the render process crashing.
  dispatcher_host_->OnFilterRemoved();

  // The dispatcher host should have removed the provider host.
  EXPECT_FALSE(context()->GetProviderHost(process_id, provider_id));

  // The EmbeddedWorkerInstance should still think it is running, since it will
  // clean itself up when its Mojo connection to the renderer breaks.
  EXPECT_EQ(EmbeddedWorkerStatus::RUNNING, version_->running_status());

  // We should be able to hook up a new dispatcher host although the old object
  // is not yet destroyed. This is what the browser does when reusing a crashed
  // render process.
  auto new_dispatcher_host =
      base::MakeRefCounted<TestingServiceWorkerDispatcherHost>(process_id,
                                                               helper_.get());
  new_dispatcher_host->Init(context_wrapper());

  // To show the new dispatcher can operate, simulate provider creation. Since
  // the old dispatcher cleaned up the old provider host, the new one won't
  // complain.
  ServiceWorkerProviderHostInfo host_info(
      provider_id, MSG_ROUTING_NONE,
      blink::mojom::ServiceWorkerProviderType::kForWindow,
      true /* is_parent_frame_secure */);
  ServiceWorkerRemoteProviderEndpoint remote_endpoint;
  remote_endpoint.BindWithProviderHostInfo(&host_info);
  new_dispatcher_host->OnProviderCreated(std::move(host_info));
  EXPECT_EQ(0, new_dispatcher_host->bad_messages_received_count_);
}

}  // namespace service_worker_dispatcher_host_unittest
}  // namespace content
