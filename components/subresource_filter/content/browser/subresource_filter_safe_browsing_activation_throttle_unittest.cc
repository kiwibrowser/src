// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/subresource_filter_safe_browsing_activation_throttle.h"

#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/message_loop/message_loop_current.h"
#include "base/metrics/field_trial.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/test/test_mock_time_task_runner.h"
#include "build/build_config.h"
#include "components/safe_browsing/db/test_database_manager.h"
#include "components/subresource_filter/content/browser/content_subresource_filter_driver_factory.h"
#include "components/subresource_filter/content/browser/fake_safe_browsing_database_manager.h"
#include "components/subresource_filter/content/browser/subresource_filter_client.h"
#include "components/subresource_filter/content/browser/subresource_filter_observer_test_utils.h"
#include "components/subresource_filter/content/browser/subresource_filter_safe_browsing_client.h"
#include "components/subresource_filter/content/browser/subresource_filter_safe_browsing_client_request.h"
#include "components/subresource_filter/content/browser/verified_ruleset_dealer.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "components/subresource_filter/core/browser/subresource_filter_features_test_support.h"
#include "components/subresource_filter/core/common/activation_decision.h"
#include "components/subresource_filter/core/common/activation_level.h"
#include "components/subresource_filter/core/common/activation_list.h"
#include "components/subresource_filter/core/common/activation_state.h"
#include "components/subresource_filter/core/common/test_ruleset_creator.h"
#include "components/subresource_filter/core/common/test_ruleset_utils.h"
#include "components/ukm/content/source_url_recorder.h"
#include "components/ukm/test_ukm_recorder.h"
#include "components/url_pattern_index/proto/rules.pb.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_navigation_throttle.h"
#include "content/public/test/test_renderer_host.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace subresource_filter {

namespace {

const char kUrlA[] = "https://example_a.com";
const char kUrlB[] = "https://example_b.com";
const char kUrlC[] = "https://example_c.com";

char kURL[] = "http://example.test/";
char kURLWithParams[] = "http://example.test/?v=10";
char kRedirectURL[] = "http://redirect.test/";

const char kSafeBrowsingNavigationDelay[] =
    "SubresourceFilter.PageLoad.SafeBrowsingDelay";
const char kSafeBrowsingNavigationDelayNoSpeculation[] =
    "SubresourceFilter.PageLoad.SafeBrowsingDelay.NoRedirectSpeculation";
const char kSafeBrowsingCheckTime[] =
    "SubresourceFilter.SafeBrowsing.CheckTime";
const char kActivationListHistogram[] =
    "SubresourceFilter.PageLoad.ActivationList";

class MockSubresourceFilterClient : public SubresourceFilterClient {
 public:
  MockSubresourceFilterClient() = default;
  ~MockSubresourceFilterClient() override = default;

  // Mocks have trouble with move-only types passed in the constructor.
  void set_ruleset_dealer(
      std::unique_ptr<VerifiedRulesetDealer::Handle> ruleset_dealer) {
    ruleset_dealer_ = std::move(ruleset_dealer);
  }

  bool OnPageActivationComputed(content::NavigationHandle* handle,
                                bool activated) override {
    DCHECK(handle->IsInMainFrame());
    return whitelisted_hosts_.count(handle->GetURL().host());
  }

  VerifiedRulesetDealer::Handle* GetRulesetDealer() override {
    return ruleset_dealer_.get();
  }

  MOCK_METHOD0(ShowNotification, void());
  MOCK_METHOD0(OnNewNavigationStarted, void());
  MOCK_METHOD0(ForceActivationInCurrentWebContents, bool());

  void WhitelistInCurrentWebContents(const GURL& url) {
    ASSERT_TRUE(url.SchemeIsHTTPOrHTTPS());
    whitelisted_hosts_.insert(url.host());
  }

  void ClearWhitelist() { whitelisted_hosts_.clear(); }

 private:
  std::set<std::string> whitelisted_hosts_;

  std::unique_ptr<VerifiedRulesetDealer::Handle> ruleset_dealer_;

  DISALLOW_COPY_AND_ASSIGN(MockSubresourceFilterClient);
};

std::string GetSuffixForList(const ActivationList& type) {
  switch (type) {
    case ActivationList::SOCIAL_ENG_ADS_INTERSTITIAL:
      return "SocialEngineeringAdsInterstitial";
    case ActivationList::PHISHING_INTERSTITIAL:
      return "PhishingInterstitial";
    case ActivationList::SUBRESOURCE_FILTER:
      return "SubresourceFilterOnly";
    case ActivationList::BETTER_ADS:
      return "BetterAds";
    case ActivationList::NONE:
      return std::string();
  }
  return std::string();
}

struct ActivationListTestData {
  const char* const activation_list;
  ActivationList activation_list_type;
  safe_browsing::SBThreatType threat_type;
  safe_browsing::ThreatPatternType threat_pattern_type;
  safe_browsing::SubresourceFilterMatch match;
};

typedef safe_browsing::SubresourceFilterLevel SBLevel;
typedef safe_browsing::SubresourceFilterType SBType;
const ActivationListTestData kActivationListTestData[] = {
    {kActivationListSocialEngineeringAdsInterstitial,
     ActivationList::SOCIAL_ENG_ADS_INTERSTITIAL,
     safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
     safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS,
     {}},
    {kActivationListPhishingInterstitial,
     ActivationList::PHISHING_INTERSTITIAL,
     safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
     safe_browsing::ThreatPatternType::NONE,
     {}},
    {kActivationListSubresourceFilter,
     ActivationList::SUBRESOURCE_FILTER,
     safe_browsing::SB_THREAT_TYPE_SUBRESOURCE_FILTER,
     safe_browsing::ThreatPatternType::NONE,
     {}},
    {kActivationListSubresourceFilter,
     ActivationList::BETTER_ADS,
     safe_browsing::SB_THREAT_TYPE_SUBRESOURCE_FILTER,
     safe_browsing::ThreatPatternType::NONE,
     {{{SBType::BETTER_ADS, SBLevel::ENFORCE}}, base::KEEP_FIRST_OF_DUPES}},
};

}  //  namespace

