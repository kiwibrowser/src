// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/test/histogram_tester.h"
#include "chrome/browser/subresource_filter/chrome_subresource_filter_client.h"
#include "chrome/browser/subresource_filter/subresource_filter_content_settings_manager.h"
#include "chrome/browser/subresource_filter/subresource_filter_test_harness.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/safe_browsing/db/util.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "components/subresource_filter/content/browser/content_activation_list_utils.h"
#include "components/subresource_filter/content/browser/content_subresource_filter_driver_factory.h"
#include "components/subresource_filter/content/browser/fake_safe_browsing_database_manager.h"
#include "components/subresource_filter/content/browser/subresource_filter_observer_test_utils.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "components/subresource_filter/core/browser/subresource_filter_features_test_support.h"
#include "components/subresource_filter/core/common/activation_decision.h"
#include "components/subresource_filter/core/common/activation_level.h"
#include "components/subresource_filter/core/common/activation_list.h"
#include "components/subresource_filter/core/common/activation_scope.h"
#include "components/subresource_filter/core/common/load_policy.h"
#include "content/public/browser/devtools_agent_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class SubresourceFilterTest : public SubresourceFilterTestHarness {};

TEST_F(SubresourceFilterTest, SimpleAllowedLoad) {
  GURL url("https://example.test");
  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_TRUE(CreateAndNavigateDisallowedSubframe(main_rfh()));
  EXPECT_FALSE(GetClient()->did_show_ui_for_navigation());
}

TEST_F(SubresourceFilterTest, SimpleDisallowedLoad) {
  GURL url("https://example.test");
  ConfigureAsSubresourceFilterOnlyURL(url);
  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_FALSE(CreateAndNavigateDisallowedSubframe(main_rfh()));
  EXPECT_TRUE(GetClient()->did_show_ui_for_navigation());
}

TEST_F(SubresourceFilterTest, DeactivateUrl_ClearsSiteMetadata) {
  GURL url("https://a.test");
  ConfigureAsSubresourceFilterOnlyURL(url);
  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_FALSE(CreateAndNavigateDisallowedSubframe(main_rfh()));

  EXPECT_NE(nullptr, GetSettingsManager()->GetSiteMetadata(url));

  RemoveURLFromBlacklist(url);

  // Navigate to |url| again and expect the site metadata to clear.
  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_TRUE(CreateAndNavigateDisallowedSubframe(main_rfh()));

  EXPECT_EQ(nullptr, GetSettingsManager()->GetSiteMetadata(url));
}

// If the underlying configuration changes and a site only activates to DRYRUN,
// we should clear the metadata.
TEST_F(SubresourceFilterTest, ActivationToDryRun_ClearsSiteMetadata) {
  GURL url("https://a.test");
  ConfigureAsSubresourceFilterOnlyURL(url);
  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_FALSE(CreateAndNavigateDisallowedSubframe(main_rfh()));

  EXPECT_NE(nullptr, GetSettingsManager()->GetSiteMetadata(url));

  // If the site later activates as DRYRUN due to e.g. a configuration change,
  // it should also be removed from the metadata.
  scoped_configuration().ResetConfiguration(subresource_filter::Configuration(
      subresource_filter::ActivationLevel::DRYRUN,
      subresource_filter::ActivationScope::ACTIVATION_LIST,
      subresource_filter::ActivationList::SUBRESOURCE_FILTER));

  // Navigate to |url| again and expect the site metadata to clear.
  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_TRUE(CreateAndNavigateDisallowedSubframe(main_rfh()));

  EXPECT_EQ(nullptr, GetSettingsManager()->GetSiteMetadata(url));
}

TEST_F(SubresourceFilterTest, ExplicitWhitelisting_ShouldNotClearMetadata) {
  GURL url("https://a.test");
  ConfigureAsSubresourceFilterOnlyURL(url);
  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_FALSE(CreateAndNavigateDisallowedSubframe(main_rfh()));

  // Simulate explicit whitelisting and reload.
  GetSettingsManager()->WhitelistSite(url);
  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_TRUE(CreateAndNavigateDisallowedSubframe(main_rfh()));

  // Should not have cleared the metadata, since the site is still on the SB
  // blacklist.
  EXPECT_NE(nullptr, GetSettingsManager()->GetSiteMetadata(url));
}

