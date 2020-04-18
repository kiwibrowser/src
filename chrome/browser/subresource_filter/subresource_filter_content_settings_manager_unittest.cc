// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/subresource_filter/subresource_filter_content_settings_manager.h"

#include <set>
#include <utility>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/subresource_filter/chrome_subresource_filter_client.h"
#include "chrome/browser/subresource_filter/subresource_filter_profile_context.h"
#include "chrome/browser/subresource_filter/subresource_filter_profile_context_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "components/subresource_filter/core/browser/subresource_filter_features_test_support.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

using subresource_filter::testing::ScopedSubresourceFilterFeatureToggle;
const char kActionsHistogram[] = "SubresourceFilter.Actions";

class SubresourceFilterContentSettingsManagerTest : public testing::Test {
 public:
  SubresourceFilterContentSettingsManagerTest() {}

  void SetUp() override {
    scoped_feature_toggle().ResetSubresourceFilterState(
        base::FeatureList::OVERRIDE_ENABLE_FEATURE,
        "SubresourceFilterExperimentalUI" /* additional_features */);
    settings_manager_ =
        SubresourceFilterProfileContextFactory::GetForProfile(&testing_profile_)
            ->settings_manager();
    settings_manager_->set_should_use_smart_ui_for_testing(true);
    auto test_clock = std::make_unique<base::SimpleTestClock>();
    test_clock_ = test_clock.get();
    settings_manager_->set_clock_for_testing(std::move(test_clock));
    histogram_tester().ExpectTotalCount(kActionsHistogram, 0);
  }

  HostContentSettingsMap* GetSettingsMap() {
    return HostContentSettingsMapFactory::GetForProfile(profile());
  }

  const base::HistogramTester& histogram_tester() { return histogram_tester_; }

  SubresourceFilterContentSettingsManager* settings_manager() {
    return settings_manager_;
  }

  ScopedSubresourceFilterFeatureToggle& scoped_feature_toggle() {
    return scoped_feature_toggle_;
  }

  TestingProfile* profile() { return &testing_profile_; }

  ContentSetting GetContentSettingMatchingUrlWithEmptyPath(const GURL& url) {
    ContentSettingsForOneType host_settings;
    GetSettingsMap()->GetSettingsForOneType(
        ContentSettingsType::CONTENT_SETTINGS_TYPE_ADS, std::string(),
        &host_settings);
    GURL url_with_empty_path = url.GetWithEmptyPath();
    for (const auto& it : host_settings) {
      // Need GURL conversion to get rid of unnecessary default ports.
      if (GURL(it.primary_pattern.ToString()) == url_with_empty_path)
        return it.GetContentSetting();
    }
    return CONTENT_SETTING_DEFAULT;
  }

  base::SimpleTestClock* test_clock() { return test_clock_; }

 private:
  base::ScopedTempDir scoped_dir_;

  content::TestBrowserThreadBundle thread_bundle_;
  ScopedSubresourceFilterFeatureToggle scoped_feature_toggle_;
  base::HistogramTester histogram_tester_;
  TestingProfile testing_profile_;

  // Owned by the testing_profile_.
  SubresourceFilterContentSettingsManager* settings_manager_ = nullptr;

  // Owned by the settings_manager_.
  base::SimpleTestClock* test_clock_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterContentSettingsManagerTest);
};

// It isn't very cheap to initialize the history service. Tests that need it can
// use this harness.
class SubresourceFilterContentSettingsManagerHistoryTest
    : public SubresourceFilterContentSettingsManagerTest {
 public:
  void SetUp() override {
    ASSERT_TRUE(profile()->CreateHistoryService(true /* delete_file */,
                                                false /* no_db */));
    SubresourceFilterContentSettingsManagerTest::SetUp();
  }
};

TEST_F(SubresourceFilterContentSettingsManagerTest, LogDefaultSetting) {
  const char kDefaultContentSetting[] =
      "ContentSettings.DefaultSubresourceFilterSetting";
  // The histogram should be logged at profile creation.
  histogram_tester().ExpectTotalCount(kDefaultContentSetting, 1);
}

TEST_F(SubresourceFilterContentSettingsManagerTest, IrrelevantSetting) {
  GetSettingsMap()->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS,
                                             CONTENT_SETTING_BLOCK);
  histogram_tester().ExpectTotalCount(kActionsHistogram, 0);
}

TEST_F(SubresourceFilterContentSettingsManagerTest, DefaultSetting) {
  // Setting to an existing value should not log any metrics.
  GetSettingsMap()->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_ADS,
                                             CONTENT_SETTING_BLOCK);
  histogram_tester().ExpectTotalCount(kActionsHistogram, 0);

  GetSettingsMap()->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_ADS,
                                             CONTENT_SETTING_ALLOW);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsAllowedGlobal, 1);

  GetSettingsMap()->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_ADS,
                                             CONTENT_SETTING_BLOCK);
  histogram_tester().ExpectTotalCount(kActionsHistogram, 2);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsBlockedGlobal, 1);
}