class SubresourceFilterSafeBrowsingActivationThrottleTest
    : public content::RenderViewHostTestHarness,
      public content::WebContentsObserver {
 public:
  SubresourceFilterSafeBrowsingActivationThrottleTest()
      : field_trial_list_(nullptr) {}
  ~SubresourceFilterSafeBrowsingActivationThrottleTest() override {}

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    Configure();
    test_io_task_runner_ = new base::TestMockTimeTaskRunner();
    // Note: Using NiceMock to allow uninteresting calls and suppress warnings.
    std::vector<url_pattern_index::proto::UrlRule> rules;
    rules.push_back(testing::CreateSuffixRule("disallowed.html"));
    ASSERT_NO_FATAL_FAILURE(test_ruleset_creator_.CreateRulesetWithRules(
        rules, &test_ruleset_pair_));
    auto ruleset_dealer = std::make_unique<VerifiedRulesetDealer::Handle>(
        base::MessageLoopCurrent::Get()->task_runner());
    ruleset_dealer->TryOpenAndSetRulesetFile(test_ruleset_pair_.indexed.path,
                                             base::DoNothing());
    client_ =
        std::make_unique<::testing::NiceMock<MockSubresourceFilterClient>>();
    client_->set_ruleset_dealer(std::move(ruleset_dealer));
    ContentSubresourceFilterDriverFactory::CreateForWebContents(
        RenderViewHostTestHarness::web_contents(), client_.get());
    fake_safe_browsing_database_ = new FakeSafeBrowsingDatabaseManager();
    NavigateAndCommit(GURL("https://test.com"));
    Observe(RenderViewHostTestHarness::web_contents());

    observer_ = std::make_unique<TestSubresourceFilterObserver>(
        RenderViewHostTestHarness::web_contents());
  }

  virtual void Configure() {
    scoped_configuration_.ResetConfiguration(Configuration(
        ActivationLevel::ENABLED, ActivationScope::ACTIVATION_LIST,
        ActivationList::SUBRESOURCE_FILTER));
  }

  void TearDown() override {
    client_.reset();

    // RunUntilIdle() must be called multiple times to flush any outstanding
    // cross-thread interactions.
    // TODO(csharrison): Clean up test teardown logic.
    RunUntilIdle();
    RunUntilIdle();

    // RunUntilIdle() called once more, to delete the database on the IO thread.
    fake_safe_browsing_database_ = nullptr;
    RunUntilIdle();

    content::RenderViewHostTestHarness::TearDown();
  }

  TestSubresourceFilterObserver* observer() { return observer_.get(); }

  // content::WebContentsObserver:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override {
    if (navigation_handle->IsInMainFrame()) {
      navigation_handle->RegisterThrottleForTesting(
          std::make_unique<SubresourceFilterSafeBrowsingActivationThrottle>(
              navigation_handle, client(), test_io_task_runner_,
              fake_safe_browsing_database_));
    }
    std::vector<std::unique_ptr<content::NavigationThrottle>> throttles;
    auto* factory = ContentSubresourceFilterDriverFactory::FromWebContents(
        navigation_handle->GetWebContents());
    factory->throttle_manager()->MaybeAppendNavigationThrottles(
        navigation_handle, &throttles);
    for (auto& it : throttles) {
      navigation_handle->RegisterThrottleForTesting(std::move(it));
    }
  }

  // Returns the frame host the navigation committed in, or nullptr if it did
  // not succeed.
  content::RenderFrameHost* CreateAndNavigateDisallowedSubframe(
      content::RenderFrameHost* parent) {
    auto* subframe =
        content::RenderFrameHostTester::For(parent)->AppendChild("subframe");
    auto simulator = content::NavigationSimulator::CreateRendererInitiated(
        GURL("https://example.test/disallowed.html"), subframe);
    simulator->Commit();
    return simulator->GetLastThrottleCheckResult().action() ==
                   content::NavigationThrottle::PROCEED
               ? simulator->GetFinalRenderFrameHost()
               : nullptr;
  }

  content::RenderFrameHost* SimulateNavigateAndCommit(
      std::vector<GURL> navigation_chain,
      content::RenderFrameHost* rfh) {
    SimulateStart(navigation_chain.front(), rfh);
    for (auto it = navigation_chain.begin() + 1; it != navigation_chain.end();
         ++it) {
      SimulateRedirectAndExpectProceed(*it);
    }
    SimulateCommitAndExpectProceed();
    return navigation_simulator_->GetFinalRenderFrameHost();
  }

  content::NavigationThrottle::ThrottleCheckResult SimulateStart(
      const GURL& first_url,
      content::RenderFrameHost* rfh) {
    navigation_simulator_ =
        content::NavigationSimulator::CreateRendererInitiated(first_url, rfh);
    navigation_simulator_->Start();
    auto result = navigation_simulator_->GetLastThrottleCheckResult();
    if (result.action() == content::NavigationThrottle::CANCEL)
      navigation_simulator_.reset();
    return result;
  }

  content::NavigationThrottle::ThrottleCheckResult SimulateRedirect(
      const GURL& new_url) {
    navigation_simulator_->Redirect(new_url);
    auto result = navigation_simulator_->GetLastThrottleCheckResult();
    if (result.action() == content::NavigationThrottle::CANCEL)
      navigation_simulator_.reset();
    return result;
  }

  content::NavigationThrottle::ThrottleCheckResult SimulateCommit(
      content::NavigationSimulator* simulator) {
    // Need to post a task to flush the IO thread because calling Commit()
    // blocks until the throttle checks are complete.
    // TODO(csharrison): Consider adding finer grained control to the
    // NavigationSimulator by giving it an option to be driven by a
    // TestMockTimeTaskRunner. Also see https://crbug.com/703346.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&base::TestMockTimeTaskRunner::RunUntilIdle,
                              base::Unretained(test_io_task_runner_.get())));
    simulator->Commit();
    return simulator->GetLastThrottleCheckResult();
  }

  void SimulateStartAndExpectProceed(const GURL& first_url) {
    EXPECT_EQ(content::NavigationThrottle::PROCEED,
              SimulateStart(first_url, main_rfh()));
  }

  void SimulateRedirectAndExpectProceed(const GURL& new_url) {
    EXPECT_EQ(content::NavigationThrottle::PROCEED, SimulateRedirect(new_url));
  }

  void SimulateCommitAndExpectProceed() {
    EXPECT_EQ(content::NavigationThrottle::PROCEED,
              SimulateCommit(navigation_simulator()));
  }

  void ConfigureForMatch(const GURL& url,
                         safe_browsing::SBThreatType pattern_type =
                             safe_browsing::SB_THREAT_TYPE_SUBRESOURCE_FILTER,
                         const safe_browsing::ThreatMetadata& metadata =
                             safe_browsing::ThreatMetadata()) {
    fake_safe_browsing_database_->AddBlacklistedUrl(url, pattern_type,
                                                    metadata);
  }

  FakeSafeBrowsingDatabaseManager* fake_safe_browsing_database() {
    return fake_safe_browsing_database_.get();
  }

  void ClearAllBlacklistedUrls() {
    fake_safe_browsing_database_->RemoveAllBlacklistedUrls();
  }

  void RunUntilIdle() {
    base::RunLoop().RunUntilIdle();
    test_io_task_runner_->RunUntilIdle();
  }

  content::NavigationSimulator* navigation_simulator() {
    return navigation_simulator_.get();
  }

  const base::HistogramTester& tester() const { return tester_; }

  MockSubresourceFilterClient* client() { return client_.get(); }

  base::TestMockTimeTaskRunner* test_io_task_runner() const {
    return test_io_task_runner_.get();
  }

  testing::ScopedSubresourceFilterConfigurator* scoped_configuration() {
    return &scoped_configuration_;
  }

 private:
  base::FieldTrialList field_trial_list_;
  testing::ScopedSubresourceFilterConfigurator scoped_configuration_;
  scoped_refptr<base::TestMockTimeTaskRunner> test_io_task_runner_;

  testing::TestRulesetCreator test_ruleset_creator_;
  testing::TestRulesetPair test_ruleset_pair_;

  std::unique_ptr<content::NavigationSimulator> navigation_simulator_;
  std::unique_ptr<MockSubresourceFilterClient> client_;
  std::unique_ptr<TestSubresourceFilterObserver> observer_;
  scoped_refptr<FakeSafeBrowsingDatabaseManager> fake_safe_browsing_database_;
  base::HistogramTester tester_;

  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterSafeBrowsingActivationThrottleTest);
};