TEST_F(SubresourceFilterTest, SimpleAllowedLoad_WithObserver) {
  GURL url("https://example.test");
  ConfigureAsSubresourceFilterOnlyURL(url);

  subresource_filter::TestSubresourceFilterObserver observer(web_contents());
  SimulateNavigateAndCommit(url, main_rfh());

  EXPECT_EQ(subresource_filter::ActivationDecision::ACTIVATED,
            observer.GetPageActivation(url).value());

  GURL allowed_url("https://example.test/foo");
  auto* subframe =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("subframe");
  SimulateNavigateAndCommit(GURL(allowed_url), subframe);
  EXPECT_EQ(subresource_filter::LoadPolicy::ALLOW,
            *observer.GetSubframeLoadPolicy(allowed_url));
  EXPECT_FALSE(*observer.GetIsAdSubframe(allowed_url));
}

TEST_F(SubresourceFilterTest, SimpleDisallowedLoad_WithObserver) {
  GURL url("https://example.test");
  ConfigureAsSubresourceFilterOnlyURL(url);

  subresource_filter::TestSubresourceFilterObserver observer(web_contents());
  SimulateNavigateAndCommit(url, main_rfh());

  EXPECT_EQ(subresource_filter::ActivationDecision::ACTIVATED,
            observer.GetPageActivation(url).value());

  GURL disallowed_url(SubresourceFilterTest::kDefaultDisallowedUrl);
  EXPECT_FALSE(CreateAndNavigateDisallowedSubframe(main_rfh()));
  EXPECT_EQ(subresource_filter::LoadPolicy::DISALLOW,
            *observer.GetSubframeLoadPolicy(disallowed_url));
  EXPECT_TRUE(*observer.GetIsAdSubframe(disallowed_url));
}

TEST_F(SubresourceFilterTest, RefreshMetadataOnActivation) {
  const GURL url("https://a.test");
  ConfigureAsSubresourceFilterOnlyURL(url);
  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_FALSE(CreateAndNavigateDisallowedSubframe(main_rfh()));
  EXPECT_NE(nullptr, GetSettingsManager()->GetSiteMetadata(url));

  // Whitelist via content settings.
  GetSettingsManager()->WhitelistSite(url);

  // Remove from blacklist, will delete the metadata. Note that there is still
  // an exception in content settings.
  RemoveURLFromBlacklist(url);
  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_EQ(nullptr, GetSettingsManager()->GetSiteMetadata(url));

  // Site re-added to the blacklist. Should not activate due to whitelist, but
  // there should be page info / site details.
  ConfigureAsSubresourceFilterOnlyURL(url);
  SimulateNavigateAndCommit(url, main_rfh());

  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            GetSettingsManager()->GetSitePermission(url));
  EXPECT_NE(nullptr, GetSettingsManager()->GetSiteMetadata(url));
}

TEST_F(SubresourceFilterTest, ToggleForceActivation) {
  base::HistogramTester histogram_tester;
  const char actions_histogram[] = "SubresourceFilter.Actions";
  const GURL url("https://example.test/");

  // Navigate initially, should be no activation.
  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_FALSE(GetClient()->ForceActivationInCurrentWebContents());
  EXPECT_TRUE(CreateAndNavigateDisallowedSubframe(main_rfh()));
  EXPECT_EQ(nullptr, GetSettingsManager()->GetSiteMetadata(url));

  // Simulate opening devtools and forcing activation.
  histogram_tester.ExpectBucketCount(actions_histogram,
                                     kActionForcedActivationEnabled, 0);
  GetClient()->ToggleForceActivationInCurrentWebContents(true);
  histogram_tester.ExpectBucketCount(actions_histogram,
                                     kActionForcedActivationEnabled, 1);
  EXPECT_TRUE(GetClient()->ForceActivationInCurrentWebContents());

  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_FALSE(CreateAndNavigateDisallowedSubframe(main_rfh()));
  EXPECT_FALSE(GetClient()->did_show_ui_for_navigation());
  EXPECT_EQ(nullptr, GetSettingsManager()->GetSiteMetadata(url));
  histogram_tester.ExpectBucketCount(
      "SubresourceFilter.PageLoad.ForcedActivation.DisallowedLoad", true, 1);

  // Simulate closing devtools.
  GetClient()->ToggleForceActivationInCurrentWebContents(false);
  EXPECT_FALSE(GetClient()->ForceActivationInCurrentWebContents());

  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_TRUE(CreateAndNavigateDisallowedSubframe(main_rfh()));
  histogram_tester.ExpectBucketCount(actions_histogram,
                                     kActionForcedActivationEnabled, 1);
}

