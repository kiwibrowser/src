// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_context_request_handler.h"

#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_provider_host.h"
#include "content/browser/service_worker/service_worker_read_from_cache_job.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/browser/service_worker/service_worker_test_utils.h"
#include "content/browser/service_worker/service_worker_write_to_cache_job.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/content_features.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/public/mojom/request_context_frame_type.mojom.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"

namespace content {

class MockHttpProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  MockHttpProtocolHandler(ResourceContext* resource_context)
      : resource_context_(resource_context) {}

  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    ServiceWorkerRequestHandler* handler =
        ServiceWorkerRequestHandler::GetHandler(request);
    return handler->MaybeCreateJob(request, network_delegate,
                                   resource_context_);
  }

 private:
  ResourceContext* resource_context_;
};

class ServiceWorkerContextRequestHandlerTest : public testing::Test {
 public:
  ServiceWorkerContextRequestHandlerTest()
      : browser_thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    helper_.reset(new EmbeddedWorkerTestHelper(base::FilePath()));
    context()->storage()->LazyInitializeForTest(base::DoNothing());
    base::RunLoop().RunUntilIdle();

    // A new unstored registration/version.
    scope_ = GURL("https://host/scope/");
    script_url_ = GURL("https://host/script.js");
    import_script_url_ = GURL("https://host/import.js");

    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = scope_;
    registration_ = base::MakeRefCounted<ServiceWorkerRegistration>(
        options, 1L, context()->AsWeakPtr());
    version_ = new ServiceWorkerVersion(registration_.get(), script_url_,
                                        context()->storage()->NewVersionId(),
                                        context()->AsWeakPtr());
    SetUpProvider();

    std::unique_ptr<MockHttpProtocolHandler> handler(
        new MockHttpProtocolHandler(
            helper_->browser_context()->GetResourceContext()));
    url_request_job_factory_.SetProtocolHandler("https", std::move(handler));
    url_request_context_.set_job_factory(&url_request_job_factory_);
  }

  void TearDown() override {
    version_ = nullptr;
    registration_ = nullptr;
    helper_.reset();
  }

  ServiceWorkerContextCore* context() const { return helper_->context(); }

  void SetUpProvider() {
    std::unique_ptr<ServiceWorkerProviderHost> host =
        CreateProviderHostForServiceWorkerContext(
            helper_->mock_render_process_id(),
            true /* is_parent_frame_secure */, version_.get(),
            context()->AsWeakPtr(), &remote_endpoint_);
    provider_host_ = host->AsWeakPtr();
    context()->AddProviderHost(std::move(host));
  }

  std::unique_ptr<net::URLRequest> CreateRequest(const GURL& url) {
    return url_request_context_.CreateRequest(url, net::DEFAULT_PRIORITY,
                                              &url_request_delegate_,
                                              TRAFFIC_ANNOTATION_FOR_TESTS);
  }

  // Creates a ServiceWorkerContextHandler directly.
  std::unique_ptr<ServiceWorkerContextRequestHandler> CreateHandler(
      ResourceType resource_type) {
    return std::make_unique<ServiceWorkerContextRequestHandler>(
        context()->AsWeakPtr(), provider_host_,
        base::WeakPtr<storage::BlobStorageContext>(), resource_type);
  }

  // Associates a ServiceWorkerRequestHandler with a request. Use this instead
  // of CreateHandler if you want to actually start the request to test what the
  // job created by the handler does.
  void InitializeHandler(net::URLRequest* request) {
    ServiceWorkerRequestHandler::InitializeHandler(
        request, helper_->context_wrapper(), &blob_storage_context_,
        helper_->mock_render_process_id(), provider_host_->provider_id(),
        false /* skip_service_worker */,
        network::mojom::FetchRequestMode::kNoCORS,
        network::mojom::FetchCredentialsMode::kOmit,
        network::mojom::FetchRedirectMode::kFollow,
        std::string() /* integrity */, false /* keepalive */,
        RESOURCE_TYPE_SERVICE_WORKER, REQUEST_CONTEXT_TYPE_SERVICE_WORKER,
        network::mojom::RequestContextFrameType::kNone, nullptr);
  }

  // Tests if net::LOAD_BYPASS_CACHE is set for a resource fetch.
  void TestBypassCache(const GURL& url,
                       ResourceType resource_type,
                       bool expect_bypass) {
    std::unique_ptr<net::URLRequest> request(CreateRequest(url));
    std::unique_ptr<ServiceWorkerContextRequestHandler> handler(
        CreateHandler(resource_type));
    std::unique_ptr<net::URLRequestJob> job(
        handler->MaybeCreateJob(request.get(), nullptr, nullptr));
    ASSERT_TRUE(job.get());
    ServiceWorkerWriteToCacheJob* sw_job =
        static_cast<ServiceWorkerWriteToCacheJob*>(job.get());
    if (expect_bypass)
      EXPECT_TRUE(sw_job->net_request_->load_flags() & net::LOAD_BYPASS_CACHE);
    else
      EXPECT_FALSE(sw_job->net_request_->load_flags() & net::LOAD_BYPASS_CACHE);
  }

  void TestBypassCacheForMainScript(bool expect_bypass) {
    TestBypassCache(script_url_, RESOURCE_TYPE_SERVICE_WORKER, expect_bypass);
  }

  void TestBypassCacheForImportedScript(bool expect_bypass) {
    TestBypassCache(import_script_url_, RESOURCE_TYPE_SCRIPT, expect_bypass);
  }

 protected:
  TestBrowserThreadBundle browser_thread_bundle_;
  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;
  base::WeakPtr<ServiceWorkerProviderHost> provider_host_;
  net::URLRequestContext url_request_context_;
  net::TestDelegate url_request_delegate_;
  net::URLRequestJobFactoryImpl url_request_job_factory_;
  GURL scope_;
  GURL script_url_;
  GURL import_script_url_;
  ServiceWorkerRemoteProviderEndpoint remote_endpoint_;
  storage::BlobStorageContext blob_storage_context_;
};

