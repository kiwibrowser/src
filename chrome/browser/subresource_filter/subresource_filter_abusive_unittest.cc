// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/stl_util.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/subresource_filter/subresource_filter_content_settings_manager.h"
#include "chrome/browser/subresource_filter/subresource_filter_test_harness.h"
#include "chrome/browser/ui/blocked_content/popup_blocker_tab_helper.h"
#include "chrome/browser/ui/blocked_content/safe_browsing_triggered_popup_blocker.h"
#include "components/safe_browsing/db/util.h"
#include "components/subresource_filter/content/browser/content_subresource_filter_driver_factory.h"
#include "components/subresource_filter/content/browser/fake_safe_browsing_database_manager.h"
#include "components/subresource_filter/core/browser/subresource_filter_constants.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "components/subresource_filter/core/browser/subresource_filter_features_test_support.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"

using safe_browsing::SubresourceFilterLevel;

namespace {

enum MetadataLevel {
  METADATA_WARN = 0,
  METADATA_ENFORCE = 1,
  METADATA_NONE = 2
};

safe_browsing::SubresourceFilterMatch GetMatch(MetadataLevel abusive_level,
                                               MetadataLevel bas_level) {
  auto to_sb_level = [](MetadataLevel l) {
    DCHECK_NE(METADATA_NONE, l);
    return l == METADATA_WARN ? SubresourceFilterLevel::WARN
                              : SubresourceFilterLevel::ENFORCE;
  };
  safe_browsing::SubresourceFilterMatch match;
  if (abusive_level != METADATA_NONE) {
    match[safe_browsing::SubresourceFilterType::ABUSIVE] =
        to_sb_level(abusive_level);
  }
  if (bas_level != METADATA_NONE) {
    match[safe_browsing::SubresourceFilterType::BETTER_ADS] =
        to_sb_level(bas_level);
  }
  return match;
}

}  // namespace

// (Abusive level, BAS level)
using MetadataInfo = std::tuple<MetadataLevel, MetadataLevel>;

// This class tests the interaction between subresource_filter enforcement and
// abusive enforcement.
class SubresourceFilterAbusiveTest
    : public SubresourceFilterTestHarness,
      public ::testing::WithParamInterface<MetadataInfo> {
 public:
  SubresourceFilterAbusiveTest() {
    std::tie(abusive_level_, bas_level_) = GetParam();
  }
  ~SubresourceFilterAbusiveTest() override {}

  // SubresourceFilterTestHarness:
  void SetUp() override {
    SubresourceFilterTestHarness::SetUp();
    scoped_features_.InitAndEnableFeature(kAbusiveExperienceEnforce);
    std::vector<subresource_filter::Configuration> configs{
        subresource_filter::Configuration::
            MakePresetForLiveRunOnPhishingSites(),
        subresource_filter::Configuration::MakePresetForLiveRunForBetterAds()};
    scoped_configuration().ResetConfiguration(
        base::MakeRefCounted<subresource_filter::ConfigurationList>(configs));
    EXPECT_TRUE(base::FeatureList::IsEnabled(kAbusiveExperienceEnforce));

    popup_blocker_ =
        SafeBrowsingTriggeredPopupBlocker::MaybeCreate(web_contents());
  }

  void ConfigureUrl(const GURL& url) {
    safe_browsing::ThreatMetadata metadata;
    metadata.subresource_filter_match = GetMatch(abusive_level_, bas_level_);
    auto threat_type =
        safe_browsing::SBThreatType::SB_THREAT_TYPE_SUBRESOURCE_FILTER;
    fake_safe_browsing_database()->AddBlacklistedUrl(url, threat_type,
                                                     metadata);
  }

  bool DidSendConsoleMessage(const std::string& message) {
    const auto& messages =
        content::RenderFrameHostTester::For(main_rfh())->GetConsoleMessages();
    return base::ContainsValue(messages, message);
  }

 protected:
  MetadataLevel abusive_level_ = METADATA_NONE;
  MetadataLevel bas_level_ = METADATA_NONE;

  std::unique_ptr<SafeBrowsingTriggeredPopupBlocker> popup_blocker_;

 private:
  base::test::ScopedFeatureList scoped_features_;
  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterAbusiveTest);
};

TEST_P(SubresourceFilterAbusiveTest, ConfigCombination) {
  SCOPED_TRACE(testing::Message() << "Abusive Level: " << abusive_level_
                                  << " BAS Level: " << bas_level_);
  const GURL url("https://example.test/");
  ConfigureUrl(url);
  SimulateNavigateAndCommit(url, main_rfh());

  bool disallow_requests = !CreateAndNavigateDisallowedSubframe(main_rfh());
  bool disallow_popups = popup_blocker_->ShouldApplyStrongPopupBlocker(nullptr);

  // Enforcement.
  EXPECT_EQ(bas_level_ == METADATA_ENFORCE, disallow_requests);
  EXPECT_EQ(bas_level_ == METADATA_ENFORCE,
            !!GetSettingsManager()->GetSiteMetadata(url));
  EXPECT_EQ(abusive_level_ == METADATA_ENFORCE, disallow_popups);

  // Activation / enforce messages.
  EXPECT_EQ(
      bas_level_ == METADATA_ENFORCE,
      DidSendConsoleMessage(subresource_filter::kActivationConsoleMessage));
  EXPECT_EQ(abusive_level_ == METADATA_ENFORCE,
            DidSendConsoleMessage(kAbusiveEnforceMessage));

  // Warn messages.
  EXPECT_EQ(bas_level_ == METADATA_WARN,
            DidSendConsoleMessage(
                subresource_filter::kActivationWarningConsoleMessage));
  EXPECT_EQ(abusive_level_ == METADATA_WARN,
            DidSendConsoleMessage(kAbusiveWarnMessage));
}

INSTANTIATE_TEST_CASE_P(
    /* no prefix */,
    SubresourceFilterAbusiveTest,
    ::testing::Combine(
        ::testing::Values(METADATA_WARN, METADATA_ENFORCE, METADATA_NONE),
        ::testing::Values(METADATA_WARN, METADATA_ENFORCE, METADATA_NONE)));
