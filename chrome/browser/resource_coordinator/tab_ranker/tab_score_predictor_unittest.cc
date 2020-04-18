// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_ranker/tab_score_predictor.h"

#include <memory>

#include "base/time/time.h"
#include "chrome/browser/resource_coordinator/tab_ranker/mru_features.h"
#include "chrome/browser/resource_coordinator/tab_ranker/tab_features.h"
#include "chrome/browser/resource_coordinator/tab_ranker/window_features.h"
#include "components/sessions/core/session_id.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"

namespace tab_ranker {
namespace {

// A URL whose host is not one of the top sites in the model.
const char* kUnseenHost = "www.example.com";

// Returns a set of features for a fairly typcial tab. If |user_activity| is
// true, the features will reflect some amount of user activity, e.g.
// navigations and input events.
// The TabFeatures returned can be used as a baseline for testing how changing a
// feature affects the score, but this is an imperfect testing method because
// features are inter-dependent in our model.
TabFeatures GetTabFeatures(std::string host, bool user_activity = false) {
  // Assign typical/reasonable values for background tabs.
  TabFeatures tab;
  tab.has_before_unload_handler = true;
  tab.has_form_entry = false;
  tab.host = host;
  tab.is_pinned = false;
  tab.key_event_count = user_activity ? 20 : 0;
  tab.mouse_event_count = user_activity ? 20 : 0;
  tab.navigation_entry_count = user_activity ? 4 : 1;
  tab.num_reactivations = user_activity ? 1 : 0;
  tab.page_transition_core_type = ui::PAGE_TRANSITION_LINK;
  tab.page_transition_from_address_bar = true;
  tab.page_transition_is_redirect = false;
  tab.site_engagement_score = user_activity ? 20 : 0;
  tab.time_from_backgrounded =
      base::TimeDelta::FromMinutes(10).InMilliseconds();
  // Even tabs with activity usually have 0 touch events.
  tab.touch_event_count = 0;
  tab.was_recently_audible = false;

  return tab;
}

// Returns a fairly typical set of window features.
WindowFeatures GetWindowFeatures() {
  WindowFeatures window(SessionID::NewUnique(),
                        metrics::WindowMetricsEvent::TYPE_TABBED);
  window.tab_count = 3;
  return window;
}

// Returns some MRU features.
// // TODO: find actual nums
MRUFeatures GetMRUFeatures() {
  MRUFeatures mru;
  mru.index = 3;
  mru.total = 6;
  return mru;
}

// These tests try to sanity-check the model by seeing how changing one feature
// impacts the score while everything else remains constant.
// For example, it seems reasonable to expect that a pinned tab would have a
// higher score than an unpinned tab, all else being equal.
//
// This won't always be the case, so these tests will be fragile, but *only*
// when updating the model; if the tests fail without changing the model,
// then the input to the model must have changed, which is probably a
// regression.
//
// If several tests fail locally after updating the model, check whether the
// features are being logged and set correctly, and think about why the model
// might be making counter-intuitive predictions. If everything checks out, try
// to find more realistic tab examples where the feature behaves as expected.
// If that fails, just remove the test -- the point is to avoid accidental model
// regressions, not to force fit a model to these expectations.
class TabScorePredictorTest : public testing::Test {
 public:
  TabScorePredictorTest() = default;
  ~TabScorePredictorTest() override = default;

 protected:
  // Returns a prediction for the tab example.
  float ScoreTab(const TabFeatures& tab,
                 const WindowFeatures& window = GetWindowFeatures(),
                 const MRUFeatures& mru = GetMRUFeatures()) {
    float score = 0;
    EXPECT_EQ(TabRankerResult::kSuccess,
              tab_score_predictor_.ScoreTab(tab, window, mru, &score));
    return score;
  }

 private:
  TabScorePredictor tab_score_predictor_;

