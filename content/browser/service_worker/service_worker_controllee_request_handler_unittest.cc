// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_controllee_request_handler.h"

#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "components/offline_pages/buildflags/buildflags.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_provider_host.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/browser/service_worker/service_worker_response_type.h"
#include "content/browser/service_worker/service_worker_test_utils.h"
#include "content/browser/service_worker/service_worker_url_request_job.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/request_context_type.h"
#include "content/public/common/resource_type.h"
#include "content/public/test/mock_resource_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/test/test_content_browser_client.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/mojom/request_context_frame_type.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"

namespace content {
namespace service_worker_controllee_request_handler_unittest {

int kMockProviderId = 1;

class ServiceWorkerControlleeRequestHandlerTest : public testing::Test {
 public:
  class ServiceWorkerRequestTestResources {
   public:
    ServiceWorkerRequestTestResources(
        ServiceWorkerControlleeRequestHandlerTest* test,
        const GURL& url,
        ResourceType type,
        network::mojom::FetchRequestMode fetch_type =
            network::mojom::FetchRequestMode::kNoCORS)
        : test_(test),
          request_(test->url_request_context_.CreateRequest(
              url,
              net::DEFAULT_PRIORITY,
              &test->url_request_delegate_,
              TRAFFIC_ANNOTATION_FOR_TESTS)),
          handler_(new ServiceWorkerControlleeRequestHandler(
              test->context()->AsWeakPtr(),
              test->provider_host_,
              base::WeakPtr<storage::BlobStorageContext>(),
              fetch_type,
              network::mojom::FetchCredentialsMode::kOmit,
              network::mojom::FetchRedirectMode::kFollow,
              std::string() /* integrity */,
              false /* keepalive */,
              type,
              REQUEST_CONTEXT_TYPE_HYPERLINK,
              network::mojom::RequestContextFrameType::kTopLevel,
              scoped_refptr<network::ResourceRequestBody>())),
          job_(nullptr) {}

    ServiceWorkerURLRequestJob* MaybeCreateJob() {
      job_.reset(handler_->MaybeCreateJob(request_.get(), nullptr,
                                          &test_->mock_resource_context_));
      return static_cast<ServiceWorkerURLRequestJob*>(job_.get());
    }

    void ResetHandler() { handler_.reset(nullptr); }

    net::URLRequest* request() const { return request_.get(); }

   private:
    ServiceWorkerControlleeRequestHandlerTest* test_;
    std::unique_ptr<net::URLRequest> request_;
    std::unique_ptr<ServiceWorkerControlleeRequestHandler> handler_;
    std::unique_ptr<net::URLRequestJob> job_;
  };

  ServiceWorkerControlleeRequestHandlerTest()
      : browser_thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    SetUpWithHelper(new EmbeddedWorkerTestHelper(base::FilePath()));
  }

  void SetUpWithHelper(EmbeddedWorkerTestHelper* helper) {
    helper_.reset(helper);

    // A new unstored registration/version.
    scope_ = GURL("https://host/scope/");
    script_url_ = GURL("https://host/script.js");
    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = scope_;
    registration_ =
        new ServiceWorkerRegistration(options, 1L, context()->AsWeakPtr());
    version_ = new ServiceWorkerVersion(
        registration_.get(), script_url_, 1L, context()->AsWeakPtr());

    context()->storage()->LazyInitializeForTest(base::DoNothing());
    base::RunLoop().RunUntilIdle();

    std::vector<ServiceWorkerDatabase::ResourceRecord> records;
    records.push_back(WriteToDiskCacheSync(
        context()->storage(), version_->script_url(),
        context()->storage()->NewResourceId(), {} /* headers */, "I'm a body",
        "I'm a meta data"));
    version_->script_cache_map()->SetResources(records);
    version_->SetMainScriptHttpResponseInfo(
        EmbeddedWorkerTestHelper::CreateHttpResponseInfo());

    // An empty host.
    remote_endpoints_.emplace_back();
    std::unique_ptr<ServiceWorkerProviderHost> host =
        CreateProviderHostForWindow(
            helper_->mock_render_process_id(), kMockProviderId,
            true /* is_parent_frame_secure */, helper_->context()->AsWeakPtr(),
            &remote_endpoints_.back());
    provider_host_ = host->AsWeakPtr();
    context()->AddProviderHost(std::move(host));
  }

