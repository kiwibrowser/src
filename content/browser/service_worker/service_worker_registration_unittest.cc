// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_registration.h"

#include <stdint.h>
#include <utility>

#include "base/callback_helpers.h"
#include "base/compiler_specific.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/browser/service_worker/embedded_worker_status.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_dispatcher_host.h"
#include "content/browser/service_worker/service_worker_provider_host.h"
#include "content/browser/service_worker/service_worker_registration_object_host.h"
#include "content/browser/service_worker/service_worker_test_utils.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/test/test_content_browser_client.h"
#include "mojo/edk/embedder/embedder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_object.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"
#include "url/gurl.h"

namespace content {
namespace service_worker_registration_unittest {

// From service_worker_registration.cc.
constexpr base::TimeDelta kMaxLameDuckTime = base::TimeDelta::FromMinutes(5);

int CreateInflightRequest(ServiceWorkerVersion* version) {
  version->StartWorker(ServiceWorkerMetrics::EventType::PUSH,
                       base::DoNothing());
  base::RunLoop().RunUntilIdle();
  return version->StartRequest(ServiceWorkerMetrics::EventType::PUSH,
                               base::DoNothing());
}

static void SaveStatusCallback(bool* called,
                               ServiceWorkerStatusCode* out,
                               ServiceWorkerStatusCode status) {
  *called = true;
  *out = status;
}

class ServiceWorkerTestContentBrowserClient : public TestContentBrowserClient {
 public:
  bool AllowServiceWorker(
      const GURL& scope,
      const GURL& first_party,
      content::ResourceContext* context,
      const base::Callback<WebContents*(void)>& wc_getter) override {
    return false;
  }
};

class MockServiceWorkerRegistrationObject
    : public blink::mojom::ServiceWorkerRegistrationObject {
 public:
  explicit MockServiceWorkerRegistrationObject(
      blink::mojom::ServiceWorkerRegistrationObjectAssociatedRequest request)
      : binding_(this) {
    binding_.Bind(std::move(request));
  }
  ~MockServiceWorkerRegistrationObject() override = default;

  int update_found_called_count() const { return update_found_called_count_; };
  int set_version_attributes_called_count() const {
    return set_version_attributes_called_count_;
  };
  int set_update_via_cache_called_count() const {
    return set_update_via_cache_called_count_;
  }
  int changed_mask() const { return changed_mask_; }
  const blink::mojom::ServiceWorkerObjectInfoPtr& installing() const {
    return installing_;
  }
  const blink::mojom::ServiceWorkerObjectInfoPtr& waiting() const {
    return waiting_;
  }
  const blink::mojom::ServiceWorkerObjectInfoPtr& active() const {
    return active_;
  }
  blink::mojom::ServiceWorkerUpdateViaCache update_via_cache() const {
    return update_via_cache_;
  }

 private:
  // Implements blink::mojom::ServiceWorkerRegistrationObject.
  void SetVersionAttributes(
      int changed_mask,
      blink::mojom::ServiceWorkerObjectInfoPtr installing,
      blink::mojom::ServiceWorkerObjectInfoPtr waiting,
      blink::mojom::ServiceWorkerObjectInfoPtr active) override {
    set_version_attributes_called_count_++;
    changed_mask_ = changed_mask;
    installing_ = std::move(installing);
    waiting_ = std::move(waiting);
    active_ = std::move(active);
  }
  void SetUpdateViaCache(
      blink::mojom::ServiceWorkerUpdateViaCache update_via_cache) override {
    set_update_via_cache_called_count_++;
    update_via_cache_ = update_via_cache;
  }
  void UpdateFound() override { update_found_called_count_++; }

  int update_found_called_count_ = 0;
  int set_version_attributes_called_count_ = 0;
  int set_update_via_cache_called_count_ = 0;
  int changed_mask_ = 0;
  blink::mojom::ServiceWorkerObjectInfoPtr installing_;
  blink::mojom::ServiceWorkerObjectInfoPtr waiting_;
  blink::mojom::ServiceWorkerObjectInfoPtr active_;
  blink::mojom::ServiceWorkerUpdateViaCache update_via_cache_ =
      blink::mojom::ServiceWorkerUpdateViaCache::kImports;

  mojo::AssociatedBinding<blink::mojom::ServiceWorkerRegistrationObject>
      binding_;
};

class ServiceWorkerRegistrationTest : public testing::Test {
 public:
  ServiceWorkerRegistrationTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    helper_.reset(new EmbeddedWorkerTestHelper(base::FilePath()));

    context()->storage()->LazyInitializeForTest(base::DoNothing());
    base::RunLoop().RunUntilIdle();
  }

  void TearDown() override {
    helper_.reset();
    base::RunLoop().RunUntilIdle();
  }

  ServiceWorkerContextCore* context() { return helper_->context(); }
  ServiceWorkerStorage* storage() { return helper_->context()->storage(); }

  class RegistrationListener : public ServiceWorkerRegistration::Listener {
   public:
    RegistrationListener() {}
    ~RegistrationListener() {
      if (observed_registration_.get())
        observed_registration_->RemoveListener(this);
    }

    void OnVersionAttributesChanged(
        ServiceWorkerRegistration* registration,
        ChangedVersionAttributesMask changed_mask,
        const ServiceWorkerRegistrationInfo& info) override {
      observed_registration_ = registration;
      observed_changed_mask_ = changed_mask;
      observed_info_ = info;
    }

    void OnRegistrationFailed(
        ServiceWorkerRegistration* registration) override {
      NOTREACHED();
    }

    void OnUpdateFound(ServiceWorkerRegistration* registration) override {
      NOTREACHED();
    }

    void Reset() {
      observed_registration_ = nullptr;
      observed_changed_mask_ = ChangedVersionAttributesMask();
      observed_info_ = ServiceWorkerRegistrationInfo();
    }

    scoped_refptr<ServiceWorkerRegistration> observed_registration_;
    ChangedVersionAttributesMask observed_changed_mask_;
    ServiceWorkerRegistrationInfo observed_info_;
  };