TEST_F(SubresourceFilterContentSettingsManagerTest, UrlSetting) {
  GURL url("https://www.example.test/");

  GetSettingsMap()->SetContentSettingDefaultScope(
      url, url, CONTENT_SETTINGS_TYPE_ADS, std::string(),
      CONTENT_SETTING_ALLOW);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsAllowed, 1);

  GetSettingsMap()->SetContentSettingDefaultScope(
      url, url, CONTENT_SETTINGS_TYPE_ADS, std::string(),
      CONTENT_SETTING_BLOCK);
  histogram_tester().ExpectTotalCount(kActionsHistogram, 2);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsBlocked, 1);
}

TEST_F(SubresourceFilterContentSettingsManagerTest, WildcardUpdate) {
  ContentSettingsPattern primary_pattern =
      ContentSettingsPattern::FromString("[*.]example.test");
  ContentSettingsPattern secondary_pattern = ContentSettingsPattern::Wildcard();

  GetSettingsMap()->SetContentSettingCustomScope(
      primary_pattern, secondary_pattern, CONTENT_SETTINGS_TYPE_ADS,
      std::string(), CONTENT_SETTING_ALLOW);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsWildcardUpdate, 1);

  GetSettingsMap()->SetContentSettingCustomScope(
      primary_pattern, secondary_pattern, CONTENT_SETTINGS_TYPE_ADS,
      std::string(), CONTENT_SETTING_BLOCK);
  histogram_tester().ExpectTotalCount(kActionsHistogram, 2);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsWildcardUpdate, 2);
}

TEST_F(SubresourceFilterContentSettingsManagerTest, SmartUI) {
  GURL url("https://example.test/");
  GURL url2("https://example.test/path");
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url));
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url2));

  EXPECT_EQ(CONTENT_SETTING_DEFAULT,
            GetContentSettingMatchingUrlWithEmptyPath(url));
  settings_manager()->OnDidShowUI(url);

  // Subsequent same-origin navigations should not show UI.
  EXPECT_FALSE(settings_manager()->ShouldShowUIForSite(url));
  EXPECT_FALSE(settings_manager()->ShouldShowUIForSite(url2));

  // Showing the UI should trigger a forced content setting update, but no
  // metrics should be recorded.
  histogram_tester().ExpectTotalCount(kActionsHistogram, 0);

  // Fast forward the clock.
  test_clock()->Advance(
      SubresourceFilterContentSettingsManager::kDelayBeforeShowingInfobarAgain);
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url));
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url2));
}

TEST_F(SubresourceFilterContentSettingsManagerTest, NoSmartUI) {
  settings_manager()->set_should_use_smart_ui_for_testing(false);

  GURL url("https://example.test/");
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url));

  EXPECT_EQ(CONTENT_SETTING_DEFAULT,
            GetContentSettingMatchingUrlWithEmptyPath(url));
  settings_manager()->OnDidShowUI(url);

  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url));
}

// If the user manually sets a content setting to block the feature, the smart
// UI should be reset.
TEST_F(SubresourceFilterContentSettingsManagerTest,
       SmartUIWithOverride_Resets) {
  GURL url("https://example.test/");
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url));

  settings_manager()->OnDidShowUI(url);

  // Subsequent navigations to same-domains should not show UI.
  EXPECT_FALSE(settings_manager()->ShouldShowUIForSite(url));

  // The user changed their mind, make sure the feature is showing up in the
  // settings UI. i.e. the setting should be non-default.
  EXPECT_EQ(CONTENT_SETTING_BLOCK, settings_manager()->GetSitePermission(url));
  GetSettingsMap()->SetContentSettingDefaultScope(
      url, GURL(), CONTENT_SETTINGS_TYPE_ADS, std::string(),
      CONTENT_SETTING_ALLOW);

  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsAllowed, 1);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsAllowedFromUI, 0);
  histogram_tester().ExpectBucketCount(
      kActionsHistogram, kActionContentSettingsAllowedWhileUISuppressed, 1);
}

TEST_F(SubresourceFilterContentSettingsManagerTest,
       DistinguishMetricsFromUIAndSettingsPage) {
  GURL url("https://example.test/");
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url));

  settings_manager()->OnDidShowUI(url);
  histogram_tester().ExpectTotalCount(kActionsHistogram, 0);

  // Simulate changing the setting via the infobar UI.
  settings_manager()->WhitelistSite(url);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsAllowedFromUI, 1);

  // The standard "Block" histograms are only triggered when blocking from the
  // settings UI, not our standard UI.
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsAllowed, 0);
  histogram_tester().ExpectBucketCount(
      kActionsHistogram, kActionContentSettingsAllowedWhileUISuppressed, 0);

  GURL url2("https://example.test2/");
  GetSettingsMap()->SetContentSettingDefaultScope(
      url2, GURL(), CONTENT_SETTINGS_TYPE_ADS, std::string(),
      CONTENT_SETTING_ALLOW);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsAllowed, 1);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsAllowedFromUI, 1);
}

