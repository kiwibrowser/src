// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/google/google_url_tracker_factory.h"

#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "components/google/core/browser/google_pref_names.h"
#include "components/google/core/browser/google_url_tracker.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class GoogleURLTrackerFactoryTest : public testing::Test {
 protected:
  void SetUp() override {
    // TestingProfile requires a testing factory to be set otherwise it
    // registers a factory that returns nullptr. Here we simply use the real
    // factory.
    GoogleURLTrackerFactory::GetInstance()->SetTestingFactory(
        &profile_, GoogleURLTrackerFactory::GetDefaultFactory());
  }

  // Required because GetForProfile() wants to run on the browser thread.
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  TestingProfile profile_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(GoogleURLTrackerFactoryTest, UsesLocalDomainByDefault) {
  profile_.GetPrefs()->SetString(prefs::kLastKnownGoogleURL,
                                 "https://www.google.co.uk/");
  GoogleURLTracker* tracker =
      GoogleURLTrackerFactory::GetInstance()->GetForProfile(&profile_);

  ASSERT_NE(nullptr, tracker);
  EXPECT_EQ(GURL("https://www.google.co.uk/"), tracker->google_url());
}

TEST_F(GoogleURLTrackerFactoryTest, UsesDotComWithNoSearchDomainCheck) {
  profile_.GetPrefs()->SetString(prefs::kLastKnownGoogleURL,
                                 "https://www.google.co.uk/");
  scoped_feature_list_.InitAndEnableFeature(
      GoogleURLTracker::kNoSearchDomainCheck);
  GoogleURLTracker* tracker =
      GoogleURLTrackerFactory::GetInstance()->GetForProfile(&profile_);

  ASSERT_NE(nullptr, tracker);
  EXPECT_EQ(GURL("https://www.google.com/"), tracker->google_url());
}
