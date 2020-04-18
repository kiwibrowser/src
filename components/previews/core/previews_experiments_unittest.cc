// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_experiments.h"

#include <map>
#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "components/previews/core/previews_features.h"
#include "components/variations/variations_associated_data.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace previews {

namespace {

const char kClientSidePreviewsFieldTrial[] = "ClientSidePreviews";
const char kEnabled[] = "Enabled";

// Verifies that we can enable offline previews via comand line.
TEST(PreviewsExperimentsTest, TestCommandLineOfflinePage) {
  EXPECT_TRUE(params::IsOfflinePreviewsEnabled());

  std::unique_ptr<base::FeatureList> feature_list =
      std::make_unique<base::FeatureList>();

  // The feature is explicitly enabled on the command-line.
  feature_list->InitializeFromCommandLine("", "OfflinePreviews");
  base::FeatureList::ClearInstanceForTesting();
  base::FeatureList::SetInstance(std::move(feature_list));

  EXPECT_FALSE(params::IsOfflinePreviewsEnabled());
  base::FeatureList::ClearInstanceForTesting();
}

// Verifies that the default params are correct, and that custom params can be
// set, for both the previews blacklist and offline previews.
TEST(PreviewsExperimentsTest, TestParamsForBlackListAndOffline) {
  // Verify that the default params are correct.
  EXPECT_EQ(4u, params::MaxStoredHistoryLengthForPerHostBlackList());
  EXPECT_EQ(10u, params::MaxStoredHistoryLengthForHostIndifferentBlackList());
  EXPECT_EQ(100u, params::MaxInMemoryHostsInBlackList());
  EXPECT_EQ(2, params::PerHostBlackListOptOutThreshold());
  EXPECT_EQ(6, params::HostIndifferentBlackListOptOutThreshold());
  EXPECT_EQ(base::TimeDelta::FromDays(30), params::PerHostBlackListDuration());
  EXPECT_EQ(base::TimeDelta::FromDays(30),
            params::HostIndifferentBlackListPerHostDuration());
  EXPECT_EQ(base::TimeDelta::FromSeconds(60 * 5),
            params::SingleOptOutDuration());
  EXPECT_EQ(base::TimeDelta::FromDays(7),
            params::OfflinePreviewFreshnessDuration());
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_2G,
            params::GetECTThresholdForPreview(PreviewsType::OFFLINE));
  EXPECT_EQ(0, params::OfflinePreviewsVersion());

  base::FieldTrialList field_trial_list(nullptr);

  // Set some custom params. Somewhat random yet valid values.
  std::map<std::string, std::string> custom_params = {
      {"per_host_max_stored_history_length", "3"},
      {"host_indifferent_max_stored_history_length", "4"},
      {"max_hosts_in_blacklist", "13"},
      {"per_host_opt_out_threshold", "12"},
      {"host_indifferent_opt_out_threshold", "84"},
      {"per_host_black_list_duration_in_days", "99"},
      {"host_indifferent_black_list_duration_in_days", "64"},
      {"single_opt_out_duration_in_seconds", "28"},
      {"offline_preview_freshness_duration_in_days", "12"},
      {"max_allowed_effective_connection_type", "4G"},
      {"version", "10"},
  };
  EXPECT_TRUE(base::AssociateFieldTrialParams(kClientSidePreviewsFieldTrial,
                                              kEnabled, custom_params));
  EXPECT_TRUE(base::FieldTrialList::CreateFieldTrial(
      kClientSidePreviewsFieldTrial, kEnabled));

  EXPECT_EQ(3u, params::MaxStoredHistoryLengthForPerHostBlackList());
  EXPECT_EQ(4u, params::MaxStoredHistoryLengthForHostIndifferentBlackList());
  EXPECT_EQ(13u, params::MaxInMemoryHostsInBlackList());
  EXPECT_EQ(12, params::PerHostBlackListOptOutThreshold());
  EXPECT_EQ(84, params::HostIndifferentBlackListOptOutThreshold());
  EXPECT_EQ(base::TimeDelta::FromDays(99), params::PerHostBlackListDuration());
  EXPECT_EQ(base::TimeDelta::FromDays(64),
            params::HostIndifferentBlackListPerHostDuration());
  EXPECT_EQ(base::TimeDelta::FromSeconds(28), params::SingleOptOutDuration());
  EXPECT_EQ(base::TimeDelta::FromDays(12),
            params::OfflinePreviewFreshnessDuration());
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_4G,
            params::GetECTThresholdForPreview(PreviewsType::OFFLINE));
  EXPECT_EQ(10, params::OfflinePreviewsVersion());

  variations::testing::ClearAllVariationParams();
}

#if defined(OS_ANDROID)

TEST(PreviewsExperimentsTest, TestClientLoFiEnabledByDefaultOnAndroid) {
  EXPECT_TRUE(params::IsClientLoFiEnabled());
}

#else  // !defined(OS_ANDROID)

TEST(PreviewsExperimentsTest, TestClientLoFiDisabledByDefaultOnNonAndroid) {
  EXPECT_FALSE(params::IsClientLoFiEnabled());
}

#endif  // defined(OS_ANDROID)

TEST(PreviewsExperimentsTest, TestClientLoFiExplicitlyDisabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(features::kClientLoFi);
  EXPECT_FALSE(params::IsClientLoFiEnabled());
}

TEST(PreviewsExperimentsTest, TestClientLoFiExplicitlyEnabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kClientLoFi);
  EXPECT_TRUE(params::IsClientLoFiEnabled());
}

TEST(PreviewsExperimentsTest, TestEnableClientLoFiWithDefaultParams) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kClientLoFi);

  EXPECT_TRUE(params::IsClientLoFiEnabled());
  EXPECT_EQ(0, params::ClientLoFiVersion());
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_2G,
            params::EffectiveConnectionTypeThresholdForClientLoFi());
  EXPECT_EQ(std::vector<std::string>(),
            params::GetBlackListedHostsForClientLoFiFieldTrial());
}

TEST(PreviewsExperimentsTest, TestEnableClientLoFiWithCustomParams) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kClientLoFi,
      {{"version", "10"},
       {"max_allowed_effective_connection_type", "3G"},
       {"short_host_blacklist", "some,hosts, to-blacklist ,,"}});

  EXPECT_TRUE(params::IsClientLoFiEnabled());
  EXPECT_EQ(10, params::ClientLoFiVersion());
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_3G,
            params::EffectiveConnectionTypeThresholdForClientLoFi());
  EXPECT_EQ(std::vector<std::string>({"some", "hosts", "to-blacklist"}),
            params::GetBlackListedHostsForClientLoFiFieldTrial());
}

}  // namespace

}  // namespace previews