TEST_F(ServiceWorkerContextRequestHandlerTest, UpdateBefore24Hours) {
  // Give the registration a very recent last update time and pretend
  // we're installing a new version.
  registration_->set_last_update_check(base::Time::Now());
  version_->SetStatus(ServiceWorkerVersion::NEW);

  TestBypassCacheForMainScript(true);
  TestBypassCacheForImportedScript(false);
}

TEST_F(ServiceWorkerContextRequestHandlerTest,
       UpdateBefore24HoursWithUpdateViaCacheAll) {
  registration_->SetUpdateViaCache(
      blink::mojom::ServiceWorkerUpdateViaCache::kAll);
  // Give the registration a very recent last update time and pretend
  // we're installing a new version.
  registration_->set_last_update_check(base::Time::Now());
  version_->SetStatus(ServiceWorkerVersion::NEW);

  TestBypassCacheForMainScript(false);
  TestBypassCacheForImportedScript(false);
}

TEST_F(ServiceWorkerContextRequestHandlerTest,
       UpdateBefore24HoursWithUpdateViaCacheNone) {
  registration_->SetUpdateViaCache(
      blink::mojom::ServiceWorkerUpdateViaCache::kNone);
  // Give the registration a very recent last update time and pretend
  // we're installing a new version.
  registration_->set_last_update_check(base::Time::Now());
  version_->SetStatus(ServiceWorkerVersion::NEW);

  TestBypassCacheForMainScript(true);
  TestBypassCacheForImportedScript(true);
}

TEST_F(ServiceWorkerContextRequestHandlerTest, UpdateAfter24Hours) {
  // Give the registration a old update time and pretend
  // we're installing a new version.
  registration_->set_last_update_check(base::Time::Now() -
                                       base::TimeDelta::FromDays(7));
  version_->SetStatus(ServiceWorkerVersion::NEW);

  TestBypassCacheForMainScript(true);
  TestBypassCacheForImportedScript(true);
}