  DISALLOW_COPY_AND_ASSIGN(TabScorePredictorTest);
};

}  // namespace

// Checks the score for an example that we have calculated a known score for
// outside of Chrome.
TEST_F(TabScorePredictorTest, KnownScore) {
  MRUFeatures mru;
  mru.index = 27;
  mru.total = 30;

  TabFeatures tab;
  tab.has_before_unload_handler = true;
  tab.has_form_entry = true;
  tab.host = "www.google.com";
  tab.is_pinned = true;
  tab.key_event_count = 21;
  tab.mouse_event_count = 22;
  tab.navigation_entry_count = 24;
  tab.num_reactivations = 25;
  tab.page_transition_core_type = ui::PAGE_TRANSITION_AUTO_BOOKMARK;
  tab.page_transition_from_address_bar = true;
  tab.page_transition_is_redirect = true;
  tab.site_engagement_score = 26;
  tab.time_from_backgrounded = 10000;
  tab.touch_event_count = 28;
  tab.was_recently_audible = true;

  WindowFeatures window(GetWindowFeatures());
  window.tab_count = 27;

  // Pre-calculated score using the generated model outside of Chrome.
  EXPECT_FLOAT_EQ(8.0287771, ScoreTab(tab, window, mru));
}

// Checks the score for a different example that we have calculated a known
// score for outside of Chrome. This example omits the optional features.
TEST_F(TabScorePredictorTest, KnownScoreMissingOptionalFeatures) {
  MRUFeatures mru;
  mru.index = 13;
  mru.total = 130;

  TabFeatures tab;
  tab.has_before_unload_handler = true;
  tab.has_form_entry = true;
  tab.host = "www.example.com";
  tab.is_pinned = true;
  tab.key_event_count = 121;
  tab.mouse_event_count = 122;
  tab.navigation_entry_count = 124;
  tab.num_reactivations = 125;
  tab.page_transition_from_address_bar = true;
  tab.page_transition_is_redirect = true;
  tab.time_from_backgrounded = 110000;
  tab.touch_event_count = 128;
  tab.was_recently_audible = true;

  WindowFeatures window(GetWindowFeatures());
  window.tab_count = 127;

  // Pre-calculated score using the generated model outside of Chrome.
  EXPECT_FLOAT_EQ(10.577342, ScoreTab(tab, window, mru));
}

TEST_F(TabScorePredictorTest, InactiveDuration) {
  // A tab that has been in the background for a much shorter time is more
  // likely to be reactivated.
  TabFeatures shorter_example = GetTabFeatures(kUnseenHost);
  TabFeatures longer_example = GetTabFeatures(kUnseenHost);
  shorter_example.time_from_backgrounded =
      base::TimeDelta::FromMinutes(1).InMilliseconds();
  longer_example.time_from_backgrounded =
      base::TimeDelta::FromMinutes(1000).InMilliseconds();
  ASSERT_GT(ScoreTab(shorter_example), ScoreTab(longer_example));
}

TEST_F(TabScorePredictorTest, NavigationEntryCount) {
  TabFeatures example = GetTabFeatures(kUnseenHost, true /*user_activity*/);
  float default_score = ScoreTab(example);

  // A tab with more navigations is more likely to be reactivated.
  example.navigation_entry_count = 10;
  float navigated_score = ScoreTab(example);
  ASSERT_GT(navigated_score, default_score);
}

TEST_F(TabScorePredictorTest, NumReactivationBefore) {
  TabFeatures example = GetTabFeatures(kUnseenHost, true /*user_activity*/);
  example.num_reactivations = 0;
  float no_reactivations_score = ScoreTab(example);

  // A tab with reactivations is more likely to be reactivated than one without.
  example.num_reactivations = 10;
  float reactivations_score = ScoreTab(example);
  ASSERT_GT(reactivations_score, no_reactivations_score);

  // A tab with more reactivations is more likely to be reactivated.
  example.num_reactivations = 20;
  float more_reactivations_score = ScoreTab(example);
  ASSERT_GT(more_reactivations_score, reactivations_score);
}

TEST_F(TabScorePredictorTest, PageTransitionTypes) {
  TabFeatures example = GetTabFeatures(kUnseenHost, true /*user_activity*/);

  example.page_transition_core_type = ui::PAGE_TRANSITION_LINK;
  float link_score = ScoreTab(example);

  example.page_transition_core_type = ui::PAGE_TRANSITION_RELOAD;
  float reload_score = ScoreTab(example);

  // A tab the user manually reloaded is more likely to be reactivated.
  ASSERT_GT(reload_score, link_score);
}

TEST_F(TabScorePredictorTest, SiteEngagementScore) {
  TabFeatures example = GetTabFeatures(kUnseenHost, true /*user_activity*/);
  example.site_engagement_score.reset();
  float engagement_score_0 = ScoreTab(example);

  // A site with low engagement ranks higher than one with no engagement.
  example.site_engagement_score = 10;
  float engagement_score_low = ScoreTab(example);
  ASSERT_GT(engagement_score_low, engagement_score_0);

  // A site with moderate engagement ranks higher than one with low engagement.
  example.site_engagement_score = 50;
  float engagement_score_medium = ScoreTab(example);
  ASSERT_GT(engagement_score_medium, engagement_score_low);

  example.site_engagement_score = 80;
  float engagement_score_high = ScoreTab(example);
  ASSERT_GT(engagement_score_high, engagement_score_medium);
}

TEST_F(TabScorePredictorTest, TopURLHigherScore) {
  // mail.google.com is more likely to be reactivated (ie, the user is more
  // likely to return to a mail tab than an ordinary tab).
  TabFeatures unseen_example = GetTabFeatures(kUnseenHost, true
                                              /*user_activity*/);
  TabFeatures higher_example = GetTabFeatures("mail.google.com", true
                                              /*user_activity*/);
  ASSERT_GT(ScoreTab(higher_example), ScoreTab(unseen_example));
}

TEST_F(TabScorePredictorTest, TopURLLowerScore) {
  // Expect www.google.com tabs to be less likely to be reactivated.
  TabFeatures unseen_example =
      GetTabFeatures(kUnseenHost, true /*user_activity*/);
  TabFeatures lower_example =
      GetTabFeatures("www.google.com", true /*user_activity*/);
  ASSERT_LT(ScoreTab(lower_example), ScoreTab(unseen_example));
}

TEST_F(TabScorePredictorTest, WasRecentlyAudible) {
  TabFeatures example = GetTabFeatures(kUnseenHost, true /*user_activity*/);
  float default_score = ScoreTab(example);

  // A recently audible tab is more likely to be reactivated.
  example.was_recently_audible = true;
  float recently_audible_score = ScoreTab(example);
  ASSERT_GT(recently_audible_score, default_score);
}

}  // namespace tab_ranker
