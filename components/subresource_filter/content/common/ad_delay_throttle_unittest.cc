// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/common/ad_delay_throttle.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/metrics/field_trial_params.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/subresource_filter/core/common/common_features.h"
#include "content/public/test/throttling_url_loader_test_util.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/test/test_url_loader_client.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace subresource_filter {

// Use a TestURLLoaderFactory with a real ThrottlingURLLoader + the delay
// throttle.
class AdDelayThrottleTest : public testing::Test {
 public:
  AdDelayThrottleTest()
      : scoped_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::MOCK_TIME),
        client_(std::make_unique<network::TestURLLoaderClient>()),
        shared_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &loader_factory_)) {
    scoped_ad_tagging_.InitAndEnableFeature(kAdTagging);
  }
  ~AdDelayThrottleTest() override {}

  void SetUp() override {
    scoped_features_.InitAndEnableFeature(kDelayUnsafeAds);
  }

 protected:
  std::unique_ptr<network::mojom::URLLoaderClient> CreateLoaderAndStart(
      const GURL& url,
      std::unique_ptr<AdDelayThrottle> throttle) {
    network::ResourceRequest request;
    request.url = url;
    std::vector<std::unique_ptr<content::URLLoaderThrottle>> throttles;
    throttles.push_back(std::move(throttle));
    auto loader = content::CreateThrottlingLoaderAndStart(
        shared_factory_, std::move(throttles), 0 /* routing_id */,
        0 /* request_id */, 0 /* options */, &request, client_.get(),
        TRAFFIC_ANNOTATION_FOR_TESTS, base::ThreadTaskRunnerHandle::Get());
    return loader;
  }

  base::TimeDelta GetExpectedDelay(const char* param) const {
    return base::TimeDelta::FromMilliseconds(
        base::GetFieldTrialParamByFeatureAsInt(
            kDelayUnsafeAds, param,
            AdDelayThrottle::kDefaultDelay.InMilliseconds()));
  }

  base::test::ScopedTaskEnvironment scoped_environment_;
  std::unique_ptr<network::TestURLLoaderClient> client_;
  network::TestURLLoaderFactory loader_factory_;

  base::test::ScopedFeatureList scoped_features_;

 private:
  // Use a separate ScopedFeatureList to enable ad tagging, as
  // InitAndEnableFeaturesWithParameters does not support enabling multiple
  // features.
  base::test::ScopedFeatureList scoped_ad_tagging_;
  scoped_refptr<network::WeakWrapperSharedURLLoaderFactory> shared_factory_;

  DISALLOW_COPY_AND_ASSIGN(AdDelayThrottleTest);
};

// Metadata provider that by default, provides metadata indicating that the
// request is an isolated ad.
class MockMetadataProvider : public AdDelayThrottle::MetadataProvider {
 public:
  MockMetadataProvider() {}

  void set_is_ad_request(bool is_ad_request) { is_ad_request_ = is_ad_request; }
  void set_is_non_isolated(bool is_non_isolated) {
    is_non_isolated_ = is_non_isolated;
  }

  // AdDelayThrottle::MetadataProvider:
  bool IsAdRequest() override { return is_ad_request_; }
  bool RequestIsInNonIsolatedSubframe() override { return is_non_isolated_; }

 private:
  bool is_ad_request_ = true;
  bool is_non_isolated_ = false;

  DISALLOW_COPY_AND_ASSIGN(MockMetadataProvider);
};

//  This test harness is identical to the AdDelayThrottleTest but is
//  parameterized by a bool that enables or disables the feature wholesale.
class AdDelayThrottleEnabledParamTest
    : public AdDelayThrottleTest,
      public ::testing::WithParamInterface<bool> {
  void SetUp() override {
    if (GetParam()) {
      scoped_features_.InitAndEnableFeature(kDelayUnsafeAds);
    } else {
      scoped_features_.InitAndDisableFeature(kDelayUnsafeAds);
    }
  }
};

