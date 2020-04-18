// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_manager_features.h"

#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_piece.h"
#include "components/variations/variations_params_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

namespace {

class TabManagerFeaturesTest : public testing::Test {
 public:
  // Enables the proactive tab discarding feature, and sets up the associated
  // variations parameter values.
  void EnableProactiveTabDiscarding() {
    std::set<std::string> features;
    features.insert(features::kProactiveTabDiscarding.name);
    variations_manager_.SetVariationParamsWithFeatureAssociations(
        "DummyTrial", params_, features);
  }

  void SetParam(base::StringPiece key, base::StringPiece value) {
    params_[key.as_string()] = value.as_string();
  }

  void ExpectProactiveTabDiscardingParams(
      int low_loaded_tab_count,
      int moderate_loaded_tab_count,
      int high_loaded_tab_count,
      int memory_in_gb,
      base::TimeDelta low_occluded_timeout,
      base::TimeDelta moderate_occluded_timeout,
      base::TimeDelta high_occluded_timeout,
      base::TimeDelta favicon_update_observation_window,
      base::TimeDelta title_update_observation_window,
      base::TimeDelta audio_usage_observation_window,
      base::TimeDelta notifications_usage_observation_window) {
    ProactiveTabDiscardParams params =
        GetProactiveTabDiscardParams(memory_in_gb);

    EXPECT_EQ(low_loaded_tab_count, params.low_loaded_tab_count);
    EXPECT_EQ(moderate_loaded_tab_count, params.moderate_loaded_tab_count);

    // Enforce that |moderate_loaded_tab_count| is within [low_loaded_tab_count,
    // high_loaded_tab_count].
    EXPECT_GE(params.moderate_loaded_tab_count, params.low_loaded_tab_count);
    EXPECT_LE(params.moderate_loaded_tab_count, params.high_loaded_tab_count);

    EXPECT_EQ(high_loaded_tab_count, params.high_loaded_tab_count);
    EXPECT_EQ(low_occluded_timeout, params.low_occluded_timeout);
    EXPECT_EQ(moderate_occluded_timeout, params.moderate_occluded_timeout);
    EXPECT_EQ(high_occluded_timeout, params.high_occluded_timeout);

    EXPECT_EQ(favicon_update_observation_window,
              params.favicon_update_observation_window);
    EXPECT_EQ(title_update_observation_window,
              params.title_update_observation_window);
    EXPECT_EQ(audio_usage_observation_window,
              params.audio_usage_observation_window);
    EXPECT_EQ(notifications_usage_observation_window,
              params.notifications_usage_observation_window);
  }

  void ExpectDefaultProactiveTabDiscardParams() {
    int memory_in_gb = 4;
    ExpectProactiveTabDiscardingParams(
        kProactiveTabDiscard_LowLoadedTabCountDefault,
        kProactiveTabDiscard_ModerateLoadedTabsPerGbRamDefault * memory_in_gb,
        kProactiveTabDiscard_HighLoadedTabCountDefault, memory_in_gb,
        kProactiveTabDiscard_LowOccludedTimeoutDefault,
        kProactiveTabDiscard_ModerateOccludedTimeoutDefault,
        kProactiveTabDiscard_HighOccludedTimeoutDefault,
        kProactiveTabDiscard_FaviconUpdateObservationWindow_Default,
        kProactiveTabDiscard_TitleUpdateObservationWindow_Default,
        kProactiveTabDiscard_AudioUsageObservationWindow_Default,
        kProactiveTabDiscard_NotificationsUsageObservationWindow_Default);
  }

 private:
  std::map<std::string, std::string> params_;
  variations::testing::VariationParamsManager variations_manager_;
};

}  // namespace

TEST_F(TabManagerFeaturesTest,
       GetProactiveTabDiscardParamsDisabledFeatureGoesToDefault) {
  // Do not enable the proactive tab discarding feature.
  ExpectDefaultProactiveTabDiscardParams();
}

TEST_F(TabManagerFeaturesTest, GetProactiveTabDiscardParamsNoneGoesToDefault) {
  EnableProactiveTabDiscarding();
  ExpectDefaultProactiveTabDiscardParams();
}