  void TearDown() override {
    version_ = nullptr;
    registration_ = nullptr;
    helper_.reset();
  }

  ServiceWorkerContextCore* context() const { return helper_->context(); }

 protected:
  TestBrowserThreadBundle browser_thread_bundle_;
  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;
  base::WeakPtr<ServiceWorkerProviderHost> provider_host_;
  net::URLRequestContext url_request_context_;
  net::TestDelegate url_request_delegate_;
  MockResourceContext mock_resource_context_;
  GURL scope_;
  GURL script_url_;
  std::vector<ServiceWorkerRemoteProviderEndpoint> remote_endpoints_;
};

class ServiceWorkerTestContentBrowserClient : public TestContentBrowserClient {
 public:
  ServiceWorkerTestContentBrowserClient() {}
  bool AllowServiceWorker(
      const GURL& scope,
      const GURL& first_party,
      content::ResourceContext* context,
      const base::Callback<WebContents*(void)>& wc_getter) override {
    return false;
  }
};

TEST_F(ServiceWorkerControlleeRequestHandlerTest, DisallowServiceWorker) {
  ServiceWorkerTestContentBrowserClient test_browser_client;
  ContentBrowserClient* old_browser_client =
      SetBrowserClientForTesting(&test_browser_client);

  // Store an activated worker.
  version_->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  version_->SetStatus(ServiceWorkerVersion::ACTIVATED);
  registration_->SetActiveVersion(version_);
  context()->storage()->StoreRegistration(registration_.get(), version_.get(),
                                          base::DoNothing());
  base::RunLoop().RunUntilIdle();

  // Conduct a main resource load.
  ServiceWorkerRequestTestResources test_resources(
      this, GURL("https://host/scope/doc"), RESOURCE_TYPE_MAIN_FRAME);
  ServiceWorkerURLRequestJob* sw_job = test_resources.MaybeCreateJob();

  EXPECT_FALSE(sw_job->ShouldFallbackToNetwork());
  EXPECT_FALSE(sw_job->ShouldForwardToServiceWorker());
  EXPECT_FALSE(version_->HasControllee());
  base::RunLoop().RunUntilIdle();

  // Verify we did not use the worker.
  EXPECT_TRUE(sw_job->ShouldFallbackToNetwork());
  EXPECT_FALSE(sw_job->ShouldForwardToServiceWorker());
  EXPECT_FALSE(version_->HasControllee());

  SetBrowserClientForTesting(old_browser_client);
}

TEST_F(ServiceWorkerControlleeRequestHandlerTest, ActivateWaitingVersion) {
  // Store a registration that is installed but not activated yet.
  version_->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  version_->SetStatus(ServiceWorkerVersion::INSTALLED);
  registration_->SetWaitingVersion(version_);
  context()->storage()->StoreRegistration(registration_.get(), version_.get(),
                                          base::DoNothing());
  base::RunLoop().RunUntilIdle();

  // Conduct a main resource load.
  ServiceWorkerRequestTestResources test_resources(
      this, GURL("https://host/scope/doc"), RESOURCE_TYPE_MAIN_FRAME);
  ServiceWorkerURLRequestJob* sw_job = test_resources.MaybeCreateJob();

  EXPECT_FALSE(sw_job->ShouldFallbackToNetwork());
  EXPECT_FALSE(sw_job->ShouldForwardToServiceWorker());
  EXPECT_FALSE(version_->HasControllee());

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ServiceWorkerVersion::ACTIVATED,
            version_->status());
  EXPECT_FALSE(sw_job->ShouldFallbackToNetwork());
  EXPECT_TRUE(sw_job->ShouldForwardToServiceWorker());
  EXPECT_TRUE(version_->HasControllee());

  // Navigations should trigger an update too.
  test_resources.ResetHandler();
  EXPECT_TRUE(version_->update_timer_.IsRunning());
}

