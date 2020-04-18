// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "content/browser/cookie_store/cookie_store_context.h"
#include "content/browser/cookie_store/cookie_store_manager.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/storage_partition_impl.h"
#include "content/common/service_worker/service_worker_event_dispatcher.mojom.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_event_status.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

namespace {

// Synchronous proxies to a wrapped CookieStore service's methods.
class CookieStoreSync {
 public:
  using Subscriptions = std::vector<blink::mojom::CookieChangeSubscriptionPtr>;

  // The caller must ensure that the CookieStore service outlives this.
  explicit CookieStoreSync(blink::mojom::CookieStore* cookie_store_service)
      : cookie_store_service_(cookie_store_service) {}
  ~CookieStoreSync() = default;

  bool AppendSubscriptions(int64_t service_worker_registration_id,
                           Subscriptions subscriptions) {
    bool success;
    base::RunLoop run_loop;
    cookie_store_service_->AppendSubscriptions(
        service_worker_registration_id, std::move(subscriptions),
        base::BindOnce(
            [](base::RunLoop* run_loop, bool* success, bool service_success) {
              *success = service_success;
              run_loop->Quit();
            },
            &run_loop, &success));
    run_loop.Run();
    return success;
  }

  base::Optional<Subscriptions> GetSubscriptions(
      int64_t service_worker_registration_id) {
    base::Optional<Subscriptions> result;
    base::RunLoop run_loop;
    cookie_store_service_->GetSubscriptions(
        service_worker_registration_id,
        base::BindOnce(
            [](base::RunLoop* run_loop, base::Optional<Subscriptions>* result,
               Subscriptions service_result, bool service_success) {
              if (service_success)
                *result = std::move(service_result);
              run_loop->Quit();
            },
            &run_loop, &result));
    run_loop.Run();
    return result;
  }

 private:
  blink::mojom::CookieStore* cookie_store_service_;

  DISALLOW_COPY_AND_ASSIGN(CookieStoreSync);
};

const char kExampleScope[] = "https://example.com/a";
const char kExampleWorkerScript[] = "https://example.com/a/script.js";
const char kGoogleScope[] = "https://google.com/a";
const char kGoogleWorkerScript[] = "https://google.com/a/script.js";

// Mocks a service worker that uses the cookieStore API.
class CookieStoreWorkerTestHelper : public EmbeddedWorkerTestHelper {
 public:
  using EmbeddedWorkerTestHelper::EmbeddedWorkerTestHelper;

  // Sets the cookie change subscriptions requested in the next install event.
  void SetOnInstallSubscriptions(
      std::vector<CookieStoreSync::Subscriptions> subscription_batches,
      blink::mojom::CookieStore* cookie_store_service) {
    install_subscription_batches_ = std::move(subscription_batches);
    cookie_store_service_ = cookie_store_service;
  }

  // Spins inside a run loop until a service worker activate event is received.
  void WaitForActivateEvent() {
    base::RunLoop run_loop;
    quit_on_activate_ = &run_loop;
    run_loop.Run();
  }

  // The data in the CookieChangeEvents received by the worker.
  std::vector<
      std::pair<net::CanonicalCookie, ::network::mojom::CookieChangeCause>>&
  changes() {
    return changes_;
  }

 protected:
  // Collects the worker's registration ID for OnInstallEvent().
  void OnStartWorker(
      int embedded_worker_id,
      int64_t service_worker_version_id,
      const GURL& scope,
      const GURL& script_url,
      bool pause_after_download,
      mojom::ServiceWorkerEventDispatcherRequest dispatcher_request,
      mojom::ControllerServiceWorkerRequest controller_request,
      mojom::EmbeddedWorkerInstanceHostAssociatedPtrInfo instance_host,
      mojom::ServiceWorkerProviderInfoForStartWorkerPtr provider_info,
      blink::mojom::ServiceWorkerInstalledScriptsInfoPtr installed_scripts_info)
      override {
    ServiceWorkerVersion* service_worker_version =
        context()->GetLiveVersion(service_worker_version_id);
    DCHECK(service_worker_version);
    service_worker_registration_id_ = service_worker_version->registration_id();

    EmbeddedWorkerTestHelper::OnStartWorker(
        embedded_worker_id, service_worker_version_id, scope, script_url,
        pause_after_download, std::move(dispatcher_request),
        std::move(controller_request), std::move(instance_host),
        std::move(provider_info), std::move(installed_scripts_info));
  }