TEST_F(AdDelayThrottleTest, NoFeature_NoDelay) {
  base::test::ScopedFeatureList scoped_disable;
  scoped_disable.InitAndDisableFeature(kDelayUnsafeAds);

  AdDelayThrottle::Factory factory;
  auto throttle = factory.MaybeCreate(std::make_unique<MockMetadataProvider>());
  EXPECT_NE(nullptr, throttle);
  std::string url = "http://example.test/ad.js";
  loader_factory_.AddResponse(url, "var ads = 1;");
  std::unique_ptr<network::mojom::URLLoaderClient> loader_client =
      CreateLoaderAndStart(GURL(url), std::move(throttle));
  scoped_environment_.RunUntilIdle();

  EXPECT_TRUE(client_->has_received_completion());
}

TEST_F(AdDelayThrottleTest, NoAdTagging_NoDelay) {
  base::test::ScopedFeatureList scoped_disable;
  scoped_disable.InitAndDisableFeature(kAdTagging);
  base::test::ScopedFeatureList scoped_params;
  scoped_params.InitAndEnableFeatureWithParameters(
      kDelayUnsafeAds,
      {{kInsecureDelayParam, "50"}, {kNonIsolatedDelayParam, "100"}});

  AdDelayThrottle::Factory factory;
  auto throttle = factory.MaybeCreate(std::make_unique<MockMetadataProvider>());
  EXPECT_NE(nullptr, throttle);
  std::string url = "http://example.test/ad.js";
  loader_factory_.AddResponse(url, "var ads = 1;");
  std::unique_ptr<network::mojom::URLLoaderClient> loader_client =
      CreateLoaderAndStart(GURL(url), std::move(throttle));
  scoped_environment_.RunUntilIdle();

  EXPECT_TRUE(client_->has_received_completion());
  EXPECT_FALSE(base::FieldTrialList::IsTrialActive(
      base::FeatureList::GetFieldTrial(kDelayUnsafeAds)->trial_name()));
}