// Test that an installing registration is associated with a provider host.
TEST_F(ServiceWorkerControlleeRequestHandlerTest, InstallingRegistration) {
  // Create an installing registration.
  version_->SetStatus(ServiceWorkerVersion::INSTALLING);
  version_->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  registration_->SetInstallingVersion(version_);
  context()->storage()->NotifyInstallingRegistration(registration_.get());

  // Conduct a main resource load.
  ServiceWorkerRequestTestResources test_resources(
      this, GURL("https://host/scope/doc"), RESOURCE_TYPE_MAIN_FRAME);
  ServiceWorkerURLRequestJob* job = test_resources.MaybeCreateJob();

  base::RunLoop().RunUntilIdle();

  // The handler should have fallen back to network and destroyed the job. The
  // registration should be associated with the provider host, although it is
  // not controlled since there is no active version.
  EXPECT_FALSE(job);
  EXPECT_EQ(registration_.get(), provider_host_->associated_registration());
  EXPECT_EQ(version_.get(), provider_host_->installing_version());
  EXPECT_FALSE(version_->HasControllee());
  EXPECT_FALSE(provider_host_->controller());
}

// Test to not regress crbug/414118.
TEST_F(ServiceWorkerControlleeRequestHandlerTest, DeletedProviderHost) {
  // Store a registration so the call to FindRegistrationForDocument will read
  // from the database.
  version_->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  version_->SetStatus(ServiceWorkerVersion::ACTIVATED);
  registration_->SetActiveVersion(version_);
  context()->storage()->StoreRegistration(registration_.get(), version_.get(),
                                          base::DoNothing());
  base::RunLoop().RunUntilIdle();
  version_ = nullptr;
  registration_ = nullptr;

  // Conduct a main resource load.
  ServiceWorkerRequestTestResources test_resources(
      this, GURL("https://host/scope/doc"), RESOURCE_TYPE_MAIN_FRAME);
  ServiceWorkerURLRequestJob* sw_job = test_resources.MaybeCreateJob();

  EXPECT_FALSE(sw_job->ShouldFallbackToNetwork());
  EXPECT_FALSE(sw_job->ShouldForwardToServiceWorker());

  // Shouldn't crash if the ProviderHost is deleted prior to completion of
  // the database lookup.
  context()->RemoveProviderHost(helper_->mock_render_process_id(),
                                kMockProviderId);
  EXPECT_FALSE(provider_host_.get());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(sw_job->ShouldFallbackToNetwork());
  EXPECT_FALSE(sw_job->ShouldForwardToServiceWorker());
}

// Tests the scenario where a controllee request handler was created
// for a subresource request, but before MaybeCreateJob() is run, the
// controller/active version becomes null.
TEST_F(ServiceWorkerControlleeRequestHandlerTest, LostActiveVersion) {
  // Store an activated worker.
  version_->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  version_->SetStatus(ServiceWorkerVersion::ACTIVATED);
  registration_->SetActiveVersion(version_);
  context()->storage()->StoreRegistration(registration_.get(), version_.get(),
                                          base::DoNothing());
  base::RunLoop().RunUntilIdle();

  // Conduct a main resource load to set the controller.
  ServiceWorkerRequestTestResources main_test_resources(
      this, GURL("https://host/scope/doc"), RESOURCE_TYPE_MAIN_FRAME);
  main_test_resources.MaybeCreateJob();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(version_->HasControllee());
  EXPECT_EQ(version_, provider_host_->controller());
  EXPECT_EQ(version_, provider_host_->active_version());

  // Unset the active version.
  provider_host_->NotifyControllerLost();
  registration_->SetActiveVersion(nullptr);
  EXPECT_FALSE(version_->HasControllee());
  EXPECT_FALSE(provider_host_->controller());
  EXPECT_FALSE(provider_host_->active_version());

  // Conduct a subresource load.
  ServiceWorkerRequestTestResources sub_test_resources(
      this, GURL("https://host/scope/doc"), RESOURCE_TYPE_IMAGE);
  ServiceWorkerURLRequestJob* sub_job = sub_test_resources.MaybeCreateJob();
  base::RunLoop().RunUntilIdle();

  // Verify that the job errored.
  EXPECT_EQ(ServiceWorkerResponseType::FAIL_DUE_TO_LOST_CONTROLLER,
            sub_job->response_type_);
}