TEST_F(SubresourceFilterContentSettingsManagerTest,
       IgnoreDuplicateGlobalSettings) {
  histogram_tester().ExpectTotalCount(kActionsHistogram, 0);

  GetSettingsMap()->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_ADS,
                                             CONTENT_SETTING_ALLOW);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsAllowedGlobal, 1);

  GetSettingsMap()->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_ADS,
                                             CONTENT_SETTING_BLOCK);
  histogram_tester().ExpectBucketCount(kActionsHistogram,
                                       kActionContentSettingsBlockedGlobal, 1);
}

TEST_F(SubresourceFilterContentSettingsManagerTest,
       NoExperimentalUI_NoWebsiteSetting) {
  GURL url("https://example.test/");
  {
    base::test::ScopedFeatureList scoped_disabled;
    scoped_disabled.InitAndDisableFeature(
        subresource_filter::kSafeBrowsingSubresourceFilterExperimentalUI);
    settings_manager()->OnDidShowUI(url);
    EXPECT_FALSE(settings_manager()->GetSiteMetadata(url));
  }
  {
    base::test::ScopedFeatureList scoped_enable;
    scoped_enable.InitAndEnableFeature(
        subresource_filter::kSafeBrowsingSubresourceFilterExperimentalUI);
    settings_manager()->OnDidShowUI(url);
    EXPECT_TRUE(settings_manager()->GetSiteMetadata(url));
  }
}

TEST_F(SubresourceFilterContentSettingsManagerTest,
       DefaultSettingsChange_NoWebsiteMetadata) {
  GURL url("https://example.test/");
  EXPECT_FALSE(settings_manager()->GetSiteMetadata(url));

  // Set the setting to the default, should not populate the metadata.
  GetSettingsMap()->SetContentSettingDefaultScope(
      url, GURL(), CONTENT_SETTINGS_TYPE_ADS, std::string(),
      CONTENT_SETTING_DEFAULT);

  EXPECT_FALSE(settings_manager()->GetSiteMetadata(url));
}

TEST_F(SubresourceFilterContentSettingsManagerHistoryTest,
       HistoryUrlDeleted_ClearsWebsiteSetting) {
  // Simulate a history already populated with a URL.
  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile(), ServiceAccessType::EXPLICIT_ACCESS);
  ASSERT_TRUE(history_service);
  history_service->AddPage(GURL("https://already-browsed.com/"),
                           base::Time::Now(), history::SOURCE_BROWSED);

  // Ensure the website setting is set.
  GURL url1("https://example.test/1");
  GURL url2("https://example.test/2");
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url1));
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url2));
  settings_manager()->OnDidShowUI(url1);

  // Simulate adding two page to the history for example.test.
  history_service->AddPage(url1, base::Time::Now(), history::SOURCE_BROWSED);
  history_service->AddPage(url2, base::Time::Now(), history::SOURCE_BROWSED);
  history::BlockUntilHistoryProcessesPendingRequests(history_service);

  EXPECT_FALSE(settings_manager()->ShouldShowUIForSite(url1));
  EXPECT_FALSE(settings_manager()->ShouldShowUIForSite(url2));

  // Deleting a URL from history while there are still other urls for the
  // same origin should not delete the setting.
  history_service->DeleteURL(url1);
  history::BlockUntilHistoryProcessesPendingRequests(history_service);
  EXPECT_FALSE(settings_manager()->ShouldShowUIForSite(url1));
  EXPECT_FALSE(settings_manager()->ShouldShowUIForSite(url2));

  // Deleting all URLs of an origin from history should clear the setting for
  // this URL. Note that since there is another URL in the history this won't
  // clear all items.
  history_service->DeleteURL(url2);
  history::BlockUntilHistoryProcessesPendingRequests(history_service);

  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url1));
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url2));
}

TEST_F(SubresourceFilterContentSettingsManagerHistoryTest,
       AllHistoryUrlDeleted_ClearsWebsiteSetting) {
  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile(), ServiceAccessType::EXPLICIT_ACCESS);
  ASSERT_TRUE(history_service);

  GURL url1("https://example.test");
  GURL url2("https://example.test");
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url1));
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url2));
  settings_manager()->OnDidShowUI(url1);
  settings_manager()->OnDidShowUI(url2);

  // Simulate adding the pages to the history.
  history_service->AddPage(url1, base::Time::Now(), history::SOURCE_BROWSED);
  history_service->AddPage(url2, base::Time::Now(), history::SOURCE_BROWSED);
  history::BlockUntilHistoryProcessesPendingRequests(history_service);

  EXPECT_FALSE(settings_manager()->ShouldShowUIForSite(url1));
  EXPECT_FALSE(settings_manager()->ShouldShowUIForSite(url2));

  // Deleting all the URLs should clear everything.
  base::RunLoop run_loop;
  base::CancelableTaskTracker task_tracker;
  history_service->ExpireHistoryBetween(std::set<GURL>(), base::Time(),
                                        base::Time(), run_loop.QuitClosure(),
                                        &task_tracker);
  run_loop.Run();

  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url1));
  EXPECT_TRUE(settings_manager()->ShouldShowUIForSite(url2));
}

}  // namespace