TEST_F(ServiceWorkerContextRequestHandlerTest,
       UpdateAfter24HoursWithUpdateViaCacheAll) {
  registration_->SetUpdateViaCache(
      blink::mojom::ServiceWorkerUpdateViaCache::kAll);
  // Give the registration a old update time and pretend
  // we're installing a new version.
  registration_->set_last_update_check(base::Time::Now() -
                                       base::TimeDelta::FromDays(7));
  version_->SetStatus(ServiceWorkerVersion::NEW);

  TestBypassCacheForMainScript(true);
  TestBypassCacheForImportedScript(true);
}

TEST_F(ServiceWorkerContextRequestHandlerTest,
       UpdateAfter24HoursWithUpdateViaCacheNone) {
  registration_->SetUpdateViaCache(
      blink::mojom::ServiceWorkerUpdateViaCache::kNone);
  // Give the registration a old update time and pretend
  // we're installing a new version.
  registration_->set_last_update_check(
      base::Time::Now() - base::TimeDelta::FromDays(7));
  version_->SetStatus(ServiceWorkerVersion::NEW);

  TestBypassCacheForMainScript(true);
  TestBypassCacheForImportedScript(true);
}

TEST_F(ServiceWorkerContextRequestHandlerTest, UpdateForceBypassCache) {
  registration_->SetUpdateViaCache(
      blink::mojom::ServiceWorkerUpdateViaCache::kAll);
  // Give the registration a very recent last update time and pretend
  // we're installing a new version.
  registration_->set_last_update_check(base::Time::Now());
  version_->SetStatus(ServiceWorkerVersion::NEW);
  version_->set_force_bypass_cache_for_scripts(true);

  TestBypassCacheForMainScript(true);
  TestBypassCacheForImportedScript(true);
}

TEST_F(ServiceWorkerContextRequestHandlerTest,
       ServiceWorkerDataRequestAnnotation) {
  version_->SetStatus(ServiceWorkerVersion::NEW);

  // Conduct a resource fetch for the main script.
  base::HistogramTester histograms;
  std::unique_ptr<net::URLRequest> request(CreateRequest(script_url_));
  std::unique_ptr<ServiceWorkerContextRequestHandler> handler(
      CreateHandler(RESOURCE_TYPE_SERVICE_WORKER));
  std::unique_ptr<net::URLRequestJob> job(
      handler->MaybeCreateJob(request.get(), nullptr, nullptr));
  ASSERT_TRUE(job.get());
  ServiceWorkerWriteToCacheJob* sw_job =
      static_cast<ServiceWorkerWriteToCacheJob*>(job.get());

  histograms.ExpectUniqueSample(
      "ServiceWorker.ContextRequestHandlerStatus.NewWorker.MainScript",
      static_cast<int>(
          ServiceWorkerContextRequestHandler::CreateJobStatus::WRITE_JOB),
      1);
  // Verify that the request is properly annotated as originating from a
  // Service Worker.
  EXPECT_TRUE(ResourceRequestInfo::OriginatedFromServiceWorker(
      sw_job->net_request_.get()));
}

// Tests starting a service worker when the skip_service_worker flag is on. The
// flag should be ignored.
TEST_F(ServiceWorkerContextRequestHandlerTest,
       SkipServiceWorkerForServiceWorkerRequest) {
  // Conduct a resource fetch for the main script.
  version_->SetStatus(ServiceWorkerVersion::NEW);
  std::unique_ptr<net::URLRequest> request(CreateRequest(script_url_));
  ServiceWorkerRequestHandler::InitializeHandler(
      request.get(), helper_->context_wrapper(), &blob_storage_context_,
      helper_->mock_render_process_id(), provider_host_->provider_id(),
      true /* skip_service_worker */, network::mojom::FetchRequestMode::kNoCORS,
      network::mojom::FetchCredentialsMode::kOmit,
      network::mojom::FetchRedirectMode::kFollow, std::string() /* integrity */,
      false /* keepalive */, RESOURCE_TYPE_SERVICE_WORKER,
      REQUEST_CONTEXT_TYPE_SERVICE_WORKER,
      network::mojom::RequestContextFrameType::kNone, nullptr);
  // Verify a ServiceWorkerRequestHandler was created.
  ServiceWorkerRequestHandler* handler =
      ServiceWorkerRequestHandler::GetHandler(request.get());
  EXPECT_TRUE(handler);
}