  // Cookie change subscriptions can only be created in this event handler.
  void OnInstallEvent(
      mojom::ServiceWorkerEventDispatcher::DispatchInstallEventCallback
          callback) override {
    for (auto& subscriptions : install_subscription_batches_) {
      cookie_store_service_->AppendSubscriptions(
          service_worker_registration_id_, std::move(subscriptions),
          base::BindOnce([](bool success) {
            CHECK(success) << "AppendSubscriptions failed";
          }));
    }
    install_subscription_batches_.clear();

    EmbeddedWorkerTestHelper::OnInstallEvent(std::move(callback));
  }

  // Used to implement WaitForActivateEvent().
  void OnActivateEvent(
      mojom::ServiceWorkerEventDispatcher::DispatchActivateEventCallback
          callback) override {
    if (quit_on_activate_) {
      quit_on_activate_->Quit();
      quit_on_activate_ = nullptr;
    }

    EmbeddedWorkerTestHelper::OnActivateEvent(std::move(callback));
  }

  void OnCookieChangeEvent(
      const net::CanonicalCookie& cookie,
      ::network::mojom::CookieChangeCause cause,
      mojom::ServiceWorkerEventDispatcher::DispatchCookieChangeEventCallback
          callback) override {
    changes_.emplace_back(cookie, cause);
    std::move(callback).Run(blink::mojom::ServiceWorkerEventStatus::COMPLETED,
                            base::Time::Now());
  }

 private:
  // Used to add cookie change subscriptions during OnInstallEvent().
  blink::mojom::CookieStore* cookie_store_service_ = nullptr;
  std::vector<CookieStoreSync::Subscriptions> install_subscription_batches_;
  int64_t service_worker_registration_id_;

  // Set by WaitForActivateEvent(), used in OnActivateEvent().
  base::RunLoop* quit_on_activate_ = nullptr;

  // Collects the changes reported to OnCookieChangeEvent().
  std::vector<
      std::pair<net::CanonicalCookie, ::network::mojom::CookieChangeCause>>
      changes_;
};

}  // namespace

