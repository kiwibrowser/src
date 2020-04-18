// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/search_engine_base_url_tracker.h"

#include "base/test/mock_callback.h"
#include "chrome/browser/search/instant_unittest_base.h"
#include "chrome/browser/search_engines/ui_thread_search_terms_data.h"
#include "url/gurl.h"

using SearchEngineBaseURLTrackerTest = InstantUnitTestBase;

TEST_F(SearchEngineBaseURLTrackerTest, DispatchDefaultSearchProviderChanged) {
  base::MockCallback<SearchEngineBaseURLTracker::BaseURLChangedCallback>
      callback;
  SearchEngineBaseURLTracker tracker(
      template_url_service_,
      std::make_unique<UIThreadSearchTermsData>(profile()), callback.Get());

  // Changing the search provider should invoke the callback.
  EXPECT_CALL(
      callback,
      Run(SearchEngineBaseURLTracker::ChangeReason::DEFAULT_SEARCH_PROVIDER));
  SetUserSelectedDefaultSearchProvider("https://bar.com/");
}

TEST_F(SearchEngineBaseURLTrackerTest, DispatchGoogleURLUpdated) {
  base::MockCallback<SearchEngineBaseURLTracker::BaseURLChangedCallback>
      callback;
  SearchEngineBaseURLTracker tracker(
      template_url_service_,
      std::make_unique<UIThreadSearchTermsData>(profile()), callback.Get());

  // While Google is the default search provider, changes to the Google base URL
  // should invoke the callback.
  EXPECT_CALL(callback,
              Run(SearchEngineBaseURLTracker::ChangeReason::GOOGLE_BASE_URL));
  NotifyGoogleBaseURLUpdate("https://www.google.es/");
}

TEST_F(SearchEngineBaseURLTrackerTest,
       DontDispatchGoogleURLUpdatedForNonGoogleSearchProvider) {
  base::MockCallback<SearchEngineBaseURLTracker::BaseURLChangedCallback>
      callback;
  SearchEngineBaseURLTracker tracker(
      template_url_service_,
      std::make_unique<UIThreadSearchTermsData>(profile()), callback.Get());

  // Set up a non-Google default search provider.
  EXPECT_CALL(
      callback,
      Run(SearchEngineBaseURLTracker::ChangeReason::DEFAULT_SEARCH_PROVIDER));
  SetUserSelectedDefaultSearchProvider("https://bar.com/");
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Now, a change to the Google base URL should not invoke the callback.
  EXPECT_CALL(callback, Run(testing::_)).Times(0);
  NotifyGoogleBaseURLUpdate("https://www.google.es/");
}