TEST_F(AdDelayThrottleTest, AdDelay) {
  enum class IsNonIsolated { kYes, kNo };
  enum class IsAd { kYes, kNo };
  enum class WillFinish { kYes, kNo };
  struct TestCase {
    const char* first_url;
    const char* redirect_url;  // nullptr if no redirect.
    IsNonIsolated is_non_isolated;
    IsAd is_ad;
    int fast_forward_ms;
    WillFinish will_finish;
  } kTestCases[] = {
      // Isolated and secure -> no delay.
      {"https://ad.test/", nullptr, IsNonIsolated::kNo, IsAd::kYes, 0,
       WillFinish::kYes},

      // Isolated and insecure -> delays 50ms.
      {"http://ad.test/", nullptr, IsNonIsolated::kNo, IsAd::kYes, 49,
       WillFinish::kNo},
      {"http://ad.test/", nullptr, IsNonIsolated::kNo, IsAd::kYes, 50,
       WillFinish::kYes},
      {"http://ad.test/", nullptr, IsNonIsolated::kNo, IsAd::kNo, 0,
       WillFinish::kYes},

      // Non-isolated and secure -> delays 100ms.
      {"https://ad.test/", nullptr, IsNonIsolated::kYes, IsAd::kYes, 99,
       WillFinish::kNo},
      {"https://ad.test/", nullptr, IsNonIsolated::kYes, IsAd::kYes, 100,
       WillFinish::kYes},
      {"https://ad.test/", nullptr, IsNonIsolated::kYes, IsAd::kNo, 0,
       WillFinish::kYes},

      // Non-isolated and insecure -> delays 150ms.
      {"http://ad.test/", nullptr, IsNonIsolated::kYes, IsAd::kYes, 149,
       WillFinish::kNo},
      {"http://ad.test/", nullptr, IsNonIsolated::kYes, IsAd::kYes, 150,
       WillFinish::kYes},
      {"http://ad.test/", nullptr, IsNonIsolated::kYes, IsAd::kNo, 0,
       WillFinish::kYes},

      // Isolated and insecure redirect -> delays 50ms.
      {"https://ad.test/", "http://ad.test/", IsNonIsolated::kNo, IsAd::kYes,
       49, WillFinish::kNo},
      {"https://ad.test/", "http://ad.test/", IsNonIsolated::kNo, IsAd::kYes,
       50, WillFinish::kYes},
      {"https://ad.test/", "http://ad.test/", IsNonIsolated::kNo, IsAd::kNo, 0,
       WillFinish::kYes},

      // Non-isolated and insecure redirect -> delays 150ms total.
      {"https://ad.test/", "http://ad.test/", IsNonIsolated::kYes, IsAd::kYes,
       149, WillFinish::kNo},
      {"https://ad.test/", "http://ad.test/", IsNonIsolated::kYes, IsAd::kYes,
       150, WillFinish::kYes},
      {"https://ad.test/", "http://ad.test/", IsNonIsolated::kYes, IsAd::kNo, 0,
       WillFinish::kYes},

      // Isolated + insecure with insecure redirect -> delays 50ms only.
      {"http://ad.test/", "http://ad.test/", IsNonIsolated::kNo, IsAd::kYes, 49,
       WillFinish::kNo},
      {"http://ad.test/", "http://ad.test/", IsNonIsolated::kNo, IsAd::kYes, 50,
       WillFinish::kYes},
      {"http://ad.test/", "http://ad.test/", IsNonIsolated::kNo, IsAd::kNo, 0,
       WillFinish::kYes},
  };
  // Initialize delays so insecure and non-isolated are not equal.
  base::test::ScopedFeatureList scoped_params;
  scoped_params.InitAndEnableFeatureWithParameters(
      kDelayUnsafeAds,
      {{kInsecureDelayParam, "50"}, {kNonIsolatedDelayParam, "100"}});
  for (const auto& test : kTestCases) {
    bool is_ad = test.is_ad == IsAd::kYes;
    bool is_non_isolated = test.is_non_isolated == IsNonIsolated::kYes;
    SCOPED_TRACE(testing::Message()
                 << test.first_url << " -> "
                 << (test.redirect_url ? test.redirect_url : "<nullptr>")
                 << " is_ad = " << is_ad
                 << " is_non_isolated = " << is_non_isolated
                 << " fast_forward_ms = " << test.fast_forward_ms);
    AdDelayThrottle::Factory factory;
    client_ = std::make_unique<network::TestURLLoaderClient>();
    auto provider = std::make_unique<MockMetadataProvider>();
    provider->set_is_non_isolated(is_non_isolated);
    provider->set_is_ad_request(is_ad);
    auto throttle = factory.MaybeCreate(std::move(provider));

    ASSERT_TRUE(test.first_url);
    if (test.redirect_url) {
      net::RedirectInfo redirect_info;
      redirect_info.status_code = 301;
      redirect_info.new_url = GURL(test.redirect_url);
      network::TestURLLoaderFactory::Redirects redirects{
          {redirect_info, network::ResourceResponseHead()}};
      loader_factory_.AddResponse(
          GURL(test.first_url), network::ResourceResponseHead(), "foo",
          network::URLLoaderCompletionStatus(), redirects);
    } else {
      loader_factory_.AddResponse(test.first_url, "foo");
    }
    std::unique_ptr<network::mojom::URLLoaderClient> loader_client =
        CreateLoaderAndStart(GURL(test.first_url), std::move(throttle));
    scoped_environment_.RunUntilIdle();
    scoped_environment_.FastForwardBy(
        base::TimeDelta::FromMilliseconds(test.fast_forward_ms));
    EXPECT_EQ(test.will_finish == WillFinish::kYes,
              client_->has_received_completion());
  }
}

TEST_F(AdDelayThrottleTest, DestroyBeforeDelay) {
  AdDelayThrottle::Factory factory;
  auto throttle = factory.MaybeCreate(std::make_unique<MockMetadataProvider>());
  EXPECT_NE(nullptr, throttle);
  std::string url = "http://example.test/ad.js";
  loader_factory_.AddResponse(url, "var ads = 1;");
  std::unique_ptr<network::mojom::URLLoaderClient> loader_client =
      CreateLoaderAndStart(GURL(url), std::move(throttle));

  scoped_environment_.RunUntilIdle();
  EXPECT_FALSE(client_->has_received_completion());

  loader_client.reset();
  scoped_environment_.FastForwardBy(GetExpectedDelay(kInsecureDelayParam));
}