// This class cannot be in an anonymous namespace because it needs to be a
// friend of StoragePartitionImpl, to access its constructor.
class CookieStoreManagerTest
    : public testing::Test,
      public testing::WithParamInterface<bool /* reset_context */> {
 public:
  CookieStoreManagerTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    // Use an on-disk service worker storage to test saving and loading.
    ASSERT_TRUE(user_data_directory_.CreateUniqueTempDir());

    ResetServiceWorkerContext();
  }

  void TearDown() override {
    thread_bundle_.RunUntilIdle();

    // Smart pointers are reset manually in destruction order because this is
    // called by ResetServiceWorkerContext().
    example_service_.reset();
    google_service_.reset();
    example_service_ptr_.reset();
    google_service_ptr_.reset();
    cookie_manager_.reset();
    cookie_store_context_ = nullptr;
    storage_partition_impl_.reset();
    worker_test_helper_.reset();
  }

  void ResetServiceWorkerContext() {
    if (cookie_store_context_)
      TearDown();

    worker_test_helper_ = std::make_unique<CookieStoreWorkerTestHelper>(
        user_data_directory_.GetPath());
    cookie_store_context_ = base::MakeRefCounted<CookieStoreContext>();
    cookie_store_context_->Initialize(worker_test_helper_->context_wrapper(),
                                      base::BindOnce([](bool success) {
                                        CHECK(success) << "Initialize failed";
                                      }));
    storage_partition_impl_ = base::WrapUnique(
        new StoragePartitionImpl(worker_test_helper_->browser_context(),
                                 user_data_directory_.GetPath(), nullptr));
    storage_partition_impl_->SetURLRequestContext(
        worker_test_helper_->browser_context()
            ->CreateRequestContextForStoragePartition(
                user_data_directory_.GetPath(), false, nullptr,
                URLRequestInterceptorScopedVector()));
    ::network::mojom::NetworkContext* network_context =
        storage_partition_impl_->GetNetworkContext();
    cookie_store_context_->ListenToCookieChanges(
        network_context, base::BindOnce([](bool success) {
          CHECK(success) << "ListenToCookieChanges failed";
        }));
    network_context->GetCookieManager(mojo::MakeRequest(&cookie_manager_));

    cookie_store_context_->CreateService(
        mojo::MakeRequest(&example_service_ptr_),
        url::Origin::Create(GURL(kExampleScope)));
    example_service_ =
        std::make_unique<CookieStoreSync>(example_service_ptr_.get());

    cookie_store_context_->CreateService(
        mojo::MakeRequest(&google_service_ptr_),
        url::Origin::Create(GURL(kGoogleScope)));
    google_service_ =
        std::make_unique<CookieStoreSync>(google_service_ptr_.get());
  }

  int64_t RegisterServiceWorker(const char* scope, const char* script_url) {
    bool success = false;
    int64_t registration_id;
    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = GURL(scope);
    base::RunLoop run_loop;
    worker_test_helper_->context()->RegisterServiceWorker(
        GURL(script_url), options,
        base::BindOnce(
            [](base::RunLoop* run_loop, bool* success, int64_t* registration_id,
               ServiceWorkerStatusCode status,
               const std::string& status_message,
               int64_t service_worker_registration_id) {
              *success = (status == SERVICE_WORKER_OK);
              *registration_id = service_worker_registration_id;
              EXPECT_EQ(SERVICE_WORKER_OK, status)
                  << ServiceWorkerStatusToString(status);
              run_loop->Quit();
            },
            &run_loop, &success, &registration_id));
    run_loop.Run();
    if (!success)
      return kInvalidRegistrationId;

    worker_test_helper_->WaitForActivateEvent();
    return registration_id;
  }

  // Simplified helper for SetCanonicalCookie.
  //
  // Creates a CanonicalCookie that is not secure, not http-only,
  // and not restricted to first parties. Returns false if creation fails.
  bool SetSessionCookie(const char* name,
                        const char* value,
                        const char* domain,
                        const char* path) {
    net::CanonicalCookie cookie(
        name, value, domain, path, base::Time(), base::Time(), base::Time(),
        /* secure = */ false,
        /* httponly = */ false, net::CookieSameSite::NO_RESTRICTION,
        net::COOKIE_PRIORITY_DEFAULT);
    base::RunLoop run_loop;
    bool success = false;
    cookie_manager_->SetCanonicalCookie(
        cookie, /* secure_source = */ true, /* can_modify_httponly = */ true,
        base::BindOnce(
            [](base::RunLoop* run_loop, bool* success, bool service_success) {
              *success = success;
              run_loop->Quit();
            },
            &run_loop, &success));
    run_loop.Run();
    return success;
  }

  bool reset_context_during_test() const { return GetParam(); }

  static constexpr const int64_t kInvalidRegistrationId = -1;

 protected:
  TestBrowserThreadBundle thread_bundle_;
  base::ScopedTempDir user_data_directory_;
  std::unique_ptr<CookieStoreWorkerTestHelper> worker_test_helper_;
  std::unique_ptr<StoragePartitionImpl> storage_partition_impl_;
  scoped_refptr<CookieStoreContext> cookie_store_context_;
  ::network::mojom::CookieManagerPtr cookie_manager_;

  blink::mojom::CookieStorePtr example_service_ptr_, google_service_ptr_;
  std::unique_ptr<CookieStoreSync> example_service_, google_service_;
};

const int64_t CookieStoreManagerTest::kInvalidRegistrationId;