class SubresourceFilterSafeBrowsingActivationThrottleParamTest
    : public SubresourceFilterSafeBrowsingActivationThrottleTest,
      public ::testing::WithParamInterface<ActivationListTestData> {
 public:
  SubresourceFilterSafeBrowsingActivationThrottleParamTest() {}
  ~SubresourceFilterSafeBrowsingActivationThrottleParamTest() override {}

  void Configure() override {
    const ActivationListTestData& test_data = GetParam();
    scoped_configuration()->ResetConfiguration(Configuration(
        ActivationLevel::ENABLED, ActivationScope::ACTIVATION_LIST,
        test_data.activation_list_type));
  }

  void ConfigureForMatchParam(const GURL& url) {
    const ActivationListTestData& test_data = GetParam();
    safe_browsing::ThreatMetadata metadata;
    metadata.threat_pattern_type = test_data.threat_pattern_type;
    metadata.subresource_filter_match = test_data.match;
    ConfigureForMatch(url, test_data.threat_type, metadata);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(
      SubresourceFilterSafeBrowsingActivationThrottleParamTest);
};

class SubresourceFilterSafeBrowsingActivationThrottleTestWithCancelling
    : public SubresourceFilterSafeBrowsingActivationThrottleTest,
      public ::testing::WithParamInterface<
          std::tuple<content::TestNavigationThrottle::ThrottleMethod,
                     content::TestNavigationThrottle::ResultSynchrony>> {
 public:
  SubresourceFilterSafeBrowsingActivationThrottleTestWithCancelling() {
    std::tie(throttle_method_, result_sync_) = GetParam();
  }
  ~SubresourceFilterSafeBrowsingActivationThrottleTestWithCancelling()
      override {}

  void DidStartNavigation(content::NavigationHandle* handle) override {
    auto throttle = std::make_unique<content::TestNavigationThrottle>(handle);
    throttle->SetResponse(throttle_method_, result_sync_,
                          content::NavigationThrottle::CANCEL);
    handle->RegisterThrottleForTesting(std::move(throttle));
    SubresourceFilterSafeBrowsingActivationThrottleTest::DidStartNavigation(
        handle);
  }

  content::TestNavigationThrottle::ThrottleMethod throttle_method() {
    return throttle_method_;
  }

  content::TestNavigationThrottle::ResultSynchrony result_sync() {
    return result_sync_;
  }

 private:
  content::TestNavigationThrottle::ThrottleMethod throttle_method_;
  content::TestNavigationThrottle::ResultSynchrony result_sync_;

  DISALLOW_COPY_AND_ASSIGN(
      SubresourceFilterSafeBrowsingActivationThrottleTestWithCancelling);
};

struct ActivationScopeTestData {
  ActivationDecision expected_activation_decision;
  bool url_matches_activation_list;
  ActivationScope activation_scope;
};

const ActivationScopeTestData kActivationScopeTestData[] = {
    {ActivationDecision::ACTIVATED, false /* url_matches_activation_list */,
     ActivationScope::ALL_SITES},
    {ActivationDecision::ACTIVATED, true /* url_matches_activation_list */,
     ActivationScope::ALL_SITES},
    {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
     true /* url_matches_activation_list */, ActivationScope::NO_SITES},
    {ActivationDecision::ACTIVATED, true /* url_matches_activation_list */,
     ActivationScope::ACTIVATION_LIST},
    {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
     false /* url_matches_activation_list */, ActivationScope::ACTIVATION_LIST},
};

class SubresourceFilterSafeBrowsingActivationThrottleScopeTest
    : public SubresourceFilterSafeBrowsingActivationThrottleTest,
      public ::testing::WithParamInterface<ActivationScopeTestData> {
 public:
  SubresourceFilterSafeBrowsingActivationThrottleScopeTest() {}
  ~SubresourceFilterSafeBrowsingActivationThrottleScopeTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(
      SubresourceFilterSafeBrowsingActivationThrottleScopeTest);
};

TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest, NoConfigs) {
  scoped_configuration()->ResetConfiguration(std::vector<Configuration>());
  SimulateNavigateAndCommit({GURL(kURL)}, main_rfh());
  EXPECT_EQ(ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
            *observer()->GetPageActivationForLastCommittedLoad());
}

TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest,
       MultipleSimultaneousConfigs) {
  Configuration config1(ActivationLevel::DRYRUN, ActivationScope::NO_SITES);
  config1.activation_conditions.priority = 2;

  Configuration config2(ActivationLevel::DISABLED,
                        ActivationScope::ACTIVATION_LIST,
                        ActivationList::SOCIAL_ENG_ADS_INTERSTITIAL);
  config2.activation_conditions.priority = 1;

  Configuration config3(ActivationLevel::ENABLED, ActivationScope::ALL_SITES);
  config3.activation_conditions.priority = 0;

  scoped_configuration()->ResetConfiguration({config1, config2, config3});

  // Should match |config2| and |config3|, the former with the higher priority.
  GURL match_url(kUrlA);
  GURL non_match_url(kUrlB);
  safe_browsing::ThreatMetadata metadata;
  metadata.threat_pattern_type =
      safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS;
  ConfigureForMatch(match_url, safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
                    metadata);
  SimulateNavigateAndCommit({match_url}, main_rfh());
  EXPECT_EQ(ActivationDecision::ACTIVATION_DISABLED,
            *observer()->GetPageActivationForLastCommittedLoad());

  // Should match |config3|.
  SimulateNavigateAndCommit({non_match_url}, main_rfh());
  EXPECT_EQ(ActivationDecision::ACTIVATED,
            *observer()->GetPageActivationForLastCommittedLoad());
}

TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest,
       ActivationLevelDisabled_NoActivation) {
  scoped_configuration()->ResetConfiguration(
      Configuration(ActivationLevel::DISABLED, ActivationScope::ACTIVATION_LIST,
                    ActivationList::SUBRESOURCE_FILTER));
  GURL url(kURL);

  SimulateNavigateAndCommit({url}, main_rfh());
  EXPECT_EQ(ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
            *observer()->GetPageActivationForLastCommittedLoad());

  ConfigureForMatch(url);
  SimulateNavigateAndCommit({url}, main_rfh());
  EXPECT_EQ(ActivationDecision::ACTIVATION_DISABLED,
            *observer()->GetPageActivationForLastCommittedLoad());

  // Whitelisting occurs last, so the decision should still be DISABLED.
  client()->WhitelistInCurrentWebContents(url);
  SimulateNavigateAndCommit({url}, main_rfh());
  EXPECT_EQ(ActivationDecision::ACTIVATION_DISABLED,
            *observer()->GetPageActivationForLastCommittedLoad());
}

TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest,
       AllSiteEnabled_Activates) {
  scoped_configuration()->ResetConfiguration(
      Configuration(ActivationLevel::ENABLED, ActivationScope::ALL_SITES));
  GURL url(kURL);
  SimulateNavigateAndCommit({url}, main_rfh());
  EXPECT_EQ(ActivationDecision::ACTIVATED,
            *observer()->GetPageActivationForLastCommittedLoad());

  ConfigureForMatch(url);
  SimulateNavigateAndCommit({url}, main_rfh());
  EXPECT_EQ(ActivationDecision::ACTIVATED,
            *observer()->GetPageActivationForLastCommittedLoad());

  // Adding performance measurement should keep activation.
  Configuration config_with_perf(
      Configuration(ActivationLevel::ENABLED, ActivationScope::ALL_SITES));
  config_with_perf.activation_options.performance_measurement_rate = 1.0;
  scoped_configuration()->ResetConfiguration(std::move(config_with_perf));
  SimulateNavigateAndCommit({url}, main_rfh());
  EXPECT_EQ(ActivationDecision::ACTIVATED,
            *observer()->GetPageActivationForLastCommittedLoad());
}

TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest,
       NavigationFails_NoActivation) {
  EXPECT_EQ(base::Optional<ActivationDecision>(),
            observer()->GetPageActivationForLastCommittedLoad());
  content::NavigationSimulator::NavigateAndFailFromDocument(
      GURL(kURL), net::ERR_TIMED_OUT, main_rfh());
  EXPECT_EQ(base::Optional<ActivationDecision>(),
            observer()->GetPageActivationForLastCommittedLoad());
}

TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest,
       NotificationVisibility) {
  GURL url(kURL);
  ConfigureForMatch(url);
  EXPECT_CALL(*client(), OnNewNavigationStarted()).Times(1);
  content::RenderFrameHost* rfh = SimulateNavigateAndCommit({url}, main_rfh());

  EXPECT_CALL(*client(), ShowNotification()).Times(1);
  EXPECT_FALSE(CreateAndNavigateDisallowedSubframe(rfh));
}

TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest,
       ActivateForFrameState) {
  const struct {
    ActivationDecision activation_decision;
    ActivationLevel activation_level;
  } kTestCases[] = {
      {ActivationDecision::ACTIVATED, ActivationLevel::DRYRUN},
      {ActivationDecision::ACTIVATED, ActivationLevel::ENABLED},
      {ActivationDecision::ACTIVATION_DISABLED, ActivationLevel::DISABLED},
  };
  for (const auto& test_data : kTestCases) {
    SCOPED_TRACE(::testing::Message()
                 << "activation_decision "
                 << static_cast<int>(test_data.activation_decision)
                 << " activation_level " << test_data.activation_level);
    client()->ClearWhitelist();
    scoped_configuration()->ResetConfiguration(Configuration(
        test_data.activation_level, ActivationScope::ACTIVATION_LIST,
        ActivationList::SOCIAL_ENG_ADS_INTERSTITIAL));
    const GURL url(kURLWithParams);
    safe_browsing::ThreatMetadata metadata;
    metadata.threat_pattern_type =
        safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS;
    ConfigureForMatch(url, safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
                      metadata);
    SimulateNavigateAndCommit({url}, main_rfh());
    EXPECT_EQ(test_data.activation_decision,
              *observer()->GetPageActivationForLastCommittedLoad());

    // Whitelisting is only applied when the page will otherwise activate.
    client()->WhitelistInCurrentWebContents(url);
    ActivationDecision decision =
        test_data.activation_level == ActivationLevel::DISABLED
            ? test_data.activation_decision
            : ActivationDecision::URL_WHITELISTED;
    SimulateNavigateAndCommit({url}, main_rfh());
    EXPECT_EQ(decision, *observer()->GetPageActivationForLastCommittedLoad());
  }
}

TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest, ActivationList) {
  const struct {
    ActivationDecision expected_activation_decision;
    ActivationList activation_list;
    safe_browsing::SBThreatType threat_type;
    safe_browsing::ThreatPatternType threat_type_metadata;
  } kTestCases[] = {
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET, ActivationList::NONE,
       safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
       safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::SOCIAL_ENG_ADS_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
       safe_browsing::ThreatPatternType::NONE},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::SOCIAL_ENG_ADS_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
       safe_browsing::ThreatPatternType::MALWARE_LANDING},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::SOCIAL_ENG_ADS_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
       safe_browsing::ThreatPatternType::MALWARE_DISTRIBUTION},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::PHISHING_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_API_ABUSE,
       safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::PHISHING_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_BLACKLISTED_RESOURCE,
       safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::PHISHING_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_URL_CLIENT_SIDE_MALWARE,
       safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::PHISHING_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_URL_BINARY_MALWARE,
       safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::PHISHING_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_URL_UNWANTED,
       safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::PHISHING_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_URL_MALWARE,
       safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::PHISHING_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_URL_CLIENT_SIDE_PHISHING,
       safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::PHISHING_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_SAFE,
       safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS},
      {ActivationDecision::ACTIVATED, ActivationList::PHISHING_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
       safe_browsing::ThreatPatternType::NONE},
      {ActivationDecision::ACTIVATED,
       ActivationList::SOCIAL_ENG_ADS_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
       safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS},
      {ActivationDecision::ACTIVATED, ActivationList::PHISHING_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
       safe_browsing::ThreatPatternType::SOCIAL_ENGINEERING_ADS},
      {ActivationDecision::ACTIVATED, ActivationList::SUBRESOURCE_FILTER,
       safe_browsing::SB_THREAT_TYPE_SUBRESOURCE_FILTER,
       safe_browsing::ThreatPatternType::NONE},
      {ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
       ActivationList::PHISHING_INTERSTITIAL,
       safe_browsing::SB_THREAT_TYPE_SUBRESOURCE_FILTER,
       safe_browsing::ThreatPatternType::NONE},
  };
  const GURL test_url("https://matched_url.com/");
  for (const auto& test_case : kTestCases) {
    scoped_configuration()->ResetConfiguration(Configuration(
        ActivationLevel::ENABLED, ActivationScope::ACTIVATION_LIST,
        test_case.activation_list));
    ClearAllBlacklistedUrls();
    safe_browsing::ThreatMetadata metadata;
    metadata.threat_pattern_type = test_case.threat_type_metadata;
    ConfigureForMatch(test_url, test_case.threat_type, metadata);
    SimulateNavigateAndCommit({GURL(kUrlA), GURL(kUrlB), GURL(kUrlC), test_url},
                              main_rfh());
    EXPECT_EQ(test_case.expected_activation_decision,
              *observer()->GetPageActivationForLastCommittedLoad());
  }
}