TEST_F(ServiceWorkerControlleeRequestHandlerTest, FallbackWithNoFetchHandler) {
  version_->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::DOES_NOT_EXIST);
  version_->SetStatus(ServiceWorkerVersion::ACTIVATED);
  registration_->SetActiveVersion(version_);
  context()->storage()->StoreRegistration(registration_.get(), version_.get(),
                                          base::DoNothing());
  base::RunLoop().RunUntilIdle();

  ServiceWorkerRequestTestResources main_test_resources(
      this, GURL("https://host/scope/doc"), RESOURCE_TYPE_MAIN_FRAME);
  ServiceWorkerURLRequestJob* main_job = main_test_resources.MaybeCreateJob();

  EXPECT_FALSE(main_job->ShouldFallbackToNetwork());
  EXPECT_FALSE(main_job->ShouldForwardToServiceWorker());
  EXPECT_FALSE(version_->HasControllee());

  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(main_job->ShouldFallbackToNetwork());
  EXPECT_FALSE(main_job->ShouldForwardToServiceWorker());
  EXPECT_TRUE(version_->HasControllee());
  EXPECT_EQ(version_, provider_host_->controller());

  ServiceWorkerRequestTestResources sub_test_resources(
      this, GURL("https://host/scope/doc/subresource"), RESOURCE_TYPE_IMAGE);
  ServiceWorkerURLRequestJob* sub_job = sub_test_resources.MaybeCreateJob();

  // This job shouldn't be created because this worker doesn't have fetch
  // handler.
  EXPECT_EQ(nullptr, sub_job);

  // CORS request should be returned to renderer for CORS checking.
  ServiceWorkerRequestTestResources sub_test_resources_cors(
      this, GURL("https://host/scope/doc/subresource"), RESOURCE_TYPE_SCRIPT,
      network::mojom::FetchRequestMode::kCORS);
  ServiceWorkerURLRequestJob* sub_cors_job =
      sub_test_resources_cors.MaybeCreateJob();

  EXPECT_TRUE(sub_cors_job);
  EXPECT_FALSE(sub_cors_job->ShouldFallbackToNetwork());
  EXPECT_FALSE(sub_cors_job->ShouldForwardToServiceWorker());

  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(sub_cors_job->ShouldFallbackToNetwork());
  EXPECT_FALSE(sub_cors_job->ShouldForwardToServiceWorker());
}

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
TEST_F(ServiceWorkerControlleeRequestHandlerTest, FallbackWithOfflineHeader) {
  version_->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  version_->SetStatus(ServiceWorkerVersion::ACTIVATED);
  registration_->SetActiveVersion(version_);
  context()->storage()->StoreRegistration(registration_.get(), version_.get(),
                                          base::DoNothing());
  base::RunLoop().RunUntilIdle();
  version_ = NULL;
  registration_ = NULL;

  ServiceWorkerRequestTestResources test_resources(
      this, GURL("https://host/scope/doc"), RESOURCE_TYPE_MAIN_FRAME);
  // Sets an offline header to indicate force loading offline page.
  test_resources.request()->SetExtraRequestHeaderByName(
      "X-Chrome-offline", "reason=download", true);
  ServiceWorkerURLRequestJob* sw_job = test_resources.MaybeCreateJob();

  EXPECT_FALSE(sw_job);
}

TEST_F(ServiceWorkerControlleeRequestHandlerTest, FallbackWithNoOfflineHeader) {
  version_->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  version_->SetStatus(ServiceWorkerVersion::ACTIVATED);
  registration_->SetActiveVersion(version_);
  context()->storage()->StoreRegistration(registration_.get(), version_.get(),
                                          base::DoNothing());
  base::RunLoop().RunUntilIdle();
  version_ = NULL;
  registration_ = NULL;

  ServiceWorkerRequestTestResources test_resources(
      this, GURL("https://host/scope/doc"), RESOURCE_TYPE_MAIN_FRAME);
  // Empty offline header value should not cause fallback.
  test_resources.request()->SetExtraRequestHeaderByName("X-Chrome-offline", "",
                                                        true);
  ServiceWorkerURLRequestJob* sw_job = test_resources.MaybeCreateJob();

  EXPECT_TRUE(sw_job);
}
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGE

}  // namespace service_worker_controllee_request_handler_unittest
}  // namespace content