TEST_F(SubresourceFilterTest, ToggleOffForceActivation_AfterCommit) {
  base::HistogramTester histogram_tester;
  GetClient()->ToggleForceActivationInCurrentWebContents(true);
  const GURL url("https://example.test/");
  SimulateNavigateAndCommit(url, main_rfh());
  GetClient()->ToggleForceActivationInCurrentWebContents(false);

  // Resource should be disallowed, since navigation commit had activation.
  EXPECT_FALSE(CreateAndNavigateDisallowedSubframe(main_rfh()));

  // UI should not have shown though.
  histogram_tester.ExpectBucketCount("SubresourceFilter.Actions",
                                     kActionUIShown, 0);
}

TEST_F(SubresourceFilterTest, NotifySafeBrowsing) {
  typedef safe_browsing::SubresourceFilterType Type;
  typedef safe_browsing::SubresourceFilterLevel Level;
  const struct {
    safe_browsing::SubresourceFilterMatch match;
    subresource_filter::ActivationList expected_activation;
    bool expected_warning;
  } kTestCases[]{
      {{}, subresource_filter::ActivationList::SUBRESOURCE_FILTER, false},
      {{{{Type::ABUSIVE, Level::ENFORCE}}, base::KEEP_FIRST_OF_DUPES},
       subresource_filter::ActivationList::NONE,
       false},
      {{{{Type::ABUSIVE, Level::WARN}}, base::KEEP_FIRST_OF_DUPES},
       subresource_filter::ActivationList::NONE,
       false},
      {{{{Type::BETTER_ADS, Level::ENFORCE}}, base::KEEP_FIRST_OF_DUPES},
       subresource_filter::ActivationList::BETTER_ADS,
       false},
      {{{{Type::BETTER_ADS, Level::WARN}}, base::KEEP_FIRST_OF_DUPES},
       subresource_filter::ActivationList::BETTER_ADS,
       true},
      {{{{Type::BETTER_ADS, Level::ENFORCE}, {Type::ABUSIVE, Level::ENFORCE}},
        base::KEEP_FIRST_OF_DUPES},
       subresource_filter::ActivationList::BETTER_ADS,
       false}};

  const GURL url("https://example.test");
  for (const auto& test_case : kTestCases) {
    subresource_filter::TestSubresourceFilterObserver observer(web_contents());
    auto threat_type =
        safe_browsing::SBThreatType::SB_THREAT_TYPE_SUBRESOURCE_FILTER;
    safe_browsing::ThreatMetadata metadata;
    metadata.subresource_filter_match = test_case.match;
    fake_safe_browsing_database()->AddBlacklistedUrl(url, threat_type,
                                                     metadata);
    SimulateNavigateAndCommit(url, main_rfh());
    bool warning = false;
    EXPECT_EQ(test_case.expected_activation,
              subresource_filter::GetListForThreatTypeAndMetadata(
                  threat_type, metadata, &warning));
  }
}

TEST_F(SubresourceFilterTest, WarningSite_NoMetadata) {
  subresource_filter::Configuration config(
      subresource_filter::ActivationLevel::ENABLED,
      subresource_filter::ActivationScope::ACTIVATION_LIST,
      subresource_filter::ActivationList::BETTER_ADS);
  scoped_configuration().ResetConfiguration(std::move(config));
  const GURL url("https://example.test/");
  safe_browsing::ThreatMetadata metadata;
  metadata.subresource_filter_match
      [safe_browsing::SubresourceFilterType::BETTER_ADS] =
      safe_browsing::SubresourceFilterLevel::WARN;
  auto threat_type =
      safe_browsing::SBThreatType::SB_THREAT_TYPE_SUBRESOURCE_FILTER;
  fake_safe_browsing_database()->AddBlacklistedUrl(url, threat_type, metadata);

  SimulateNavigateAndCommit(url, main_rfh());
  EXPECT_EQ(nullptr, GetSettingsManager()->GetSiteMetadata(url));
}