// Regression test for an issue where synchronous failure from the SB database
// caused a double cancel. This is DCHECKed in the fake database.
TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest,
       SynchronousResponse) {
  const GURL url(kURL);
  fake_safe_browsing_database()->set_synchronous_failure();
  SimulateStartAndExpectProceed(url);
  SimulateCommitAndExpectProceed();
  tester().ExpectUniqueSample(kActivationListHistogram, ActivationList::NONE,
                              1);
}

TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest, LogsUkm) {
  ukm::InitializeSourceUrlRecorderForWebContents(
      RenderViewHostTestHarness::web_contents());
  ukm::TestAutoSetUkmRecorder test_ukm_recorder;
  const GURL url(kURL);
  ConfigureForMatch(url);
  SimulateNavigateAndCommit({url}, main_rfh());
  using SubresourceFilter = ukm::builders::SubresourceFilter;
  const auto& entries =
      test_ukm_recorder.GetEntriesByName(SubresourceFilter::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    test_ukm_recorder.ExpectEntrySourceHasUrl(entry, url);
    test_ukm_recorder.ExpectEntryMetric(
        entry, SubresourceFilter::kActivationDecisionName,
        static_cast<int64_t>(ActivationDecision::ACTIVATED));
  }
}

TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest,
       LogsUkmNoActivation) {
  ukm::InitializeSourceUrlRecorderForWebContents(
      RenderViewHostTestHarness::web_contents());
  ukm::TestAutoSetUkmRecorder test_ukm_recorder;
  const GURL url(kURL);
  SimulateNavigateAndCommit({url}, main_rfh());
  using SubresourceFilter = ukm::builders::SubresourceFilter;
  const auto& entries =
      test_ukm_recorder.GetEntriesByName(SubresourceFilter::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    test_ukm_recorder.ExpectEntrySourceHasUrl(entry, url);
    test_ukm_recorder.ExpectEntryMetric(
        entry, SubresourceFilter::kActivationDecisionName,
        static_cast<int64_t>(
            ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET));
  }
}

TEST_F(SubresourceFilterSafeBrowsingActivationThrottleTest, LogsUkmDryRun) {
  scoped_configuration()->ResetConfiguration(
      Configuration(ActivationLevel::DRYRUN, ActivationScope::ALL_SITES));
  ukm::InitializeSourceUrlRecorderForWebContents(
      RenderViewHostTestHarness::web_contents());
  ukm::TestAutoSetUkmRecorder test_ukm_recorder;
  const GURL url(kURL);
  SimulateNavigateAndCommit({url}, main_rfh());
  using SubresourceFilter = ukm::builders::SubresourceFilter;
  const auto& entries =
      test_ukm_recorder.GetEntriesByName(SubresourceFilter::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    test_ukm_recorder.ExpectEntrySourceHasUrl(entry, url);
    test_ukm_recorder.ExpectEntryMetric(
        entry, SubresourceFilter::kActivationDecisionName,
        static_cast<int64_t>(ActivationDecision::ACTIVATED));
    test_ukm_recorder.ExpectEntryMetric(entry, SubresourceFilter::kDryRunName,
                                        true);
  }
}

TEST_P(SubresourceFilterSafeBrowsingActivationThrottleScopeTest,
       ActivateForScopeType) {
  const ActivationScopeTestData& test_data = GetParam();
  scoped_configuration()->ResetConfiguration(
      Configuration(ActivationLevel::ENABLED, test_data.activation_scope,
                    ActivationList::SUBRESOURCE_FILTER));

  const GURL test_url(kURLWithParams);
  if (test_data.url_matches_activation_list)
    ConfigureForMatch(test_url);
  SimulateNavigateAndCommit({test_url}, main_rfh());
  EXPECT_EQ(test_data.expected_activation_decision,
            *observer()->GetPageActivationForLastCommittedLoad());
  if (test_data.url_matches_activation_list) {
    client()->WhitelistInCurrentWebContents(test_url);
    ActivationDecision expected_decision =
        test_data.expected_activation_decision;
    if (expected_decision == ActivationDecision::ACTIVATED)
      expected_decision = ActivationDecision::URL_WHITELISTED;
    SimulateNavigateAndCommit({test_url}, main_rfh());
    EXPECT_EQ(expected_decision,
              *observer()->GetPageActivationForLastCommittedLoad());
  }
};