TEST_F(TabManagerFeaturesTest,
       GetProactiveTabDiscardParamsInvalidGoesToDefault) {
  SetParam(kProactiveTabDiscard_LowLoadedTabCountParam, "ab");
  SetParam(kProactiveTabDiscard_ModerateLoadedTabsPerGbRamParam, "27.8");
  SetParam(kProactiveTabDiscard_HighLoadedTabCountParam, "4e8");
  SetParam(kProactiveTabDiscard_LowOccludedTimeoutParam, "---");
  SetParam(kProactiveTabDiscard_ModerateOccludedTimeoutParam, " ");
  SetParam(kProactiveTabDiscard_HighOccludedTimeoutParam, "");
  SetParam(kProactiveTabDiscard_FaviconUpdateObservationWindow, "    ");
  SetParam(kProactiveTabDiscard_TitleUpdateObservationWindow, "foo");
  SetParam(kProactiveTabDiscard_AudioUsageObservationWindow, ".");
  SetParam(kProactiveTabDiscard_NotificationsUsageObservationWindow, "abc");
  EnableProactiveTabDiscarding();
  ExpectDefaultProactiveTabDiscardParams();
}

TEST_F(TabManagerFeaturesTest, GetProactiveTabDiscardParams) {
  SetParam(kProactiveTabDiscard_LowLoadedTabCountParam, "7");
  SetParam(kProactiveTabDiscard_ModerateLoadedTabsPerGbRamParam, "4");
  SetParam(kProactiveTabDiscard_HighLoadedTabCountParam, "42");
  // These are expressed in seconds.
  SetParam(kProactiveTabDiscard_LowOccludedTimeoutParam, "60");
  SetParam(kProactiveTabDiscard_ModerateOccludedTimeoutParam, "120");
  SetParam(kProactiveTabDiscard_HighOccludedTimeoutParam, "247");
  SetParam(kProactiveTabDiscard_FaviconUpdateObservationWindow, "3600");
  SetParam(kProactiveTabDiscard_TitleUpdateObservationWindow, "36000");
  SetParam(kProactiveTabDiscard_AudioUsageObservationWindow, "360000");
  SetParam(kProactiveTabDiscard_NotificationsUsageObservationWindow, "3600000");
  EnableProactiveTabDiscarding();

  // Should snap |moderate_loaded_tab_count| to |low_loaded_tab_count|, when the
  // amount of physical memory is so low that (|memory_in_gb| *
  // |moderate_tab_count_per_gb_ram|) < |low_loaded_tab_count|).
  int memory_in_gb_low = 1;
  ExpectProactiveTabDiscardingParams(
      7, 7, 42, memory_in_gb_low, base::TimeDelta::FromSeconds(60),
      base::TimeDelta::FromSeconds(120), base::TimeDelta::FromSeconds(247),
      base::TimeDelta::FromSeconds(3600), base::TimeDelta::FromSeconds(36000),
      base::TimeDelta::FromSeconds(360000),
      base::TimeDelta::FromSeconds(3600000));

  // Should snap |moderate_loaded_tab_count| to |high_loaded_tab_count|, when
  // the amount of physical memory is so high that (|memory_in_gb| *
  // |moderate_tab_count_per_gb_ram|) > |high_loaded_tab_count|).
  int memory_in_gb_high = 100;
  ExpectProactiveTabDiscardingParams(
      7, 42, 42, memory_in_gb_high, base::TimeDelta::FromSeconds(60),
      base::TimeDelta::FromSeconds(120), base::TimeDelta::FromSeconds(247),
      base::TimeDelta::FromSeconds(3600), base::TimeDelta::FromSeconds(36000),
      base::TimeDelta::FromSeconds(360000),
      base::TimeDelta::FromSeconds(3600000));

  // Tests normal case where |memory_in gb| * |moderate_tab_count_per_gb_ram| is
  // in the interval [low_loaded_tab_count, high_loaded_tab_count].
  int memory_in_gb_normal = 4;
  ExpectProactiveTabDiscardingParams(
      7, 16, 42, memory_in_gb_normal, base::TimeDelta::FromSeconds(60),
      base::TimeDelta::FromSeconds(120), base::TimeDelta::FromSeconds(247),
      base::TimeDelta::FromSeconds(3600), base::TimeDelta::FromSeconds(36000),
      base::TimeDelta::FromSeconds(360000),
      base::TimeDelta::FromSeconds(3600000));
}

}  // namespace resource_coordinator