TEST_F(ServiceWorkerContextRequestHandlerTest, NewWorker) {
  // Conduct a resource fetch for the main script.
  {
    base::HistogramTester histograms;
    std::unique_ptr<net::URLRequest> request(CreateRequest(script_url_));
    std::unique_ptr<ServiceWorkerContextRequestHandler> handler(
        CreateHandler(RESOURCE_TYPE_SERVICE_WORKER));
    std::unique_ptr<net::URLRequestJob> job(
        handler->MaybeCreateJob(request.get(), nullptr, nullptr));
    EXPECT_TRUE(job);
    histograms.ExpectUniqueSample(
        "ServiceWorker.ContextRequestHandlerStatus.NewWorker.MainScript",
        static_cast<int>(
            ServiceWorkerContextRequestHandler::CreateJobStatus::WRITE_JOB),
        1);
  }

  // Conduct a resource fetch for an imported script.
  {
    base::HistogramTester histograms;
    std::unique_ptr<net::URLRequest> request(CreateRequest(import_script_url_));
    std::unique_ptr<ServiceWorkerContextRequestHandler> handler(
        CreateHandler(RESOURCE_TYPE_SCRIPT));
    std::unique_ptr<net::URLRequestJob> job(
        handler->MaybeCreateJob(request.get(), nullptr, nullptr));
    EXPECT_TRUE(job);
    histograms.ExpectUniqueSample(
        "ServiceWorker.ContextRequestHandlerStatus.NewWorker.ImportedScript",
        static_cast<int>(
            ServiceWorkerContextRequestHandler::CreateJobStatus::WRITE_JOB),
        1);
  }
}

TEST_F(ServiceWorkerContextRequestHandlerTest, InstalledWorker) {
  using Resource = ServiceWorkerDatabase::ResourceRecord;
  std::vector<Resource> resources = {
      Resource(context()->storage()->NewResourceId(), script_url_, 100),
      Resource(context()->storage()->NewResourceId(), import_script_url_, 100)};
  version_->script_cache_map()->SetResources(resources);
  version_->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  version_->SetStatus(ServiceWorkerVersion::ACTIVATED);
  registration_->SetActiveVersion(version_);

  // Conduct a resource fetch for the main script.
  {
    base::HistogramTester histograms;
    std::unique_ptr<net::URLRequest> request(CreateRequest(script_url_));
    std::unique_ptr<ServiceWorkerContextRequestHandler> handler(
        CreateHandler(RESOURCE_TYPE_SERVICE_WORKER));
    std::unique_ptr<net::URLRequestJob> job(
        handler->MaybeCreateJob(request.get(), nullptr, nullptr));
    EXPECT_TRUE(job);
    histograms.ExpectUniqueSample(
        "ServiceWorker.ContextRequestHandlerStatus.InstalledWorker.MainScript",
        static_cast<int>(
            ServiceWorkerContextRequestHandler::CreateJobStatus::READ_JOB),
        1);
  }

  // Conduct a resource fetch for an imported script.
  {
    base::HistogramTester histograms;
    std::unique_ptr<net::URLRequest> request(CreateRequest(import_script_url_));
    std::unique_ptr<ServiceWorkerContextRequestHandler> handler(
        CreateHandler(RESOURCE_TYPE_SCRIPT));
    std::unique_ptr<net::URLRequestJob> job(
        handler->MaybeCreateJob(request.get(), nullptr, nullptr));
    EXPECT_TRUE(job);
    histograms.ExpectUniqueSample(
        "ServiceWorker.ContextRequestHandlerStatus.InstalledWorker."
        "ImportedScript",
        static_cast<int>(
            ServiceWorkerContextRequestHandler::CreateJobStatus::READ_JOB),
        1);
  }
}