TEST_F(AdDelayThrottleTest, AdDiscoveredAfterRedirect) {
  AdDelayThrottle::Factory factory;
  auto metadata = std::make_unique<MockMetadataProvider>();
  MockMetadataProvider* raw_metadata = metadata.get();
  metadata->set_is_ad_request(false);
  auto throttle = factory.MaybeCreate(std::move(metadata));
  const GURL url("http://example.test/ad.js");

  net::RedirectInfo redirect_info;
  redirect_info.status_code = 301;
  redirect_info.new_url = GURL("http://example2.test/ad.js");
  network::TestURLLoaderFactory::Redirects redirects{
      {redirect_info, network::ResourceResponseHead()}};
  loader_factory_.AddResponse(url, network::ResourceResponseHead(),
                              "var ads = 1;",
                              network::URLLoaderCompletionStatus(), redirects);

  std::unique_ptr<network::mojom::URLLoaderClient> loader_client =
      CreateLoaderAndStart(GURL(url), std::move(throttle));

  raw_metadata->set_is_ad_request(true);
  scoped_environment_.RunUntilIdle();
  EXPECT_FALSE(client_->has_received_completion());
  scoped_environment_.FastForwardBy(GetExpectedDelay(kInsecureDelayParam));
  EXPECT_TRUE(client_->has_received_completion());
}

// Note: the AdDelayThrottle supports MetadataProviders that update IsAdRequest
// bit after redirects, but not all MetadataProviders necessarily support that.
TEST_F(AdDelayThrottleTest, AdDiscoveredAfterSecureRedirect) {
  AdDelayThrottle::Factory factory;
  auto metadata = std::make_unique<MockMetadataProvider>();
  MockMetadataProvider* raw_metadata = metadata.get();
  metadata->set_is_ad_request(false);
  auto throttle = factory.MaybeCreate(std::move(metadata));

  // Request an insecure non-ad request to a secure ad request. Since part of
  // the journey was over an insecure channel, the request should be delayed.
  const GURL url("http://example.test/ad.js");
  net::RedirectInfo redirect_info;
  redirect_info.status_code = 301;
  redirect_info.new_url = GURL("https://example.test/ad.js");
  network::TestURLLoaderFactory::Redirects redirects{
      {redirect_info, network::ResourceResponseHead()}};
  loader_factory_.AddResponse(url, network::ResourceResponseHead(),
                              "var ads = 1;",
                              network::URLLoaderCompletionStatus(), redirects);

  std::unique_ptr<network::mojom::URLLoaderClient> loader_client =
      CreateLoaderAndStart(GURL(url), std::move(throttle));

  raw_metadata->set_is_ad_request(true);
  scoped_environment_.RunUntilIdle();
  EXPECT_FALSE(client_->has_received_completion());
  scoped_environment_.FastForwardBy(GetExpectedDelay(kInsecureDelayParam));
  EXPECT_TRUE(client_->has_received_completion());
}

