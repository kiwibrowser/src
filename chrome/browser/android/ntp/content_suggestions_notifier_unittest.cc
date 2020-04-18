// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/ntp/content_suggestions_notifier.h"

#include "chrome/browser/android/ntp/content_suggestions_notifier_service.h"
#include "chrome/common/pref_names.h"
#include "components/ntp_snippets/features.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/variations/variations_params_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using ContentSuggestionsNotifierTest = ::testing::Test;

TEST_F(ContentSuggestionsNotifierTest, AreNotificationsEnabled) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  ContentSuggestionsNotifierService::RegisterProfilePrefs(prefs.registry());
  prefs.registry()->RegisterBooleanPref(
      ntp_snippets::prefs::kArticlesListVisible, true);

  EXPECT_TRUE(ContentSuggestionsNotifier::ShouldSendNotifications(&prefs));

  prefs.SetBoolean(prefs::kContentSuggestionsNotificationsEnabled, false);
  EXPECT_FALSE(ContentSuggestionsNotifier::ShouldSendNotifications(&prefs));

  prefs.SetBoolean(prefs::kContentSuggestionsNotificationsEnabled, true);
  EXPECT_TRUE(ContentSuggestionsNotifier::ShouldSendNotifications(&prefs));

  prefs.SetBoolean(ntp_snippets::prefs::kArticlesListVisible, false);
  EXPECT_FALSE(ContentSuggestionsNotifier::ShouldSendNotifications(&prefs));

  prefs.SetBoolean(ntp_snippets::prefs::kArticlesListVisible, true);
  EXPECT_TRUE(ContentSuggestionsNotifier::ShouldSendNotifications(&prefs));
}

TEST_F(ContentSuggestionsNotifierTest, AreNotificationsEnabledAutoOptOut) {
  variations::testing::VariationParamsManager params(
      "ContentSuggestionsNotifierTest",
      {
          {"enable_auto_opt_out", "true"},
          {ntp_snippets::kNotificationsIgnoredLimitParam, "3"},
      },
      {ntp_snippets::kNotificationsFeature.name});

  sync_preferences::TestingPrefServiceSyncable prefs;
  ContentSuggestionsNotifierService::RegisterProfilePrefs(prefs.registry());
  prefs.registry()->RegisterBooleanPref(
      ntp_snippets::prefs::kArticlesListVisible, true);

  EXPECT_TRUE(ContentSuggestionsNotifier::ShouldSendNotifications(
      &prefs));  // Enabled by default.

  prefs.SetInteger(prefs::kContentSuggestionsConsecutiveIgnoredPrefName, 2);
  EXPECT_TRUE(ContentSuggestionsNotifier::ShouldSendNotifications(
      &prefs));  // Not yet disabled.

  prefs.SetInteger(prefs::kContentSuggestionsConsecutiveIgnoredPrefName, 3);
  EXPECT_FALSE(ContentSuggestionsNotifier::ShouldSendNotifications(
      &prefs));  // Past the threshold.

  // Below the threshold again; notifications are re-enabled. In practice, this
  // could happen in one of two ways:
  //   * There were multiple notifications visible; one was ignored, triggering
  //     auto-opt-out, but the user opened another, resetting to 0.
  //   * A change in variations parameters raised the limit.
  prefs.SetInteger(prefs::kContentSuggestionsConsecutiveIgnoredPrefName, 2);
  EXPECT_TRUE(ContentSuggestionsNotifier::ShouldSendNotifications(&prefs));
}

}  // namespace