// Only main frames with http/https schemes should activate.
TEST_P(SubresourceFilterSafeBrowsingActivationThrottleScopeTest,
       ActivateForSupportedUrlScheme) {
  const ActivationScopeTestData& test_data = GetParam();
  scoped_configuration()->ResetConfiguration(
      Configuration(ActivationLevel::ENABLED, test_data.activation_scope,
                    ActivationList::SUBRESOURCE_FILTER));

  // data URLs are also not supported, but not listed here, as it's not possible
  // for a page to redirect to them after https://crbug.com/594215 is fixed.
  const char* unsupported_urls[] = {"ftp://example.com/", "chrome://settings",
                                    "chrome-extension://some-extension",
                                    "file:///var/www/index.html"};
  const char* supported_urls[] = {"http://example.test",
                                  "https://example.test"};
  for (auto* url : unsupported_urls) {
    SCOPED_TRACE(url);
    if (test_data.url_matches_activation_list)
      ConfigureForMatch(GURL(url));
    SimulateNavigateAndCommit({GURL(url)}, main_rfh());
    EXPECT_EQ(ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
              *observer()->GetPageActivationForLastCommittedLoad());
  }

  for (auto* url : supported_urls) {
    SCOPED_TRACE(url);
    if (test_data.url_matches_activation_list)
      ConfigureForMatch(GURL(url));
    SimulateNavigateAndCommit({GURL(url)}, main_rfh());
    EXPECT_EQ(test_data.expected_activation_decision,
              *observer()->GetPageActivationForLastCommittedLoad());
  }
};

TEST_P(SubresourceFilterSafeBrowsingActivationThrottleParamTest,
       ListNotMatched_NoActivation) {
  const GURL url(kURL);
  SimulateStartAndExpectProceed(url);
  SimulateCommitAndExpectProceed();
  EXPECT_EQ(ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
            *observer()->GetPageActivationForLastCommittedLoad());
  tester().ExpectUniqueSample(kActivationListHistogram,
                              static_cast<int>(ActivationList::NONE), 1);

  tester().ExpectTotalCount(kSafeBrowsingNavigationDelay, 1);
  tester().ExpectTotalCount(kSafeBrowsingNavigationDelayNoSpeculation, 1);
  tester().ExpectTotalCount(kSafeBrowsingCheckTime, 1);
}

TEST_P(SubresourceFilterSafeBrowsingActivationThrottleParamTest,
       ListMatched_Activation) {
  const ActivationListTestData& test_data = GetParam();
  const GURL url(kURL);
  ConfigureForMatchParam(url);
  SimulateStartAndExpectProceed(url);
  SimulateCommitAndExpectProceed();
  EXPECT_EQ(ActivationDecision::ACTIVATED,
            *observer()->GetPageActivationForLastCommittedLoad());
  tester().ExpectUniqueSample(kActivationListHistogram,
                              static_cast<int>(test_data.activation_list_type),
                              1);
}

TEST_P(SubresourceFilterSafeBrowsingActivationThrottleParamTest,
       ListNotMatchedAfterRedirect_NoActivation) {
  const GURL url(kURL);
  SimulateStartAndExpectProceed(url);
  SimulateRedirectAndExpectProceed(GURL(kRedirectURL));
  SimulateCommitAndExpectProceed();
  EXPECT_EQ(ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
            *observer()->GetPageActivationForLastCommittedLoad());
  tester().ExpectUniqueSample(kActivationListHistogram,
                              static_cast<int>(ActivationList::NONE), 1);
}

TEST_P(SubresourceFilterSafeBrowsingActivationThrottleParamTest,
       ListMatchedAfterRedirect_Activation) {
  const ActivationListTestData& test_data = GetParam();
  const GURL url(kURL);
  ConfigureForMatchParam(GURL(kRedirectURL));
  SimulateStartAndExpectProceed(url);
  SimulateRedirectAndExpectProceed(GURL(kRedirectURL));
  SimulateCommitAndExpectProceed();
  EXPECT_EQ(ActivationDecision::ACTIVATED,
            *observer()->GetPageActivationForLastCommittedLoad());
  tester().ExpectUniqueSample(kActivationListHistogram,
                              static_cast<int>(test_data.activation_list_type),
                              1);
}

TEST_P(SubresourceFilterSafeBrowsingActivationThrottleParamTest,
       ListNotMatchedAndTimeout_NoActivation) {
  const ActivationListTestData& test_data = GetParam();
  const GURL url(kURL);
  const std::string suffix(GetSuffixForList(test_data.activation_list_type));
  fake_safe_browsing_database()->SimulateTimeout();
  SimulateStartAndExpectProceed(url);

  // Flush the pending tasks on the IO thread, so the delayed task surely gets
  // posted.
  test_io_task_runner()->RunUntilIdle();

  // Expect one delayed task, and fast forward time.
  base::TimeDelta expected_delay =
      SubresourceFilterSafeBrowsingClientRequest::kCheckURLTimeout;
  EXPECT_EQ(expected_delay, test_io_task_runner()->NextPendingTaskDelay());
  test_io_task_runner()->FastForwardBy(expected_delay);
  SimulateCommitAndExpectProceed();
  EXPECT_EQ(ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
            *observer()->GetPageActivationForLastCommittedLoad());
  tester().ExpectTotalCount(kSafeBrowsingNavigationDelay, 1);
  tester().ExpectTotalCount(kSafeBrowsingNavigationDelayNoSpeculation, 1);
  tester().ExpectTotalCount(kSafeBrowsingCheckTime, 1);
}