TEST_F(AdDelayThrottleTest, DelayMetrics) {
  AdDelayThrottle::Factory factory;
  const GURL secure_url("https://example.test/ad.js");
  const GURL insecure_url("http://example.test/ad.js");
  loader_factory_.AddResponse(secure_url.spec(), "foo");
  loader_factory_.AddResponse(insecure_url.spec(), "foo");

  const base::TimeDelta kQueuingDelay = base::TimeDelta::FromMilliseconds(25);

  const char kDelayHistogram[] = "SubresourceFilter.AdDelay.Delay";
  const char kQueuingDelayHistogram[] =
      "SubresourceFilter.AdDelay.Delay.Queuing";
  const char kExpectedDelayHistogram[] =
      "SubresourceFilter.AdDelay.Delay.Expected";
  {
    // Secure isolated ad -> no delay.
    base::HistogramTester histograms;
    {
      auto throttle =
          factory.MaybeCreate(std::make_unique<MockMetadataProvider>());
      throttle->set_tick_clock_for_testing(
          scoped_environment_.GetMockTickClock());
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto loader = CreateLoaderAndStart(secure_url, std::move(throttle));
      scoped_environment_.FastForwardUntilNoTasksRemain();
    }
    histograms.ExpectTotalCount(kDelayHistogram, 0);
    histograms.ExpectTotalCount(kQueuingDelayHistogram, 0);
    histograms.ExpectTotalCount(kExpectedDelayHistogram, 0);
  }
  {
    // Insecure isolated non-ad -> no delay.
    base::HistogramTester histograms;
    {
      auto non_ad_metadata = std::make_unique<MockMetadataProvider>();
      non_ad_metadata->set_is_ad_request(false);
      auto throttle = factory.MaybeCreate(std::move(non_ad_metadata));
      throttle->set_tick_clock_for_testing(
          scoped_environment_.GetMockTickClock());
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto loader = CreateLoaderAndStart(insecure_url, std::move(throttle));
      scoped_environment_.FastForwardUntilNoTasksRemain();
    }
    histograms.ExpectTotalCount(kDelayHistogram, 0);
    histograms.ExpectTotalCount(kQueuingDelayHistogram, 0);
    histograms.ExpectTotalCount(kExpectedDelayHistogram, 0);
  }

  // Use a test clock instead of the scoped task environment's clock because the
  // environment executes tasks as soon as it is able, and we want to simulate
  // jank by advancing time more than the expected delay.
  base::SimpleTestTickClock tick_clock;
  {
    // Insecure isolated ad -> delay.
    base::HistogramTester histograms;
    base::TimeDelta expected_delay = GetExpectedDelay(kInsecureDelayParam);
    {
      auto throttle =
          factory.MaybeCreate(std::make_unique<MockMetadataProvider>());
      throttle->set_tick_clock_for_testing(&tick_clock);
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto loader = CreateLoaderAndStart(insecure_url, std::move(throttle));

      tick_clock.Advance(expected_delay + kQueuingDelay);
      scoped_environment_.FastForwardBy(expected_delay + kQueuingDelay);
    }
    histograms.ExpectUniqueSample(
        kDelayHistogram, (expected_delay + kQueuingDelay).InMilliseconds(), 1);
    histograms.ExpectUniqueSample(kQueuingDelayHistogram,
                                  kQueuingDelay.InMilliseconds(), 1);
    histograms.ExpectUniqueSample(kExpectedDelayHistogram,
                                  expected_delay.InMilliseconds(), 1);
  }
  {
    // Insecure non-isolated ad -> delay.
    base::HistogramTester histograms;
    base::TimeDelta expected_delay = GetExpectedDelay(kInsecureDelayParam) +
                                     GetExpectedDelay(kNonIsolatedDelayParam);
    {
      auto non_isolated_metadata = std::make_unique<MockMetadataProvider>();
      non_isolated_metadata->set_is_non_isolated(true);
      auto throttle = factory.MaybeCreate(std::move(non_isolated_metadata));
      throttle->set_tick_clock_for_testing(&tick_clock);
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto loader = CreateLoaderAndStart(insecure_url, std::move(throttle));

      tick_clock.Advance(expected_delay + kQueuingDelay);
      scoped_environment_.FastForwardBy(expected_delay + kQueuingDelay);
    }
    histograms.ExpectUniqueSample(
        kDelayHistogram, (expected_delay + kQueuingDelay).InMilliseconds(), 1);
    histograms.ExpectUniqueSample(kQueuingDelayHistogram,
                                  kQueuingDelay.InMilliseconds(), 1);
    histograms.ExpectUniqueSample(kExpectedDelayHistogram,
                                  expected_delay.InMilliseconds(), 1);
  }
}