namespace {

// Useful for sorting a vector of cookie change subscriptions.
bool CookieChangeSubscriptionLessThan(
    const blink::mojom::CookieChangeSubscriptionPtr& lhs,
    const blink::mojom::CookieChangeSubscriptionPtr& rhs) {
  return std::tie(lhs->name, lhs->match_type, lhs->url) <
         std::tie(rhs->name, rhs->match_type, rhs->url);
}

TEST_P(CookieStoreManagerTest, NoSubscriptions) {
  worker_test_helper_->SetOnInstallSubscriptions(
      std::vector<CookieStoreSync::Subscriptions>(),
      example_service_ptr_.get());
  int64_t registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(registration_id, kInvalidRegistrationId);

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  EXPECT_EQ(0u, all_subscriptions_opt.value().size());
}

TEST_P(CookieStoreManagerTest, EmptySubscriptions) {
  std::vector<CookieStoreSync::Subscriptions> batches;
  batches.emplace_back();
  worker_test_helper_->SetOnInstallSubscriptions(std::move(batches),
                                                 example_service_ptr_.get());
  int64_t registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(registration_id, kInvalidRegistrationId);

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  EXPECT_EQ(0u, all_subscriptions_opt.value().size());
}

TEST_P(CookieStoreManagerTest, OneSubscription) {
  std::vector<CookieStoreSync::Subscriptions> batches;
  batches.emplace_back();

  CookieStoreSync::Subscriptions& subscriptions = batches.back();
  subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
  subscriptions.back()->name = "cookie_name_prefix";
  subscriptions.back()->match_type =
      ::network::mojom::CookieMatchType::STARTS_WITH;
  subscriptions.back()->url = GURL(kExampleScope);

  worker_test_helper_->SetOnInstallSubscriptions(std::move(batches),
                                                 example_service_ptr_.get());
  int64_t registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(registration_id, kInvalidRegistrationId);

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  CookieStoreSync::Subscriptions all_subscriptions =
      std::move(all_subscriptions_opt).value();
  EXPECT_EQ(1u, all_subscriptions.size());
  EXPECT_EQ("cookie_name_prefix", all_subscriptions[0]->name);
  EXPECT_EQ(::network::mojom::CookieMatchType::STARTS_WITH,
            all_subscriptions[0]->match_type);
  EXPECT_EQ(GURL(kExampleScope), all_subscriptions[0]->url);
}

TEST_P(CookieStoreManagerTest, AppendSubscriptionsAfterEmptyInstall) {
  worker_test_helper_->SetOnInstallSubscriptions(
      std::vector<CookieStoreSync::Subscriptions>(),
      example_service_ptr_.get());
  int64_t registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(registration_id, kInvalidRegistrationId);

  CookieStoreSync::Subscriptions subscriptions;
  subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
  subscriptions.back()->name = "cookie_name_prefix";
  subscriptions.back()->match_type =
      ::network::mojom::CookieMatchType::STARTS_WITH;
  subscriptions.back()->url = GURL(kExampleScope);

  EXPECT_FALSE(example_service_->AppendSubscriptions(registration_id,
                                                     std::move(subscriptions)));

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  EXPECT_EQ(0u, all_subscriptions_opt.value().size());
}

TEST_P(CookieStoreManagerTest, AppendSubscriptionsAfterInstall) {
  {
    std::vector<CookieStoreSync::Subscriptions> batches;
    batches.emplace_back();

    CookieStoreSync::Subscriptions& subscriptions = batches.back();
    subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
    subscriptions.back()->name = "cookie_name_prefix";
    subscriptions.back()->match_type =
        ::network::mojom::CookieMatchType::STARTS_WITH;
    subscriptions.back()->url = GURL(kExampleScope);

    worker_test_helper_->SetOnInstallSubscriptions(std::move(batches),
                                                   example_service_ptr_.get());
  }
  int64_t registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(registration_id, kInvalidRegistrationId);

  {
    CookieStoreSync::Subscriptions subscriptions;
    subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
    subscriptions.back()->name = "cookie_name";
    subscriptions.back()->match_type =
        ::network::mojom::CookieMatchType::EQUALS;
    subscriptions.back()->url = GURL(kExampleScope);

    EXPECT_FALSE(example_service_->AppendSubscriptions(
        registration_id, std::move(subscriptions)));
  }

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  CookieStoreSync::Subscriptions all_subscriptions =
      std::move(all_subscriptions_opt).value();
  EXPECT_EQ(1u, all_subscriptions.size());
  EXPECT_EQ("cookie_name_prefix", all_subscriptions[0]->name);
  EXPECT_EQ(::network::mojom::CookieMatchType::STARTS_WITH,
            all_subscriptions[0]->match_type);
  EXPECT_EQ(GURL(kExampleScope), all_subscriptions[0]->url);
}

TEST_P(CookieStoreManagerTest, AppendSubscriptionsFromWrongOrigin) {
  worker_test_helper_->SetOnInstallSubscriptions(
      std::vector<CookieStoreSync::Subscriptions>(),
      example_service_ptr_.get());
  int64_t example_registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(example_registration_id, kInvalidRegistrationId);

  CookieStoreSync::Subscriptions subscriptions;
  subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
  subscriptions.back()->name = "cookie_name_prefix";
  subscriptions.back()->match_type =
      ::network::mojom::CookieMatchType::STARTS_WITH;
  subscriptions.back()->url = GURL(kExampleScope);

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  EXPECT_FALSE(google_service_->AppendSubscriptions(example_registration_id,
                                                    std::move(subscriptions)));

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(example_registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  EXPECT_EQ(0u, all_subscriptions_opt.value().size());
}

TEST_P(CookieStoreManagerTest, AppendSubscriptionsInvalidRegistrationId) {
  worker_test_helper_->SetOnInstallSubscriptions(
      std::vector<CookieStoreSync::Subscriptions>(),
      example_service_ptr_.get());
  int64_t registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(registration_id, kInvalidRegistrationId);

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  CookieStoreSync::Subscriptions subscriptions;
  subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
  subscriptions.back()->name = "cookie_name_prefix";
  subscriptions.back()->match_type =
      ::network::mojom::CookieMatchType::STARTS_WITH;
  subscriptions.back()->url = GURL(kExampleScope);

  EXPECT_FALSE(example_service_->AppendSubscriptions(registration_id + 100,
                                                     std::move(subscriptions)));

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  EXPECT_EQ(0u, all_subscriptions_opt.value().size());
}

TEST_P(CookieStoreManagerTest, MultiWorkerSubscriptions) {
  {
    std::vector<CookieStoreSync::Subscriptions> batches;
    batches.emplace_back();

    CookieStoreSync::Subscriptions& subscriptions = batches.back();
    subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
    subscriptions.back()->name = "cookie_name_prefix";
    subscriptions.back()->match_type =
        ::network::mojom::CookieMatchType::STARTS_WITH;
    subscriptions.back()->url = GURL(kExampleScope);

    worker_test_helper_->SetOnInstallSubscriptions(std::move(batches),
                                                   example_service_ptr_.get());
  }
  int64_t example_registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(example_registration_id, kInvalidRegistrationId);

  {
    std::vector<CookieStoreSync::Subscriptions> batches;
    batches.emplace_back();

    CookieStoreSync::Subscriptions& subscriptions = batches.back();
    subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
    subscriptions.back()->name = "cookie_name";
    subscriptions.back()->match_type =
        ::network::mojom::CookieMatchType::EQUALS;
    subscriptions.back()->url = GURL(kGoogleScope);

    worker_test_helper_->SetOnInstallSubscriptions(std::move(batches),
                                                   google_service_ptr_.get());
  }
  int64_t google_registration_id =
      RegisterServiceWorker(kGoogleScope, kGoogleWorkerScript);
  ASSERT_NE(google_registration_id, kInvalidRegistrationId);
  EXPECT_NE(example_registration_id, google_registration_id);

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  base::Optional<CookieStoreSync::Subscriptions> example_subscriptions_opt =
      example_service_->GetSubscriptions(example_registration_id);
  ASSERT_TRUE(example_subscriptions_opt.has_value());
  CookieStoreSync::Subscriptions example_subscriptions =
      std::move(example_subscriptions_opt).value();
  EXPECT_EQ(1u, example_subscriptions.size());
  EXPECT_EQ("cookie_name_prefix", example_subscriptions[0]->name);
  EXPECT_EQ(::network::mojom::CookieMatchType::STARTS_WITH,
            example_subscriptions[0]->match_type);
  EXPECT_EQ(GURL(kExampleScope), example_subscriptions[0]->url);

  base::Optional<CookieStoreSync::Subscriptions> google_subscriptions_opt =
      google_service_->GetSubscriptions(google_registration_id);
  ASSERT_TRUE(google_subscriptions_opt.has_value());
  CookieStoreSync::Subscriptions google_subscriptions =
      std::move(google_subscriptions_opt).value();
  EXPECT_EQ(1u, google_subscriptions.size());
  EXPECT_EQ("cookie_name", google_subscriptions[0]->name);
  EXPECT_EQ(::network::mojom::CookieMatchType::EQUALS,
            google_subscriptions[0]->match_type);
  EXPECT_EQ(GURL(kGoogleScope), google_subscriptions[0]->url);
}

TEST_P(CookieStoreManagerTest, MultipleSubscriptions) {
  std::vector<CookieStoreSync::Subscriptions> batches;

  {
    batches.emplace_back();
    CookieStoreSync::Subscriptions& subscriptions = batches.back();

    subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
    subscriptions.back()->name = "name1";
    subscriptions.back()->match_type =
        ::network::mojom::CookieMatchType::STARTS_WITH;
    subscriptions.back()->url = GURL("https://example.com/a/1");
    subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
    subscriptions.back()->name = "name2";
    subscriptions.back()->match_type =
        ::network::mojom::CookieMatchType::EQUALS;
    subscriptions.back()->url = GURL("https://example.com/a/2");
  }

  batches.emplace_back();

  {
    batches.emplace_back();
    CookieStoreSync::Subscriptions& subscriptions = batches.back();

    subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
    subscriptions.back()->name = "name3";
    subscriptions.back()->match_type =
        ::network::mojom::CookieMatchType::STARTS_WITH;
    subscriptions.back()->url = GURL("https://example.com/a/3");
  }

  worker_test_helper_->SetOnInstallSubscriptions(std::move(batches),
                                                 example_service_ptr_.get());
  int64_t registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(registration_id, kInvalidRegistrationId);

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  CookieStoreSync::Subscriptions all_subscriptions =
      std::move(all_subscriptions_opt).value();

  std::sort(all_subscriptions.begin(), all_subscriptions.end(),
            CookieChangeSubscriptionLessThan);

  EXPECT_EQ(3u, all_subscriptions.size());
  EXPECT_EQ("name1", all_subscriptions[0]->name);
  EXPECT_EQ(::network::mojom::CookieMatchType::STARTS_WITH,
            all_subscriptions[0]->match_type);
  EXPECT_EQ(GURL("https://example.com/a/1"), all_subscriptions[0]->url);
  EXPECT_EQ("name2", all_subscriptions[1]->name);
  EXPECT_EQ(::network::mojom::CookieMatchType::EQUALS,
            all_subscriptions[1]->match_type);
  EXPECT_EQ(GURL("https://example.com/a/2"), all_subscriptions[1]->url);
  EXPECT_EQ("name3", all_subscriptions[2]->name);
  EXPECT_EQ(::network::mojom::CookieMatchType::STARTS_WITH,
            all_subscriptions[2]->match_type);
  EXPECT_EQ(GURL("https://example.com/a/3"), all_subscriptions[2]->url);
}

TEST_P(CookieStoreManagerTest, OneCookieChange) {
  std::vector<CookieStoreSync::Subscriptions> batches;
  batches.emplace_back();

  CookieStoreSync::Subscriptions& subscriptions = batches.back();
  subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
  subscriptions.back()->name = "";
  subscriptions.back()->match_type =
      ::network::mojom::CookieMatchType::STARTS_WITH;
  subscriptions.back()->url = GURL(kExampleScope);

  worker_test_helper_->SetOnInstallSubscriptions(std::move(batches),
                                                 example_service_ptr_.get());
  int64_t registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(registration_id, kInvalidRegistrationId);

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  ASSERT_EQ(1u, all_subscriptions_opt.value().size());

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  ASSERT_TRUE(
      SetSessionCookie("cookie-name", "cookie-value", "example.com", "/"));
  thread_bundle_.RunUntilIdle();

  ASSERT_EQ(1u, worker_test_helper_->changes().size());
  EXPECT_EQ("cookie-name", worker_test_helper_->changes()[0].first.Name());
  EXPECT_EQ("cookie-value", worker_test_helper_->changes()[0].first.Value());
  EXPECT_EQ("example.com", worker_test_helper_->changes()[0].first.Domain());
  EXPECT_EQ("/", worker_test_helper_->changes()[0].first.Path());
  EXPECT_EQ(::network::mojom::CookieChangeCause::INSERTED,
            worker_test_helper_->changes()[0].second);
}

TEST_P(CookieStoreManagerTest, CookieChangeNameStartsWith) {
  std::vector<CookieStoreSync::Subscriptions> batches;
  batches.emplace_back();

  CookieStoreSync::Subscriptions& subscriptions = batches.back();
  subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
  subscriptions.back()->name = "cookie-name-2";
  subscriptions.back()->match_type =
      ::network::mojom::CookieMatchType::STARTS_WITH;
  subscriptions.back()->url = GURL(kExampleScope);

  worker_test_helper_->SetOnInstallSubscriptions(std::move(batches),
                                                 example_service_ptr_.get());
  int64_t registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(registration_id, kInvalidRegistrationId);

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  ASSERT_EQ(1u, all_subscriptions_opt.value().size());

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  ASSERT_TRUE(
      SetSessionCookie("cookie-name-1", "cookie-value-1", "example.com", "/"));
  thread_bundle_.RunUntilIdle();
  EXPECT_EQ(0u, worker_test_helper_->changes().size());

  worker_test_helper_->changes().clear();
  ASSERT_TRUE(
      SetSessionCookie("cookie-name-2", "cookie-value-2", "example.com", "/"));
  thread_bundle_.RunUntilIdle();

  ASSERT_EQ(1u, worker_test_helper_->changes().size());
  EXPECT_EQ("cookie-name-2", worker_test_helper_->changes()[0].first.Name());
  EXPECT_EQ("cookie-value-2", worker_test_helper_->changes()[0].first.Value());
  EXPECT_EQ("example.com", worker_test_helper_->changes()[0].first.Domain());
  EXPECT_EQ("/", worker_test_helper_->changes()[0].first.Path());
  EXPECT_EQ(::network::mojom::CookieChangeCause::INSERTED,
            worker_test_helper_->changes()[0].second);

  worker_test_helper_->changes().clear();
  ASSERT_TRUE(SetSessionCookie("cookie-name-22", "cookie-value-22",
                               "example.com", "/"));
  thread_bundle_.RunUntilIdle();

  ASSERT_EQ(1u, worker_test_helper_->changes().size());
  EXPECT_EQ("cookie-name-22", worker_test_helper_->changes()[0].first.Name());
  EXPECT_EQ("cookie-value-22", worker_test_helper_->changes()[0].first.Value());
  EXPECT_EQ("example.com", worker_test_helper_->changes()[0].first.Domain());
  EXPECT_EQ("/", worker_test_helper_->changes()[0].first.Path());
  EXPECT_EQ(::network::mojom::CookieChangeCause::INSERTED,
            worker_test_helper_->changes()[0].second);
}

TEST_P(CookieStoreManagerTest, CookieChangeUrl) {
  std::vector<CookieStoreSync::Subscriptions> batches;
  batches.emplace_back();

  CookieStoreSync::Subscriptions& subscriptions = batches.back();
  subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
  subscriptions.back()->name = "";
  subscriptions.back()->match_type =
      ::network::mojom::CookieMatchType::STARTS_WITH;
  subscriptions.back()->url = GURL(kExampleScope);

  worker_test_helper_->SetOnInstallSubscriptions(std::move(batches),
                                                 example_service_ptr_.get());
  int64_t registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(registration_id, kInvalidRegistrationId);

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  ASSERT_EQ(1u, all_subscriptions_opt.value().size());

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  ASSERT_TRUE(
      SetSessionCookie("cookie-name-1", "cookie-value-1", "google.com", "/"));
  thread_bundle_.RunUntilIdle();
  ASSERT_EQ(0u, worker_test_helper_->changes().size());

  worker_test_helper_->changes().clear();
  ASSERT_TRUE(SetSessionCookie("cookie-name-2", "cookie-value-2", "example.com",
                               "/a/subpath"));
  thread_bundle_.RunUntilIdle();
  EXPECT_EQ(0u, worker_test_helper_->changes().size());

  worker_test_helper_->changes().clear();
  ASSERT_TRUE(
      SetSessionCookie("cookie-name-3", "cookie-value-3", "example.com", "/"));
  thread_bundle_.RunUntilIdle();

  ASSERT_EQ(1u, worker_test_helper_->changes().size());
  EXPECT_EQ("cookie-name-3", worker_test_helper_->changes()[0].first.Name());
  EXPECT_EQ("cookie-value-3", worker_test_helper_->changes()[0].first.Value());
  EXPECT_EQ("example.com", worker_test_helper_->changes()[0].first.Domain());
  EXPECT_EQ("/", worker_test_helper_->changes()[0].first.Path());
  EXPECT_EQ(::network::mojom::CookieChangeCause::INSERTED,
            worker_test_helper_->changes()[0].second);

  worker_test_helper_->changes().clear();
  ASSERT_TRUE(
      SetSessionCookie("cookie-name-4", "cookie-value-4", "example.com", "/a"));
  thread_bundle_.RunUntilIdle();

  ASSERT_EQ(1u, worker_test_helper_->changes().size());
  EXPECT_EQ("cookie-name-4", worker_test_helper_->changes()[0].first.Name());
  EXPECT_EQ("cookie-value-4", worker_test_helper_->changes()[0].first.Value());
  EXPECT_EQ("example.com", worker_test_helper_->changes()[0].first.Domain());
  EXPECT_EQ("/a", worker_test_helper_->changes()[0].first.Path());
  EXPECT_EQ(::network::mojom::CookieChangeCause::INSERTED,
            worker_test_helper_->changes()[0].second);
}

TEST_P(CookieStoreManagerTest, GetSubscriptionsFromWrongOrigin) {
  std::vector<CookieStoreSync::Subscriptions> batches;
  batches.emplace_back();

  CookieStoreSync::Subscriptions& subscriptions = batches.back();
  subscriptions.emplace_back(blink::mojom::CookieChangeSubscription::New());
  subscriptions.back()->name = "cookie_name_prefix";
  subscriptions.back()->match_type =
      ::network::mojom::CookieMatchType::STARTS_WITH;
  subscriptions.back()->url = GURL(kExampleScope);

  worker_test_helper_->SetOnInstallSubscriptions(std::move(batches),
                                                 example_service_ptr_.get());
  int64_t example_registration_id =
      RegisterServiceWorker(kExampleScope, kExampleWorkerScript);
  ASSERT_NE(example_registration_id, kInvalidRegistrationId);

  if (reset_context_during_test())
    ResetServiceWorkerContext();

  base::Optional<CookieStoreSync::Subscriptions> all_subscriptions_opt =
      example_service_->GetSubscriptions(example_registration_id);
  ASSERT_TRUE(all_subscriptions_opt.has_value());
  EXPECT_EQ(1u, all_subscriptions_opt.value().size());

  base::Optional<CookieStoreSync::Subscriptions> wrong_subscriptions_opt =
      google_service_->GetSubscriptions(example_registration_id);
  EXPECT_FALSE(wrong_subscriptions_opt.has_value());
}

INSTANTIATE_TEST_CASE_P(CookieStoreManagerTest,
                        CookieStoreManagerTest,
                        testing::Bool() /* reset_storage_during_test */);

}  // namespace

}  // namespace content