// Flaky on Win, Chromium and Linux. http://crbug.com/748524
TEST_P(SubresourceFilterSafeBrowsingActivationThrottleParamTest,
       DISABLED_ListMatchedOnStart_NoDelay) {
  const ActivationListTestData& test_data = GetParam();
  const GURL url(kURL);
  ConfigureForMatchParam(url);
  SimulateStartAndExpectProceed(url);

  // Get the database result back before commit.
  RunUntilIdle();

  SimulateCommitAndExpectProceed();
  EXPECT_EQ(ActivationDecision::ACTIVATED,
            *observer()->GetPageActivationForLastCommittedLoad());
  tester().ExpectUniqueSample(kActivationListHistogram,
                              static_cast<int>(test_data.activation_list_type),
                              1);

  tester().ExpectTimeBucketCount(kSafeBrowsingNavigationDelay,
                                 base::TimeDelta::FromMilliseconds(0), 1);
  tester().ExpectTotalCount(kSafeBrowsingNavigationDelayNoSpeculation, 1);
}

// Flaky on Win, Chromium and Linux. http://crbug.com/748524
TEST_P(SubresourceFilterSafeBrowsingActivationThrottleParamTest,
       DISABLED_ListMatchedOnRedirect_NoDelay) {
  const ActivationListTestData& test_data = GetParam();
  const GURL url(kURL);
  const GURL redirect_url(kRedirectURL);
  ConfigureForMatchParam(redirect_url);

  SimulateStartAndExpectProceed(url);
  SimulateRedirectAndExpectProceed(redirect_url);

  // Get the database result back before commit.
  RunUntilIdle();

  SimulateCommitAndExpectProceed();
  EXPECT_EQ(ActivationDecision::ACTIVATED,
            *observer()->GetPageActivationForLastCommittedLoad());
  tester().ExpectUniqueSample(kActivationListHistogram,
                              static_cast<int>(test_data.activation_list_type),
                              1);

  const std::string suffix(GetSuffixForList(test_data.activation_list_type));
  tester().ExpectTimeBucketCount(kSafeBrowsingNavigationDelay,
                                 base::TimeDelta::FromMilliseconds(0), 1);
  tester().ExpectTotalCount(kSafeBrowsingNavigationDelayNoSpeculation, 1);
  tester().ExpectTotalCount(kSafeBrowsingCheckTime, 2);
}

// Disabled due to flaky failures: https://crbug.com/753669.
TEST_P(SubresourceFilterSafeBrowsingActivationThrottleParamTest,
       DISABLED_ListMatchedOnStartWithRedirect_NoActivation) {
  const GURL url(kURL);
  const GURL redirect_url(kRedirectURL);
  ConfigureForMatchParam(url);

  // These two lines also test how the database client reacts to two requests
  // happening one after another.
  SimulateStartAndExpectProceed(url);
  SimulateRedirectAndExpectProceed(redirect_url);

  // Get the database result back before commit.
  RunUntilIdle();

  SimulateCommitAndExpectProceed();
  EXPECT_EQ(ActivationDecision::ACTIVATION_CONDITIONS_NOT_MET,
            *observer()->GetPageActivationForLastCommittedLoad());
  tester().ExpectTimeBucketCount(kSafeBrowsingNavigationDelay,
                                 base::TimeDelta::FromMilliseconds(0), 1);
  tester().ExpectTotalCount(kSafeBrowsingNavigationDelayNoSpeculation, 1);
}

TEST_P(SubresourceFilterSafeBrowsingActivationThrottleTestWithCancelling,
       Cancel) {
  const GURL url(kURL);
  SCOPED_TRACE(::testing::Message() << "ThrottleMethod: " << throttle_method()
                                    << " ResultSynchrony: " << result_sync());
  ConfigureForMatch(url);
  content::NavigationThrottle::ThrottleCheckResult result =
      SimulateStart(url, main_rfh());
  if (throttle_method() ==
      content::TestNavigationThrottle::WILL_START_REQUEST) {
    EXPECT_EQ(content::NavigationThrottle::CANCEL, result);
    tester().ExpectTotalCount(kSafeBrowsingNavigationDelay, 0);
    return;
  }
  EXPECT_EQ(content::NavigationThrottle::PROCEED, result);

  result = SimulateRedirect(GURL(kRedirectURL));
  if (throttle_method() ==
      content::TestNavigationThrottle::WILL_REDIRECT_REQUEST) {
    EXPECT_EQ(content::NavigationThrottle::CANCEL, result);
    tester().ExpectTotalCount(kSafeBrowsingNavigationDelay, 0);
    return;
  }
  EXPECT_EQ(content::NavigationThrottle::PROCEED, result);

  base::RunLoop().RunUntilIdle();

  result = SimulateCommit(navigation_simulator());
  EXPECT_EQ(content::NavigationThrottle::CANCEL, result);
  tester().ExpectTotalCount(kSafeBrowsingNavigationDelay, 0);
}

INSTANTIATE_TEST_CASE_P(
    CancelMethod,
    SubresourceFilterSafeBrowsingActivationThrottleTestWithCancelling,
    ::testing::Combine(
        ::testing::Values(
            content::TestNavigationThrottle::WILL_START_REQUEST,
            content::TestNavigationThrottle::WILL_REDIRECT_REQUEST,
            content::TestNavigationThrottle::WILL_PROCESS_RESPONSE),
        ::testing::Values(content::TestNavigationThrottle::SYNCHRONOUS,
                          content::TestNavigationThrottle::ASYNCHRONOUS)));

INSTANTIATE_TEST_CASE_P(
    ActivationLevelTest,
    SubresourceFilterSafeBrowsingActivationThrottleParamTest,
    ::testing::ValuesIn(kActivationListTestData));

INSTANTIATE_TEST_CASE_P(
    ActivationScopeTest,
    SubresourceFilterSafeBrowsingActivationThrottleScopeTest,
    ::testing::ValuesIn(kActivationScopeTestData));

}  // namespace subresource_filter
