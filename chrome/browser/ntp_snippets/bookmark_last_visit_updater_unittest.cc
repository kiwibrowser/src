// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ntp_snippets/bookmark_last_visit_updater.h"

#include <string>
#include <vector>

#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/ntp_snippets/bookmarks/bookmark_last_visit_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_web_contents_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::NavigationSimulator;
using content::WebContents;
using content::TestBrowserThreadBundle;
using content::TestWebContentsFactory;
using testing::IsEmpty;
using testing::Not;

TEST(BookmarkLastVisitUpdaterTest, DoesNotCrashForNoBookmarkModel) {
  TestBrowserThreadBundle thread_bundle;
  TestingProfile profile;

  // Create a tab with the BookmarkLastVisitUpdater (with no BookmarkModel*).
  TestWebContentsFactory factory;
  WebContents* tab = factory.CreateWebContents(&profile);
  BookmarkLastVisitUpdater::MaybeCreateForWebContentsWithBookmarkModel(
      tab, /*bookmark_model=*/nullptr);

  // Visit a URL.
  auto navigation =
      NavigationSimulator::CreateBrowserInitiated(GURL("http://foo.org/"), tab);
  navigation->Start();

  // The only expectation is that it does not crash.
}

TEST(BookmarkLastVisitUpdaterTest, IsTrackingVisits) {
  TestBrowserThreadBundle thread_bundle;
  TestingProfile profile;
  profile.CreateBookmarkModel(true);
  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(&profile);
  bookmarks::test::WaitForBookmarkModelToLoad(bookmark_model);

  // Create a bookmark.
  GURL url = GURL("http://foo.org/");
  bookmark_model->AddURL(bookmark_model->bookmark_bar_node(), 0,
                         base::string16(), url);

  // Create a tab with the BookmarkLastVisitUpdater.
  TestWebContentsFactory factory;
  WebContents* tab = factory.CreateWebContents(&profile);
  BookmarkLastVisitUpdater::MaybeCreateForWebContentsWithBookmarkModel(
      tab, bookmark_model);

  // Visit the bookmarked URL.
  auto navigation = NavigationSimulator::CreateBrowserInitiated(url, tab);
  navigation->Start();

  EXPECT_THAT(ntp_snippets::GetRecentlyVisitedBookmarks(
                  bookmark_model, 2, base::Time::UnixEpoch(),
                  /*consider_visits_from_desktop=*/true),
              Not(IsEmpty()));
}

TEST(BookmarkLastVisitUpdaterTest, IsNotTrackingIncognitoVisits) {
  TestBrowserThreadBundle thread_bundle;
  TestingProfile profile;
  profile.CreateBookmarkModel(true);
  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(&profile);
  bookmarks::test::WaitForBookmarkModelToLoad(bookmark_model);

  // Create a couple of bookmarks.
  GURL url = GURL("http://foo.org/");
  bookmark_model->AddURL(bookmark_model->bookmark_bar_node(), 0,
                         base::string16(), url);

  // Create an incognito tab with the BookmarkLastVisitUpdater.
  Profile* incognito = profile.GetOffTheRecordProfile();
  TestWebContentsFactory factory;
  WebContents* tab = factory.CreateWebContents(incognito);
  BookmarkLastVisitUpdater::MaybeCreateForWebContentsWithBookmarkModel(
      tab, bookmark_model);

  // Visit the bookmarked URL.
  auto navigation = NavigationSimulator::CreateBrowserInitiated(url, tab);
  navigation->Start();

  // The incognito visit should _not_ appear in recent bookmarks.
  EXPECT_THAT(ntp_snippets::GetRecentlyVisitedBookmarks(
                  bookmark_model, 2, base::Time::UnixEpoch(),
                  /*consider_visits_from_desktop=*/true),
              IsEmpty());
}