// Make sure metrics are logged when the feature is enabled and disabled.
TEST_P(AdDelayThrottleEnabledParamTest, SecureMetrics) {
  AdDelayThrottle::Factory factory;
  const GURL insecure_url("http://example.test/ad.js");
  const GURL secure_url("https://example.test/ad.js");
  loader_factory_.AddResponse(insecure_url.spec(), "foo");
  loader_factory_.AddResponse(secure_url.spec(), "foo");

  const char kSecureHistogram[] = "Ads.Features.ResourceIsSecure";
  {
    base::HistogramTester histograms;
    {
      auto throttle =
          factory.MaybeCreate(std::make_unique<MockMetadataProvider>());
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto loader = CreateLoaderAndStart(insecure_url, std::move(throttle));
      scoped_environment_.FastForwardUntilNoTasksRemain();
    }
    histograms.ExpectUniqueSample(
        kSecureHistogram,
        static_cast<int>(AdDelayThrottle::SecureInfo::kInsecureAd), 1);
  }
  {
    base::HistogramTester histograms;
    {
      auto throttle =
          factory.MaybeCreate(std::make_unique<MockMetadataProvider>());
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto loader = CreateLoaderAndStart(secure_url, std::move(throttle));
      scoped_environment_.FastForwardUntilNoTasksRemain();
    }
    histograms.ExpectUniqueSample(
        kSecureHistogram,
        static_cast<int>(AdDelayThrottle::SecureInfo::kSecureAd), 1);
  }
  {
    base::HistogramTester histograms;
    {
      auto non_ad_metadata = std::make_unique<MockMetadataProvider>();
      non_ad_metadata->set_is_ad_request(false);
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto throttle = factory.MaybeCreate(std::move(non_ad_metadata));
      auto loader = CreateLoaderAndStart(insecure_url, std::move(throttle));
      scoped_environment_.FastForwardUntilNoTasksRemain();
    }
    histograms.ExpectUniqueSample(
        kSecureHistogram,
        static_cast<int>(AdDelayThrottle::SecureInfo::kInsecureNonAd), 1);
  }
  {
    base::HistogramTester histograms;
    {
      auto non_ad_metadata = std::make_unique<MockMetadataProvider>();
      non_ad_metadata->set_is_ad_request(false);
      auto throttle = factory.MaybeCreate(std::move(non_ad_metadata));
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto loader = CreateLoaderAndStart(secure_url, std::move(throttle));
      scoped_environment_.FastForwardUntilNoTasksRemain();
    }
    histograms.ExpectUniqueSample(
        kSecureHistogram,
        static_cast<int>(AdDelayThrottle::SecureInfo::kSecureNonAd), 1);
  }
}

TEST_P(AdDelayThrottleEnabledParamTest, IsolatedMetrics) {
  AdDelayThrottle::Factory factory;
  const GURL url("https://example.test/ad.js");
  loader_factory_.AddResponse(url.spec(), "foo");

  const char kIsolatedHistogram[] = "Ads.Features.AdResourceIsIsolated";
  {
    base::HistogramTester histograms;
    {
      auto throttle =
          factory.MaybeCreate(std::make_unique<MockMetadataProvider>());
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto loader = CreateLoaderAndStart(url, std::move(throttle));
      scoped_environment_.FastForwardUntilNoTasksRemain();
    }
    histograms.ExpectUniqueSample(
        kIsolatedHistogram,
        static_cast<int>(AdDelayThrottle::IsolatedInfo::kIsolatedAd), 1);
  }
  {
    base::HistogramTester histograms;
    {
      auto metadata = std::make_unique<MockMetadataProvider>();
      metadata->set_is_non_isolated(true);
      auto throttle = factory.MaybeCreate(std::move(metadata));
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto loader = CreateLoaderAndStart(url, std::move(throttle));
      scoped_environment_.FastForwardUntilNoTasksRemain();
    }
    histograms.ExpectUniqueSample(
        kIsolatedHistogram,
        static_cast<int>(AdDelayThrottle::IsolatedInfo::kNonIsolatedAd), 1);
  }
  {
    base::HistogramTester histograms;
    {
      auto non_ad_metadata = std::make_unique<MockMetadataProvider>();
      non_ad_metadata->set_is_ad_request(false);
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto throttle = factory.MaybeCreate(std::move(non_ad_metadata));
      auto loader = CreateLoaderAndStart(url, std::move(throttle));
      scoped_environment_.FastForwardUntilNoTasksRemain();
    }
    histograms.ExpectTotalCount(kIsolatedHistogram, 0);
  }
  {
    base::HistogramTester histograms;
    {
      auto non_ad_metadata = std::make_unique<MockMetadataProvider>();
      non_ad_metadata->set_is_ad_request(false);
      non_ad_metadata->set_is_non_isolated(true);
      auto throttle = factory.MaybeCreate(std::move(non_ad_metadata));
      client_ = std::make_unique<network::TestURLLoaderClient>();
      auto loader = CreateLoaderAndStart(url, std::move(throttle));
      scoped_environment_.FastForwardUntilNoTasksRemain();
    }
    histograms.ExpectTotalCount(kIsolatedHistogram, 0);
  }
}

INSTANTIATE_TEST_CASE_P(,
                        AdDelayThrottleEnabledParamTest,
                        ::testing::Values(true, false));

}  // namespace subresource_filter