TEST_F(ServiceWorkerContextRequestHandlerTest, Incumbent) {
  // Make an incumbent version.
  scoped_refptr<ServiceWorkerVersion> incumbent = new ServiceWorkerVersion(
      registration_.get(), script_url_, context()->storage()->NewVersionId(),
      context()->AsWeakPtr());
  incumbent->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  std::vector<ServiceWorkerDatabase::ResourceRecord> resources = {
      ServiceWorkerDatabase::ResourceRecord(
          context()->storage()->NewResourceId(), script_url_, 100)};
  incumbent->script_cache_map()->SetResources(resources);
  incumbent->SetStatus(ServiceWorkerVersion::ACTIVATED);
  registration_->SetActiveVersion(incumbent);

  // Make a new version.
  version_->SetStatus(ServiceWorkerVersion::NEW);

  // Conduct a resource fetch for the main script.
  base::HistogramTester histograms;
  std::unique_ptr<net::URLRequest> request(CreateRequest(script_url_));
  std::unique_ptr<ServiceWorkerContextRequestHandler> handler(
      CreateHandler(RESOURCE_TYPE_SERVICE_WORKER));
  std::unique_ptr<net::URLRequestJob> job(
      handler->MaybeCreateJob(request.get(), nullptr, nullptr));
  histograms.ExpectUniqueSample(
      "ServiceWorker.ContextRequestHandlerStatus.NewWorker.MainScript",
      static_cast<int>(ServiceWorkerContextRequestHandler::CreateJobStatus::
                           WRITE_JOB_WITH_INCUMBENT),
      1);
}

TEST_F(ServiceWorkerContextRequestHandlerTest, ErrorCases) {
  {
    // Set up a request.
    base::HistogramTester histograms;
    std::unique_ptr<net::URLRequest> request(CreateRequest(script_url_));
    InitializeHandler(request.get());

    // Make the version redundant.
    version_->SetStatus(ServiceWorkerVersion::REDUNDANT);

    // Verify that the request fails.
    request->Start();
    base::RunLoop().Run();
    EXPECT_EQ(net::ERR_FAILED, url_request_delegate_.request_status());
    histograms.ExpectUniqueSample(
        "ServiceWorker.ContextRequestHandlerStatus.NewWorker.MainScript",
        static_cast<int>(ServiceWorkerContextRequestHandler::CreateJobStatus::
                             ERROR_REDUNDANT_VERSION),
        1);
  }
  // Return the version to normal.
  version_->SetStatus(ServiceWorkerVersion::NEW);

  {
    // Set up a request.
    base::HistogramTester histograms;
    std::unique_ptr<net::URLRequest> request(CreateRequest(script_url_));
    InitializeHandler(request.get());

    // Remove the host.
    context()->RemoveProviderHost(helper_->mock_render_process_id(),
                                  provider_host_->provider_id());

    // Verify that the request fails.
    request->Start();
    base::RunLoop().Run();
    EXPECT_EQ(net::ERR_FAILED, url_request_delegate_.request_status());
    histograms.ExpectUniqueSample(
        "ServiceWorker.ContextRequestHandlerStatus.NewWorker.MainScript",
        static_cast<int>(ServiceWorkerContextRequestHandler::CreateJobStatus::
                             ERROR_NO_PROVIDER),
        1);
  }
  // Recreate the host.
  SetUpProvider();

  {
    // Set up a request.
    base::HistogramTester histograms;
    std::unique_ptr<net::URLRequest> request(CreateRequest(script_url_));
    InitializeHandler(request.get());

    // Destroy the context.
    helper_->ShutdownContext();
    base::RunLoop().RunUntilIdle();

    // Verify that the request fails.
    request->Start();
    base::RunLoop().Run();
    EXPECT_EQ(net::ERR_FAILED, url_request_delegate_.request_status());
    histograms.ExpectUniqueSample(
        "ServiceWorker.ContextRequestHandlerStatus.NewWorker.MainScript",
        static_cast<int>(ServiceWorkerContextRequestHandler::CreateJobStatus::
                             ERROR_NO_CONTEXT),
        1);
  }
}

}  // namespace content