 protected:
  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
  TestBrowserThreadBundle thread_bundle_;
};

TEST_F(ServiceWorkerRegistrationTest, SetAndUnsetVersions) {
  const GURL kScope("http://www.example.not/");
  const GURL kScript("http://www.example.not/service_worker.js");
  int64_t kRegistrationId = 1L;

  blink::mojom::ServiceWorkerRegistrationOptions options;
  options.scope = kScope;
  scoped_refptr<ServiceWorkerRegistration> registration =
      base::MakeRefCounted<ServiceWorkerRegistration>(options, kRegistrationId,
                                                      context()->AsWeakPtr());

  const int64_t version_1_id = 1L;
  const int64_t version_2_id = 2L;
  scoped_refptr<ServiceWorkerVersion> version_1 =
      base::MakeRefCounted<ServiceWorkerVersion>(
          registration.get(), kScript, version_1_id, context()->AsWeakPtr());
  scoped_refptr<ServiceWorkerVersion> version_2 =
      base::MakeRefCounted<ServiceWorkerVersion>(
          registration.get(), kScript, version_2_id, context()->AsWeakPtr());

  RegistrationListener listener;
  registration->AddListener(&listener);
  registration->SetActiveVersion(version_1);

  EXPECT_EQ(version_1.get(), registration->active_version());
  EXPECT_EQ(registration, listener.observed_registration_);
  EXPECT_EQ(ChangedVersionAttributesMask::ACTIVE_VERSION,
            listener.observed_changed_mask_.changed());
  EXPECT_EQ(kScope, listener.observed_info_.pattern);
  EXPECT_EQ(version_1_id, listener.observed_info_.active_version.version_id);
  EXPECT_EQ(kScript, listener.observed_info_.active_version.script_url);
  EXPECT_EQ(listener.observed_info_.installing_version.version_id,
            blink::mojom::kInvalidServiceWorkerVersionId);
  EXPECT_EQ(listener.observed_info_.waiting_version.version_id,
            blink::mojom::kInvalidServiceWorkerVersionId);
  listener.Reset();

  registration->SetInstallingVersion(version_2);

  EXPECT_EQ(version_2.get(), registration->installing_version());
  EXPECT_EQ(ChangedVersionAttributesMask::INSTALLING_VERSION,
            listener.observed_changed_mask_.changed());
  EXPECT_EQ(version_1_id, listener.observed_info_.active_version.version_id);
  EXPECT_EQ(version_2_id,
            listener.observed_info_.installing_version.version_id);
  EXPECT_EQ(listener.observed_info_.waiting_version.version_id,
            blink::mojom::kInvalidServiceWorkerVersionId);
  listener.Reset();

  registration->SetWaitingVersion(version_2);

  EXPECT_EQ(version_2.get(), registration->waiting_version());
  EXPECT_FALSE(registration->installing_version());
  EXPECT_TRUE(listener.observed_changed_mask_.waiting_changed());
  EXPECT_TRUE(listener.observed_changed_mask_.installing_changed());
  EXPECT_EQ(version_1_id, listener.observed_info_.active_version.version_id);
  EXPECT_EQ(version_2_id, listener.observed_info_.waiting_version.version_id);
  EXPECT_EQ(listener.observed_info_.installing_version.version_id,
            blink::mojom::kInvalidServiceWorkerVersionId);
  listener.Reset();

  registration->UnsetVersion(version_2.get());

  EXPECT_FALSE(registration->waiting_version());
  EXPECT_EQ(ChangedVersionAttributesMask::WAITING_VERSION,
            listener.observed_changed_mask_.changed());
  EXPECT_EQ(version_1_id, listener.observed_info_.active_version.version_id);
  EXPECT_EQ(listener.observed_info_.waiting_version.version_id,
            blink::mojom::kInvalidServiceWorkerVersionId);
  EXPECT_EQ(listener.observed_info_.installing_version.version_id,
            blink::mojom::kInvalidServiceWorkerVersionId);
}

TEST_F(ServiceWorkerRegistrationTest, FailedRegistrationNoCrash) {
  const GURL kScope("http://www.example.not/");
  int64_t kRegistrationId = 1L;
  blink::mojom::ServiceWorkerRegistrationOptions options;
  options.scope = kScope;
  auto registration = base::MakeRefCounted<ServiceWorkerRegistration>(
      options, kRegistrationId, context()->AsWeakPtr());
  auto dispatcher_host = base::MakeRefCounted<ServiceWorkerDispatcherHost>(
      helper_->mock_render_process_id());
  // Prepare a ServiceWorkerProviderHost.
  ServiceWorkerRemoteProviderEndpoint remote_endpoint;
  std::unique_ptr<ServiceWorkerProviderHost> provider_host =
      CreateProviderHostWithDispatcherHost(
          helper_->mock_render_process_id(), 1 /* dummy provider_id */,
          context()->AsWeakPtr(), 1 /* route_id */, dispatcher_host.get(),
          &remote_endpoint);
  auto registration_object_host =
      std::make_unique<ServiceWorkerRegistrationObjectHost>(
          context()->AsWeakPtr(), provider_host.get(), registration);
  // To enable the caller end point
  // |registration_object_host->remote_registration_| to make calls safely with
  // no need to pass |object_info_->request| through a message pipe endpoint.
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr object_info =
      registration_object_host->CreateObjectInfo();
  mojo::AssociateWithDisconnectedPipe(object_info->request.PassHandle());

  registration->NotifyRegistrationFailed();
  // Don't crash when |registration_object_host| gets destructed.
}

TEST_F(ServiceWorkerRegistrationTest, NavigationPreload) {
  const GURL kScope("http://www.example.not/");
  const GURL kScript("https://www.example.not/service_worker.js");
  // Setup.

  blink::mojom::ServiceWorkerRegistrationOptions options;
  options.scope = kScope;
  scoped_refptr<ServiceWorkerRegistration> registration =
      base::MakeRefCounted<ServiceWorkerRegistration>(
          options, storage()->NewRegistrationId(), context()->AsWeakPtr());
  scoped_refptr<ServiceWorkerVersion> version_1 =
      base::MakeRefCounted<ServiceWorkerVersion>(registration.get(), kScript,
                                                 storage()->NewVersionId(),
                                                 context()->AsWeakPtr());
  version_1->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  registration->SetActiveVersion(version_1);
  version_1->SetStatus(ServiceWorkerVersion::ACTIVATED);
  scoped_refptr<ServiceWorkerVersion> version_2 =
      base::MakeRefCounted<ServiceWorkerVersion>(registration.get(), kScript,
                                                 storage()->NewVersionId(),
                                                 context()->AsWeakPtr());
  version_2->set_fetch_handler_existence(
      ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  registration->SetWaitingVersion(version_2);
  version_2->SetStatus(ServiceWorkerVersion::INSTALLED);

  // Navigation preload is disabled by default.
  EXPECT_FALSE(version_1->navigation_preload_state().enabled);
  // Enabling it sets the flag on the active version.
  registration->EnableNavigationPreload(true);
  EXPECT_TRUE(version_1->navigation_preload_state().enabled);
  // A new active version gets the flag.
  registration->SetActiveVersion(version_2);
  version_2->SetStatus(ServiceWorkerVersion::ACTIVATING);
  EXPECT_TRUE(version_2->navigation_preload_state().enabled);
  // Disabling it unsets the flag on the active version.
  registration->EnableNavigationPreload(false);
  EXPECT_FALSE(version_2->navigation_preload_state().enabled);
}

// Sets up a registration with a waiting worker, and an active worker
// with a controllee and an inflight request.
class ServiceWorkerActivationTest : public ServiceWorkerRegistrationTest {
 public:
  ServiceWorkerActivationTest() : ServiceWorkerRegistrationTest() {}

  void SetUp() override {
    ServiceWorkerRegistrationTest::SetUp();

    const GURL kScope("https://www.example.not/");
    const GURL kScript("https://www.example.not/service_worker.js");

    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = kScope;
    registration_ = base::MakeRefCounted<ServiceWorkerRegistration>(
        options, storage()->NewRegistrationId(), context()->AsWeakPtr());

    // Create an active version.
    scoped_refptr<ServiceWorkerVersion> version_1 =
        base::MakeRefCounted<ServiceWorkerVersion>(registration_.get(), kScript,
                                                   storage()->NewVersionId(),
                                                   context()->AsWeakPtr());
    version_1->set_fetch_handler_existence(
        ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
    registration_->SetActiveVersion(version_1);
    version_1->SetStatus(ServiceWorkerVersion::ACTIVATED);

    // Store the registration.
    std::vector<ServiceWorkerDatabase::ResourceRecord> records_1;
    records_1.push_back(WriteToDiskCacheSync(
        helper_->context()->storage(), version_1->script_url(),
        helper_->context()->storage()->NewResourceId(), {} /* headers */,
        "I'm the body", "I'm the meta data"));
    version_1->script_cache_map()->SetResources(records_1);
    version_1->SetMainScriptHttpResponseInfo(
        EmbeddedWorkerTestHelper::CreateHttpResponseInfo());
    ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_MAX_VALUE;
    context()->storage()->StoreRegistration(
        registration_.get(), version_1.get(),
        CreateReceiverOnCurrentThread(&status));
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(SERVICE_WORKER_OK, status);

    // Give the active version a controllee.
    host_ = CreateProviderHostForWindow(
        helper_->mock_render_process_id(), 1 /* dummy provider_id */,
        true /* is_parent_frame_secure */, context()->AsWeakPtr(),
        &remote_endpoint_);
    DCHECK(remote_endpoint_.client_request()->is_pending());
    DCHECK(remote_endpoint_.host_ptr()->is_bound());
    version_1->AddControllee(host_.get());

    // Give the active version an in-flight request.
    inflight_request_id_ = CreateInflightRequest(version_1.get());

    // Create a waiting version.
    scoped_refptr<ServiceWorkerVersion> version_2 =
        base::MakeRefCounted<ServiceWorkerVersion>(registration_.get(), kScript,
                                                   storage()->NewVersionId(),
                                                   context()->AsWeakPtr());
    std::vector<ServiceWorkerDatabase::ResourceRecord> records_2;
    records_2.push_back(WriteToDiskCacheSync(
        helper_->context()->storage(), version_2->script_url(),
        helper_->context()->storage()->NewResourceId(), {} /* headers */,
        "I'm the body", "I'm the meta data"));
    version_2->script_cache_map()->SetResources(records_2);
    version_2->SetMainScriptHttpResponseInfo(
        EmbeddedWorkerTestHelper::CreateHttpResponseInfo());
    version_2->set_fetch_handler_existence(
        ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
    registration_->SetWaitingVersion(version_2);
    version_2->StartWorker(ServiceWorkerMetrics::EventType::INSTALL,
                           base::DoNothing());
    version_2->SetStatus(ServiceWorkerVersion::INSTALLED);

    // Set it to activate when ready. The original version should still be
    // active.
    registration_->ActivateWaitingVersionWhenReady();
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(version_1.get(), registration_->active_version());
  }

  void TearDown() override {
    registration_->active_version()->RemoveListener(registration_.get());
    ServiceWorkerRegistrationTest::TearDown();
  }

  ServiceWorkerRegistration* registration() { return registration_.get(); }
  ServiceWorkerProviderHost* controllee() { return host_.get(); }
  int inflight_request_id() const { return inflight_request_id_; }

  bool IsLameDuckTimerRunning() {
    return registration_->lame_duck_timer_.IsRunning();
  }

  void RunLameDuckTimer() { registration_->RemoveLameDuckIfNeeded(); }

  // Simulates skipWaiting(). Note that skipWaiting() might not try to activate
  // the worker "immediately", if it can't yet be activated yet. If activation
  // is delayed, |out_result| will not be set. If activation is attempted,
  // |out_result| is generally true but false in case of a fatal/unexpected
  // error like ServiceWorkerContext shutdown.
  void SimulateSkipWaiting(ServiceWorkerVersion* version,
                           base::Optional<bool>* out_result) {
    version->SkipWaiting(
        base::BindOnce([](base::Optional<bool>* out_result,
                          bool success) { *out_result = success; },
                       out_result));
    base::RunLoop().RunUntilIdle();
  }

 private:
  scoped_refptr<ServiceWorkerRegistration> registration_;
  std::unique_ptr<ServiceWorkerProviderHost> host_;
  ServiceWorkerRemoteProviderEndpoint remote_endpoint_;
  int inflight_request_id_ = -1;
};

// Test activation triggered by finishing all requests.
TEST_F(ServiceWorkerActivationTest, NoInflightRequest) {
  scoped_refptr<ServiceWorkerRegistration> reg = registration();
  scoped_refptr<ServiceWorkerVersion> version_1 = reg->active_version();
  scoped_refptr<ServiceWorkerVersion> version_2 = reg->waiting_version();

  // Remove the controllee. Since there is an in-flight request,
  // activation should not yet happen.
  version_1->RemoveControllee(controllee());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(version_1.get(), reg->active_version());

  // Finish the request. Activation should happen.
  version_1->FinishRequest(inflight_request_id(), true /* was_handled */,
                           base::Time::Now());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(version_2.get(), reg->active_version());
}

// Test activation triggered by loss of controllee.
TEST_F(ServiceWorkerActivationTest, NoControllee) {
  scoped_refptr<ServiceWorkerRegistration> reg = registration();
  scoped_refptr<ServiceWorkerVersion> version_1 = reg->active_version();
  scoped_refptr<ServiceWorkerVersion> version_2 = reg->waiting_version();

  // Finish the request. Since there is a controllee, activation should not yet
  // happen.
  version_1->FinishRequest(inflight_request_id(), true /* was_handled */,
                           base::Time::Now());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(version_1.get(), reg->active_version());

  // Remove the controllee. Activation should happen.
  version_1->RemoveControllee(controllee());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(version_2.get(), reg->active_version());
}

// Test activation triggered by skipWaiting.
TEST_F(ServiceWorkerActivationTest, SkipWaiting) {
  scoped_refptr<ServiceWorkerRegistration> reg = registration();
  scoped_refptr<ServiceWorkerVersion> version_1 = reg->active_version();
  scoped_refptr<ServiceWorkerVersion> version_2 = reg->waiting_version();

  // Finish the in-flight request. Since there is a controllee,
  // activation should not happen.
  version_1->FinishRequest(inflight_request_id(), true /* was_handled */,
                           base::Time::Now());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(version_1.get(), reg->active_version());

  // Call skipWaiting. Activation should happen.
  base::Optional<bool> result;
  SimulateSkipWaiting(version_2.get(), &result);
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(*result);
  EXPECT_EQ(version_2.get(), reg->active_version());
}

// Test activation triggered by skipWaiting and finishing requests.
TEST_F(ServiceWorkerActivationTest, SkipWaitingWithInflightRequest) {
  scoped_refptr<ServiceWorkerRegistration> reg = registration();
  scoped_refptr<ServiceWorkerVersion> version_1 = reg->active_version();
  scoped_refptr<ServiceWorkerVersion> version_2 = reg->waiting_version();

  base::Optional<bool> result;
  // Set skip waiting flag. Since there is still an in-flight request,
  // activation should not happen.
  SimulateSkipWaiting(version_2.get(), &result);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(version_1.get(), reg->active_version());

  // Finish the request. Activation should happen.
  version_1->FinishRequest(inflight_request_id(), true /* was_handled */,
                           base::Time::Now());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(*result);
  EXPECT_EQ(version_2.get(), reg->active_version());
}

TEST_F(ServiceWorkerActivationTest, TimeSinceSkipWaiting_Installing) {
  scoped_refptr<ServiceWorkerRegistration> reg = registration();
  scoped_refptr<ServiceWorkerVersion> version = reg->waiting_version();
  base::SimpleTestTickClock clock;
  clock.SetNowTicks(base::TimeTicks::Now());
  version->SetTickClockForTesting(&clock);

  // Reset version to the installing phase.
  reg->UnsetVersion(version.get());
  version->SetStatus(ServiceWorkerVersion::INSTALLING);

  base::Optional<bool> result;
  // Call skipWaiting(). The time ticks since skip waiting shouldn't start
  // since the version is not yet installed.
  SimulateSkipWaiting(version.get(), &result);
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(*result);
  clock.Advance(base::TimeDelta::FromSeconds(11));
  EXPECT_EQ(base::TimeDelta(), version->TimeSinceSkipWaiting());

  // Install the version. Now the skip waiting time starts ticking.
  version->SetStatus(ServiceWorkerVersion::INSTALLED);
  reg->SetWaitingVersion(version);
  base::RunLoop().RunUntilIdle();
  clock.Advance(base::TimeDelta::FromSeconds(33));
  EXPECT_EQ(base::TimeDelta::FromSeconds(33), version->TimeSinceSkipWaiting());

  result.reset();
  // Call skipWaiting() again. It doesn't reset the time.
  SimulateSkipWaiting(version.get(), &result);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(base::TimeDelta::FromSeconds(33), version->TimeSinceSkipWaiting());
}

// Test lame duck timer triggered by skip waiting.
TEST_F(ServiceWorkerActivationTest, LameDuckTime_SkipWaiting) {
  scoped_refptr<ServiceWorkerRegistration> reg = registration();
  scoped_refptr<ServiceWorkerVersion> version_1 = reg->active_version();
  scoped_refptr<ServiceWorkerVersion> version_2 = reg->waiting_version();
  base::SimpleTestTickClock clock_1;
  base::SimpleTestTickClock clock_2;
  clock_1.SetNowTicks(base::TimeTicks::Now());
  clock_2.SetNowTicks(clock_1.NowTicks());
  version_1->SetTickClockForTesting(&clock_1);
  version_2->SetTickClockForTesting(&clock_2);

  base::Optional<bool> result;
  // Set skip waiting flag. Since there is still an in-flight request,
  // activation should not happen. But the lame duck timer should start.
  EXPECT_FALSE(IsLameDuckTimerRunning());
  SimulateSkipWaiting(version_2.get(), &result);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(version_1.get(), reg->active_version());
  EXPECT_TRUE(IsLameDuckTimerRunning());

  // Move forward by lame duck time.
  clock_2.Advance(kMaxLameDuckTime + base::TimeDelta::FromSeconds(1));

  // Activation should happen by the lame duck timer.
  RunLameDuckTimer();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(*result);
  EXPECT_EQ(version_2.get(), reg->active_version());
  EXPECT_FALSE(IsLameDuckTimerRunning());
}

// Test lame duck timer triggered by loss of controllee.
TEST_F(ServiceWorkerActivationTest, LameDuckTime_NoControllee) {
  scoped_refptr<ServiceWorkerRegistration> reg = registration();
  scoped_refptr<ServiceWorkerVersion> version_1 = reg->active_version();
  scoped_refptr<ServiceWorkerVersion> version_2 = reg->waiting_version();
  base::SimpleTestTickClock clock_1;
  base::SimpleTestTickClock clock_2;
  clock_1.SetNowTicks(base::TimeTicks::Now());
  clock_2.SetNowTicks(clock_1.NowTicks());
  version_1->SetTickClockForTesting(&clock_1);
  version_2->SetTickClockForTesting(&clock_2);

  // Remove the controllee. Since there is still an in-flight request,
  // activation should not happen. But the lame duck timer should start.
  EXPECT_FALSE(IsLameDuckTimerRunning());
  version_1->RemoveControllee(controllee());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(version_1.get(), reg->active_version());
  EXPECT_TRUE(IsLameDuckTimerRunning());

  // Move clock forward by a little bit.
  constexpr base::TimeDelta kLittleBit = base::TimeDelta::FromMinutes(1);
  clock_1.Advance(kLittleBit);

  // Add a controllee again to reset the lame duck period.
  version_1->AddControllee(controllee());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(IsLameDuckTimerRunning());

  // Remove the controllee.
  version_1->RemoveControllee(controllee());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(IsLameDuckTimerRunning());

  // Move clock forward to the next lame duck timer tick.
  clock_1.Advance(kMaxLameDuckTime - kLittleBit +
                  base::TimeDelta::FromSeconds(1));

  // Run the lame duck timer. Activation should not yet happen
  // since the lame duck period has not expired.
  RunLameDuckTimer();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(version_1.get(), reg->active_version());
  EXPECT_TRUE(IsLameDuckTimerRunning());

  // Continue on to the next lame duck timer tick.
  clock_1.Advance(kMaxLameDuckTime + base::TimeDelta::FromSeconds(1));

  // Activation should happen by the lame duck timer.
  RunLameDuckTimer();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(version_2.get(), reg->active_version());
  EXPECT_FALSE(IsLameDuckTimerRunning());
}

// Sets up a registration with a ServiceWorkerRegistrationObjectHost to hold it.
class ServiceWorkerRegistrationObjectHostTest
    : public ServiceWorkerRegistrationTest {
 protected:
  void SetUp() override {
    ServiceWorkerRegistrationTest::SetUp();
    mojo::edk::SetDefaultProcessErrorCallback(base::AdaptCallbackForRepeating(
        base::BindOnce(&ServiceWorkerRegistrationObjectHostTest::OnMojoError,
                       base::Unretained(this))));
  }

  void TearDown() override {
    mojo::edk::SetDefaultProcessErrorCallback(
        mojo::edk::ProcessErrorCallback());
    ServiceWorkerRegistrationTest::TearDown();
  }

  blink::mojom::ServiceWorkerErrorType CallUpdate(
      blink::mojom::ServiceWorkerRegistrationObjectHost* registration_host) {
    blink::mojom::ServiceWorkerErrorType error =
        blink::mojom::ServiceWorkerErrorType::kUnknown;
    registration_host->Update(base::BindOnce(
        [](blink::mojom::ServiceWorkerErrorType* out_error,
           blink::mojom::ServiceWorkerErrorType error,
           const base::Optional<std::string>& error_msg) {
          *out_error = error;
        },
        &error));
    base::RunLoop().RunUntilIdle();
    return error;
  }

  blink::mojom::ServiceWorkerErrorType CallUnregister(
      blink::mojom::ServiceWorkerRegistrationObjectHost* registration_host) {
    blink::mojom::ServiceWorkerErrorType error =
        blink::mojom::ServiceWorkerErrorType::kUnknown;
    registration_host->Unregister(base::BindOnce(
        [](blink::mojom::ServiceWorkerErrorType* out_error,
           blink::mojom::ServiceWorkerErrorType error,
           const base::Optional<std::string>& error_msg) {
          *out_error = error;
        },
        &error));
    base::RunLoop().RunUntilIdle();
    return error;
  }

  ServiceWorkerStatusCode FindRegistrationInStorage(int64_t registration_id,
                                                    const GURL& scope) {
    ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_MAX_VALUE;
    storage()->FindRegistrationForId(
        registration_id, scope,
        base::AdaptCallbackForRepeating(base::BindOnce(
            [](ServiceWorkerStatusCode* out_status,
               ServiceWorkerStatusCode status,
               scoped_refptr<ServiceWorkerRegistration> registration) {
              *out_status = status;
            },
            &status)));
    return status;
  }

  int64_t SetUpRegistration(const GURL& scope, const GURL& script_url) {
    storage()->LazyInitializeForTest(base::DoNothing());
    base::RunLoop().RunUntilIdle();

    // Prepare ServiceWorkerRegistration.
    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = scope;
    scoped_refptr<ServiceWorkerRegistration> registration =
        base::MakeRefCounted<ServiceWorkerRegistration>(
            options, storage()->NewRegistrationId(), context()->AsWeakPtr());
    // Prepare ServiceWorkerVersion.
    scoped_refptr<ServiceWorkerVersion> version =
        base::MakeRefCounted<ServiceWorkerVersion>(
            registration.get(), script_url, storage()->NewVersionId(),
            context()->AsWeakPtr());
    std::vector<ServiceWorkerDatabase::ResourceRecord> records;
    records.push_back(WriteToDiskCacheSync(
        storage(), version->script_url(), storage()->NewResourceId(),
        {} /* headers */, "I'm the body", "I'm the meta data"));
    version->script_cache_map()->SetResources(records);
    version->SetMainScriptHttpResponseInfo(
        EmbeddedWorkerTestHelper::CreateHttpResponseInfo());
    version->set_fetch_handler_existence(
        ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
    version->SetStatus(ServiceWorkerVersion::INSTALLING);
    // Make the registration findable via storage functions.
    bool called = false;
    ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_MAX_VALUE;
    storage()->StoreRegistration(registration.get(), version.get(),
                                 base::AdaptCallbackForRepeating(base::BindOnce(
                                     &SaveStatusCallback, &called, &status)));
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(SERVICE_WORKER_OK, status);

    return registration->id();
  }

  ServiceWorkerRemoteProviderEndpoint PrepareProviderHost(
      int64_t provider_id,
      const GURL& document_url) {
    ServiceWorkerRemoteProviderEndpoint remote_endpoint;
    std::unique_ptr<ServiceWorkerProviderHost> host =
        CreateProviderHostWithDispatcherHost(
            helper_->mock_render_process_id(), provider_id,
            context()->AsWeakPtr(), 1 /* route_id */, dispatcher_host(),
            &remote_endpoint);
    host->SetDocumentUrl(document_url);
    context()->AddProviderHost(std::move(host));
    return remote_endpoint;
  }

  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr
  GetRegistrationFromRemote(mojom::ServiceWorkerContainerHost* container_host,
                            const GURL& url) {
    blink::mojom::ServiceWorkerRegistrationObjectInfoPtr registration_info;
    container_host->GetRegistration(
        url, base::BindOnce(
                 [](blink::mojom::ServiceWorkerRegistrationObjectInfoPtr*
                        out_registration_info,
                    blink::mojom::ServiceWorkerErrorType error,
                    const base::Optional<std::string>& error_msg,
                    blink::mojom::ServiceWorkerRegistrationObjectInfoPtr
                        registration) {
                   ASSERT_EQ(blink::mojom::ServiceWorkerErrorType::kNone,
                             error);
                   *out_registration_info = std::move(registration);
                 },
                 &registration_info));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(registration_info->host_ptr_info.is_valid());
    return registration_info;
  }

  ServiceWorkerDispatcherHost* dispatcher_host() {
    return helper_->GetDispatcherHostForProcess(
        helper_->mock_render_process_id());
  }

  void OnMojoError(const std::string& error) { bad_messages_.push_back(error); }

  std::vector<std::string> bad_messages_;
};

TEST_F(ServiceWorkerRegistrationObjectHostTest, BreakConnection_Destroy) {
  const GURL kScope("https://www.example.com/");
  const GURL kScriptUrl("https://www.example.com/sw.js");
  int64_t registration_id = SetUpRegistration(kScope, kScriptUrl);
  const int64_t kProviderId = 99;  // Dummy value
  ServiceWorkerRemoteProviderEndpoint remote_endpoint =
      PrepareProviderHost(kProviderId, kScope);
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info =
      GetRegistrationFromRemote(remote_endpoint.host_ptr()->get(), kScope);
  blink::mojom::ServiceWorkerRegistrationObjectHostAssociatedPtr
      registration_host_ptr;
  registration_host_ptr.Bind(std::move(info->host_ptr_info));

  EXPECT_NE(nullptr, context()->GetLiveRegistration(registration_id));
  registration_host_ptr.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(nullptr, context()->GetLiveRegistration(registration_id));
}

TEST_F(ServiceWorkerRegistrationObjectHostTest, Update_Success) {
  const GURL kScope("https://www.example.com/");
  const GURL kScriptUrl("https://www.example.com/sw.js");
  SetUpRegistration(kScope, kScriptUrl);
  const int64_t kProviderId = 99;  // Dummy value
  ServiceWorkerRemoteProviderEndpoint remote_endpoint =
      PrepareProviderHost(kProviderId, kScope);
  blink::mojom::ServiceWorkerRegistrationObjectHostAssociatedPtr
      registration_host_ptr;

  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info =
      GetRegistrationFromRemote(remote_endpoint.host_ptr()->get(), kScope);
  registration_host_ptr.Bind(std::move(info->host_ptr_info));
  // Ignore the messages to the registration object, otherwise the callbacks
  // issued from |registration_host_ptr| may wait for receiving the messages to
  // |info->request|.
  info->request = nullptr;

  EXPECT_EQ(blink::mojom::ServiceWorkerErrorType::kNone,
            CallUpdate(registration_host_ptr.get()));
}

TEST_F(ServiceWorkerRegistrationObjectHostTest, Update_CrossOriginShouldFail) {
  const GURL kScope("https://www.example.com/");
  const GURL kScriptUrl("https://www.example.com/sw.js");
  SetUpRegistration(kScope, kScriptUrl);
  const int64_t kProviderId = 99;  // Dummy value
  ServiceWorkerRemoteProviderEndpoint remote_endpoint =
      PrepareProviderHost(kProviderId, kScope);
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info =
      GetRegistrationFromRemote(remote_endpoint.host_ptr()->get(), kScope);
  blink::mojom::ServiceWorkerRegistrationObjectHostAssociatedPtr
      registration_host_ptr;
  registration_host_ptr.Bind(std::move(info->host_ptr_info));

  ASSERT_TRUE(bad_messages_.empty());
  context()
      ->GetProviderHost(helper_->mock_render_process_id(), kProviderId)
      ->SetDocumentUrl(GURL("https://does.not.exist/"));
  CallUpdate(registration_host_ptr.get());
  EXPECT_EQ(1u, bad_messages_.size());
}

TEST_F(ServiceWorkerRegistrationObjectHostTest,
       Update_ContentSettingsDisallowsServiceWorker) {
  const GURL kScope("https://www.example.com/");
  const GURL kScriptUrl("https://www.example.com/sw.js");
  SetUpRegistration(kScope, kScriptUrl);
  const int64_t kProviderId = 99;  // Dummy value
  ServiceWorkerRemoteProviderEndpoint remote_endpoint =
      PrepareProviderHost(kProviderId, kScope);
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info =
      GetRegistrationFromRemote(remote_endpoint.host_ptr()->get(), kScope);
  blink::mojom::ServiceWorkerRegistrationObjectHostAssociatedPtr
      registration_host_ptr;
  registration_host_ptr.Bind(std::move(info->host_ptr_info));

  ServiceWorkerTestContentBrowserClient test_browser_client;
  ContentBrowserClient* old_browser_client =
      SetBrowserClientForTesting(&test_browser_client);
  EXPECT_EQ(blink::mojom::ServiceWorkerErrorType::kDisabled,
            CallUpdate(registration_host_ptr.get()));
  SetBrowserClientForTesting(old_browser_client);
}

TEST_F(ServiceWorkerRegistrationObjectHostTest, Unregister_Success) {
  const GURL kScope("https://www.example.com/");
  const GURL kScriptUrl("https://www.example.com/sw.js");
  int64_t registration_id = SetUpRegistration(kScope, kScriptUrl);
  const int64_t kProviderId = 99;  // Dummy value
  ServiceWorkerRemoteProviderEndpoint remote_endpoint =
      PrepareProviderHost(kProviderId, kScope);
  blink::mojom::ServiceWorkerRegistrationObjectHostAssociatedPtr
      registration_host_ptr;
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info =
      GetRegistrationFromRemote(remote_endpoint.host_ptr()->get(), kScope);
  registration_host_ptr.Bind(std::move(info->host_ptr_info));
  // Ignore the messages to the registration object and corresponding service
  // worker objects, otherwise the callbacks issued from |registration_host_ptr|
  // may wait for receiving the messages to them.
  info->request = nullptr;
  info->waiting->request = nullptr;

  EXPECT_EQ(SERVICE_WORKER_OK,
            FindRegistrationInStorage(registration_id, kScope));
  EXPECT_EQ(blink::mojom::ServiceWorkerErrorType::kNone,
            CallUnregister(registration_host_ptr.get()));

  EXPECT_EQ(SERVICE_WORKER_ERROR_NOT_FOUND,
            FindRegistrationInStorage(registration_id, kScope));
  EXPECT_EQ(blink::mojom::ServiceWorkerErrorType::kNotFound,
            CallUnregister(registration_host_ptr.get()));
}

TEST_F(ServiceWorkerRegistrationObjectHostTest,
       Unregister_CrossOriginShouldFail) {
  const GURL kScope("https://www.example.com/");
  const GURL kScriptUrl("https://www.example.com/sw.js");
  SetUpRegistration(kScope, kScriptUrl);
  const int64_t kProviderId = 99;  // Dummy value
  ServiceWorkerRemoteProviderEndpoint remote_endpoint =
      PrepareProviderHost(kProviderId, kScope);
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info =
      GetRegistrationFromRemote(remote_endpoint.host_ptr()->get(), kScope);
  blink::mojom::ServiceWorkerRegistrationObjectHostAssociatedPtr
      registration_host_ptr;
  registration_host_ptr.Bind(std::move(info->host_ptr_info));

  ASSERT_TRUE(bad_messages_.empty());
  context()
      ->GetProviderHost(helper_->mock_render_process_id(), kProviderId)
      ->SetDocumentUrl(GURL("https://does.not.exist/"));
  CallUnregister(registration_host_ptr.get());
  EXPECT_EQ(1u, bad_messages_.size());
}

TEST_F(ServiceWorkerRegistrationObjectHostTest,
       Unregister_ContentSettingsDisallowsServiceWorker) {
  const GURL kScope("https://www.example.com/");
  const GURL kScriptUrl("https://www.example.com/sw.js");
  SetUpRegistration(kScope, kScriptUrl);
  const int64_t kProviderId = 99;  // Dummy value
  ServiceWorkerRemoteProviderEndpoint remote_endpoint =
      PrepareProviderHost(kProviderId, kScope);
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info =
      GetRegistrationFromRemote(remote_endpoint.host_ptr()->get(), kScope);
  blink::mojom::ServiceWorkerRegistrationObjectHostAssociatedPtr
      registration_host_ptr;
  registration_host_ptr.Bind(std::move(info->host_ptr_info));

  ServiceWorkerTestContentBrowserClient test_browser_client;
  ContentBrowserClient* old_browser_client =
      SetBrowserClientForTesting(&test_browser_client);
  EXPECT_EQ(blink::mojom::ServiceWorkerErrorType::kDisabled,
            CallUnregister(registration_host_ptr.get()));
  SetBrowserClientForTesting(old_browser_client);
}

TEST_F(ServiceWorkerRegistrationObjectHostTest, SetVersionAttributes) {
  const GURL kScope("https://www.example.com/");
  const GURL kScriptUrl("https://www.example.com/sw.js");
  int64_t registration_id = SetUpRegistration(kScope, kScriptUrl);
  const int64_t kProviderId = 99;  // Dummy value
  ServiceWorkerRemoteProviderEndpoint remote_endpoint =
      PrepareProviderHost(kProviderId, kScope);
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info =
      GetRegistrationFromRemote(remote_endpoint.host_ptr()->get(), kScope);
  EXPECT_EQ(registration_id, info->registration_id);
  EXPECT_TRUE(info->request.is_pending());
  auto mock_registration_object =
      std::make_unique<MockServiceWorkerRegistrationObject>(
          std::move(info->request));

  ServiceWorkerRegistration* registration =
      context()->GetLiveRegistration(registration_id);
  ASSERT_NE(nullptr, registration);
  const int64_t version_1_id = 1L;
  const int64_t version_2_id = 2L;
  scoped_refptr<ServiceWorkerVersion> version_1 =
      base::MakeRefCounted<ServiceWorkerVersion>(
          registration, kScriptUrl, version_1_id, context()->AsWeakPtr());
  scoped_refptr<ServiceWorkerVersion> version_2 =
      base::MakeRefCounted<ServiceWorkerVersion>(
          registration, kScriptUrl, version_2_id, context()->AsWeakPtr());

  // Set an active worker.
  {
    registration->SetActiveVersion(version_1);
    EXPECT_EQ(version_1.get(), registration->active_version());
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(1,
              mock_registration_object->set_version_attributes_called_count());
    ChangedVersionAttributesMask mask(mock_registration_object->changed_mask());
    EXPECT_FALSE(mask.installing_changed());
    EXPECT_FALSE(mock_registration_object->installing());
    EXPECT_FALSE(mask.waiting_changed());
    EXPECT_FALSE(mock_registration_object->waiting());
    EXPECT_TRUE(mask.active_changed());
    EXPECT_TRUE(mock_registration_object->active());
    EXPECT_EQ(version_1_id, mock_registration_object->active()->version_id);
    EXPECT_EQ(kScriptUrl, mock_registration_object->active()->url);
  }

  // Set an installing worker.
  {
    registration->SetInstallingVersion(version_2);
    EXPECT_EQ(version_2.get(), registration->installing_version());
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(2,
              mock_registration_object->set_version_attributes_called_count());
    ChangedVersionAttributesMask mask(mock_registration_object->changed_mask());
    EXPECT_TRUE(mask.installing_changed());
    EXPECT_TRUE(mock_registration_object->installing());
    EXPECT_FALSE(mask.waiting_changed());
    EXPECT_FALSE(mock_registration_object->waiting());
    EXPECT_FALSE(mask.active_changed());
    EXPECT_FALSE(mock_registration_object->active());
    EXPECT_EQ(version_2_id, mock_registration_object->installing()->version_id);
    EXPECT_EQ(kScriptUrl, mock_registration_object->installing()->url);
  }

  // Promote the installing worker to waiting.
  {
    registration->SetWaitingVersion(version_2);
    EXPECT_EQ(version_2.get(), registration->waiting_version());
    EXPECT_FALSE(registration->installing_version());
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(3,
              mock_registration_object->set_version_attributes_called_count());
    ChangedVersionAttributesMask mask(mock_registration_object->changed_mask());
    EXPECT_TRUE(mask.installing_changed());
    EXPECT_FALSE(mock_registration_object->installing());
    EXPECT_TRUE(mask.waiting_changed());
    EXPECT_TRUE(mock_registration_object->waiting());
    EXPECT_FALSE(mask.active_changed());
    EXPECT_FALSE(mock_registration_object->active());
    EXPECT_EQ(version_2_id, mock_registration_object->waiting()->version_id);
    EXPECT_EQ(kScriptUrl, mock_registration_object->waiting()->url);
  }

  // Remove the waiting worker.
  {
    registration->UnsetVersion(version_2.get());
    EXPECT_FALSE(registration->waiting_version());
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(4,
              mock_registration_object->set_version_attributes_called_count());
    ChangedVersionAttributesMask mask(mock_registration_object->changed_mask());
    EXPECT_FALSE(mask.installing_changed());
    EXPECT_FALSE(mock_registration_object->installing());
    EXPECT_TRUE(mask.waiting_changed());
    EXPECT_FALSE(mock_registration_object->waiting());
    EXPECT_FALSE(mask.active_changed());
    EXPECT_FALSE(mock_registration_object->active());
  }
}

TEST_F(ServiceWorkerRegistrationObjectHostTest, SetUpdateViaCache) {
  const GURL kScope("https://www.example.com/");
  const GURL kScriptUrl("https://www.example.com/sw.js");
  int64_t registration_id = SetUpRegistration(kScope, kScriptUrl);
  const int64_t kProviderId = 99;  // Dummy value
  ServiceWorkerRemoteProviderEndpoint remote_endpoint =
      PrepareProviderHost(kProviderId, kScope);
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info =
      GetRegistrationFromRemote(remote_endpoint.host_ptr()->get(), kScope);
  EXPECT_EQ(registration_id, info->registration_id);
  EXPECT_TRUE(info->request.is_pending());
  auto mock_registration_object =
      std::make_unique<MockServiceWorkerRegistrationObject>(
          std::move(info->request));

  ServiceWorkerRegistration* registration =
      context()->GetLiveRegistration(registration_id);
  ASSERT_EQ(blink::mojom::ServiceWorkerUpdateViaCache::kImports,
            registration->update_via_cache());
  ASSERT_EQ(0, mock_registration_object->set_update_via_cache_called_count());
  ASSERT_EQ(blink::mojom::ServiceWorkerUpdateViaCache::kImports,
            mock_registration_object->update_via_cache());

  registration->SetUpdateViaCache(
      blink::mojom::ServiceWorkerUpdateViaCache::kImports);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, mock_registration_object->set_update_via_cache_called_count());
  EXPECT_EQ(blink::mojom::ServiceWorkerUpdateViaCache::kImports,
            mock_registration_object->update_via_cache());

  registration->SetUpdateViaCache(
      blink::mojom::ServiceWorkerUpdateViaCache::kAll);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, mock_registration_object->set_update_via_cache_called_count());
  EXPECT_EQ(blink::mojom::ServiceWorkerUpdateViaCache::kAll,
            mock_registration_object->update_via_cache());

  registration->SetUpdateViaCache(
      blink::mojom::ServiceWorkerUpdateViaCache::kNone);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, mock_registration_object->set_update_via_cache_called_count());
  EXPECT_EQ(blink::mojom::ServiceWorkerUpdateViaCache::kNone,
            mock_registration_object->update_via_cache());

  registration->SetUpdateViaCache(
      blink::mojom::ServiceWorkerUpdateViaCache::kImports);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(3, mock_registration_object->set_update_via_cache_called_count());
  EXPECT_EQ(blink::mojom::ServiceWorkerUpdateViaCache::kImports,
            mock_registration_object->update_via_cache());
}

TEST_F(ServiceWorkerRegistrationObjectHostTest, UpdateFound) {
  const GURL kScope("https://www.example.com/");
  const GURL kScriptUrl("https://www.example.com/sw.js");
  int64_t registration_id = SetUpRegistration(kScope, kScriptUrl);
  const int64_t kProviderId = 99;  // Dummy value
  ServiceWorkerRemoteProviderEndpoint remote_endpoint =
      PrepareProviderHost(kProviderId, kScope);
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info =
      GetRegistrationFromRemote(remote_endpoint.host_ptr()->get(), kScope);
  EXPECT_EQ(registration_id, info->registration_id);
  EXPECT_TRUE(info->request.is_pending());
  auto mock_registration_object =
      std::make_unique<MockServiceWorkerRegistrationObject>(
          std::move(info->request));

  ServiceWorkerRegistration* registration =
      context()->GetLiveRegistration(registration_id);
  ASSERT_NE(nullptr, registration);
  EXPECT_EQ(0, mock_registration_object->update_found_called_count());
  registration->NotifyUpdateFound();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, mock_registration_object->update_found_called_count());
}

}  // namespace service_worker_registration_unittest
}  // namespace content
