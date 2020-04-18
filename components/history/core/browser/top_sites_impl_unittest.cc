// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/top_sites_impl.h"

#include <stddef.h>
#include <stdint.h>

#include "base/bind.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "components/history/core/browser/default_top_sites_provider.h"
#include "components/history/core/browser/history_client.h"
#include "components/history/core/browser/history_constants.h"
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/history_db_task.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/top_sites.h"
#include "components/history/core/browser/top_sites_cache.h"
#include "components/history/core/browser/top_sites_observer.h"
#include "components/history/core/browser/visit_delegate.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/history/core/test/history_unittest_base.h"
#include "components/history/core/test/test_history_database.h"
#include "components/history/core/test/wait_top_sites_loaded_observer.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "url/gurl.h"

using testing::ContainerEq;

namespace history {

namespace {

const char kApplicationScheme[] = "application";
const char kPrepopulatedPageURL[] =
    "http://www.google.com/int/chrome/welcome.html";

// Returns whether |url| can be added to history.
bool MockCanAddURLToHistory(const GURL& url) {
  return url.is_valid() && !url.SchemeIs(kApplicationScheme);
}

// Used for querying top sites. Either runs sequentially, or runs a nested
// nested run loop until the response is complete. The later is used when
// TopSites is queried before it finishes loading.
class TopSitesQuerier {
 public:
  TopSitesQuerier()
      : number_of_callbacks_(0), waiting_(false), weak_ptr_factory_(this) {}

  // Queries top sites. If |wait| is true a nested run loop is run until the
  // callback is notified.
  void QueryTopSites(TopSitesImpl* top_sites, bool wait) {
    QueryAllTopSites(top_sites, wait, false);
  }

  // Queries top sites, including potentially forced URLs if
  // |include_forced_urls| is true.
  void QueryAllTopSites(TopSitesImpl* top_sites,
                        bool wait,
                        bool include_forced_urls) {
    int start_number_of_callbacks = number_of_callbacks_;
    base::RunLoop run_loop;
    top_sites->GetMostVisitedURLs(
        base::Bind(&TopSitesQuerier::OnTopSitesAvailable,
                   weak_ptr_factory_.GetWeakPtr(), &run_loop),
        include_forced_urls);
    if (wait && start_number_of_callbacks == number_of_callbacks_) {
      waiting_ = true;
      run_loop.Run();
    }
  }

  void CancelRequest() { weak_ptr_factory_.InvalidateWeakPtrs(); }

  void set_urls(const MostVisitedURLList& urls) { urls_ = urls; }
  const MostVisitedURLList& urls() const { return urls_; }

  int number_of_callbacks() const { return number_of_callbacks_; }

 private:
  // Callback for TopSitesImpl::GetMostVisitedURLs.
  void OnTopSitesAvailable(base::RunLoop* run_loop,
                           const history::MostVisitedURLList& data) {
    urls_ = data;
    number_of_callbacks_++;
    if (waiting_) {
      run_loop->QuitWhenIdle();
      waiting_ = false;
    }
  }

  MostVisitedURLList urls_;
  int number_of_callbacks_;
  bool waiting_;
  base::WeakPtrFactory<TopSitesQuerier> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TopSitesQuerier);
};

// Returns true if t1 and t2 contain the same data.
bool ThumbnailsAreEqual(base::RefCountedMemory* t1,
                        base::RefCountedMemory* t2) {
  if (!t1 || !t2)
    return false;
  if (t1->size() != t2->size())
    return false;
  return !memcmp(t1->front(), t2->front(), t1->size());
}

}  // namespace

class TopSitesImplTest : public HistoryUnitTestBase {
 public:
  TopSitesImplTest() {}

  void SetUp() override {
    ASSERT_TRUE(scoped_temp_dir_.CreateUniqueTempDir());
    pref_service_.reset(new TestingPrefServiceSimple);
    TopSitesImpl::RegisterPrefs(pref_service_->registry());
    history_service_.reset(
        new HistoryService(nullptr, std::unique_ptr<VisitDelegate>()));
    ASSERT_TRUE(history_service_->Init(
        TestHistoryDatabaseParamsForPath(scoped_temp_dir_.GetPath())));
    ResetTopSites();
    WaitTopSitesLoaded();
  }

  void TearDown() override {
    DestroyTopSites();
    history_service_->Shutdown();
    history_service_.reset();
    pref_service_.reset();
  }

  // Creates a bitmap of the specified color. Caller takes ownership.
  gfx::Image CreateBitmap(SkColor color) {
    SkBitmap thumbnail;
    thumbnail.allocN32Pixels(4, 4);
    thumbnail.eraseColor(color);
    return gfx::Image::CreateFrom1xBitmap(thumbnail);  // adds ref.
  }

  // Forces top sites to load top sites from history, then recreates top sites.
  // Recreating top sites makes sure the changes from history are saved and
  // loaded from the db.
  void RefreshTopSitesAndRecreate() {
    StartQueryForMostVisited();
    WaitForHistory();
    RecreateTopSitesAndBlock();
  }

  // Blocks the caller until history processes a task. This is useful if you
  // need to wait until you know history has processed a task.
  void WaitForHistory() {
    BlockUntilHistoryProcessesPendingRequests(history_service());
  }

  TopSitesImpl* top_sites() { return top_sites_impl_.get(); }

  HistoryService* history_service() { return history_service_.get(); }

  PrepopulatedPageList GetPrepopulatedPages() {
    return top_sites()->GetPrepopulatedPages();
  }

  // Returns true if the TopSitesQuerier contains the prepopulate data starting
  // at |start_index|.
  void ContainsPrepopulatePages(const TopSitesQuerier& querier,
                                size_t start_index) {
    PrepopulatedPageList prepopulate_pages = GetPrepopulatedPages();
    ASSERT_LE(start_index + prepopulate_pages.size(), querier.urls().size());
    for (size_t i = 0; i < prepopulate_pages.size(); ++i) {
      EXPECT_EQ(prepopulate_pages[i].most_visited.url.spec(),
                querier.urls()[start_index + i].url.spec())
          << " @ index " << i;
    }
  }

  // Adds a page to history.
  void AddPageToHistory(const GURL& url) {
    RedirectList redirects;
    redirects.push_back(url);
    history_service()->AddPage(
        url, base::Time::Now(), reinterpret_cast<ContextID>(1), 0, GURL(),
        redirects, ui::PAGE_TRANSITION_TYPED, history::SOURCE_BROWSED, false);
  }

  // Adds a page to history.
  void AddPageToHistory(const GURL& url, const base::string16& title) {
    RedirectList redirects;
    redirects.push_back(url);
    history_service()->AddPage(
        url, base::Time::Now(), reinterpret_cast<ContextID>(1), 0, GURL(),
        redirects, ui::PAGE_TRANSITION_TYPED, history::SOURCE_BROWSED, false);
    history_service()->SetPageTitle(url, title);
  }

  // Adds a page to history.
  void AddPageToHistory(const GURL& url,
                        const base::string16& title,
                        const history::RedirectList& redirects,
                        base::Time time) {
    history_service()->AddPage(
        url, time, reinterpret_cast<ContextID>(1), 0, GURL(),
        redirects, ui::PAGE_TRANSITION_TYPED, history::SOURCE_BROWSED,
        false);
    history_service()->SetPageTitle(url, title);
  }

  // Delets a url.
  void DeleteURL(const GURL& url) { history_service()->DeleteURL(url); }

  // Returns true if the thumbnail equals the specified bytes.
  bool ThumbnailEqualsBytes(const gfx::Image& image,
                            base::RefCountedMemory* bytes) {
    scoped_refptr<base::RefCountedBytes> encoded_image;
    TopSitesImpl::EncodeBitmap(image, &encoded_image);
    return ThumbnailsAreEqual(encoded_image.get(), bytes);
  }

  // Recreates top sites. This forces top sites to reread from the db.
  void RecreateTopSitesAndBlock() {
    // Recreate TopSites and wait for it to load.
    ResetTopSites();
    WaitTopSitesLoaded();
  }

  // Wrappers that allow private TopSites functions to be called from the
  // individual tests without making them all be friends.
  GURL GetCanonicalURL(const GURL& url) {
    return top_sites()->cache_->GetCanonicalURL(url);
  }

  void SetTopSites(const MostVisitedURLList& new_top_sites) {
    top_sites()->SetTopSites(new_top_sites,
                             TopSitesImpl::CALL_LOCATION_FROM_OTHER_PLACES);
  }

  bool AddForcedURL(const GURL& url, base::Time time) {
    return top_sites()->AddForcedURL(url, time);
  }

  void StartQueryForMostVisited() { top_sites()->StartQueryForMostVisited(); }

  bool IsTopSitesLoaded() { return top_sites()->loaded_; }

  bool AddPrepopulatedPages(MostVisitedURLList* urls) {
    return top_sites()->AddPrepopulatedPages(urls, 0u);
  }

  void EmptyThreadSafeCache() {
    base::AutoLock lock(top_sites()->lock_);
    MostVisitedURLList empty;
    top_sites()->thread_safe_cache_->SetTopSites(empty);
  }

  void ResetTopSites() {
    // TopSites shutdown takes some time as it happens on the DB thread and does
    // not support the existence of two TopSitesImpl for a location (due to
    // database locking). DestroyTopSites() waits for the TopSites cleanup to
    // complete before returning.
    DestroyTopSites();
    DCHECK(!top_sites_impl_);
    PrepopulatedPageList prepopulated_pages;
    prepopulated_pages.push_back(PrepopulatedPage(GURL(kPrepopulatedPageURL),
                                                  base::string16(), -1, -1, 0));
    top_sites_impl_ = new TopSitesImpl(
        pref_service_.get(), history_service_.get(),
        std::make_unique<DefaultTopSitesProvider>(history_service_.get()),
        prepopulated_pages, base::Bind(MockCanAddURLToHistory));
    top_sites_impl_->Init(scoped_temp_dir_.GetPath().Append(kTopSitesFilename));
  }

  void DestroyTopSites() {
    if (top_sites_impl_) {
      top_sites_impl_->ShutdownOnUIThread();
      top_sites_impl_ = nullptr;

      scoped_task_environment_.RunUntilIdle();
    }
  }

  void WaitTopSitesLoaded() {
    DCHECK(top_sites_impl_);
    WaitTopSitesLoadedObserver wait_top_sites_loaded_observer(top_sites_impl_);
    wait_top_sites_loaded_observer.Run();
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  base::ScopedTempDir scoped_temp_dir_;

  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  std::unique_ptr<HistoryService> history_service_;
  scoped_refptr<TopSitesImpl> top_sites_impl_;

  // To cancel HistoryService tasks.
  base::CancelableTaskTracker history_tracker_;

  // To cancel TopSitesBackend tasks.
  base::CancelableTaskTracker top_sites_tracker_;

  DISALLOW_COPY_AND_ASSIGN(TopSitesImplTest);
};  // Class TopSitesImplTest

// Helper function for appending a URL to a vector of "most visited" URLs,
// using the default values for everything but the URL.
void AppendMostVisitedURL(const GURL& url, std::vector<MostVisitedURL>* list) {
  MostVisitedURL mv;
  mv.url = url;
  mv.redirects.push_back(url);
  list->push_back(mv);
}

// Helper function for appending a URL to a vector of "most visited" URLs,
// using the default values for everything but the URL.
void AppendForcedMostVisitedURL(const GURL& url,
                                double last_forced_time,
                                std::vector<MostVisitedURL>* list) {
  MostVisitedURL mv;
  mv.url = url;
  mv.last_forced_time = base::Time::FromJsTime(last_forced_time);
  mv.redirects.push_back(url);
  list->push_back(mv);
}

// Same as AppendMostVisitedURL except that it adds a redirect from the first
// URL to the second.
void AppendMostVisitedURLWithRedirect(const GURL& redirect_source,
                                      const GURL& redirect_dest,
                                      std::vector<MostVisitedURL>* list) {
  MostVisitedURL mv;
  mv.url = redirect_dest;
  mv.redirects.push_back(redirect_source);
  mv.redirects.push_back(redirect_dest);
  list->push_back(mv);
}

// Helper function for appending a URL to a vector of "most visited" URLs,
// using the default values for everything but the URL and the title.
void AppendMostVisitedURLwithTitle(const GURL& url,
                                   const base::string16& title,
                                   std::vector<MostVisitedURL>* list) {
  MostVisitedURL mv;
  mv.url = url;
  mv.title = title;
  mv.redirects.push_back(url);
  list->push_back(mv);
}

// Tests GetCanonicalURL.
TEST_F(TopSitesImplTest, GetCanonicalURL) {
  // Have two chains:
  //   google.com -> www.google.com
  //   news.google.com (no redirects)
  GURL news("http://news.google.com/");
  GURL source("http://google.com/");
  GURL dest("http://www.google.com/");

  std::vector<MostVisitedURL> most_visited;
  AppendMostVisitedURLWithRedirect(source, dest, &most_visited);
  AppendMostVisitedURL(news, &most_visited);
  SetTopSites(most_visited);

  // Random URLs not in the database are returned unchanged.
  GURL result = GetCanonicalURL(GURL("http://fark.com/"));
  EXPECT_EQ(GURL("http://fark.com/"), result);

  // Easy case, there are no redirects and the exact URL is stored.
  result = GetCanonicalURL(news);
  EXPECT_EQ(news, result);

  // The URL in question is the source URL in a redirect list.
  result = GetCanonicalURL(source);
  EXPECT_EQ(dest, result);

  // The URL in question is the destination of a redirect.
  result = GetCanonicalURL(dest);
  EXPECT_EQ(dest, result);
}

class MockTopSitesObserver : public TopSitesObserver {
 public:
  MockTopSitesObserver() {}

  // history::TopSitesObserver:
  void TopSitesLoaded(TopSites* top_sites) override {}
  void TopSitesChanged(TopSites* top_sites,
                       ChangeReason change_reason) override {
    is_notified_ = true;
  }

  void ResetIsNotifiedState() { is_notified_ = false; }
  bool is_notified() const { return is_notified_; }

 private:
  bool is_notified_ = false;

  DISALLOW_COPY_AND_ASSIGN(MockTopSitesObserver);
};

// Tests DoTitlesDiffer.
TEST_F(TopSitesImplTest, DoTitlesDiffer) {
  GURL url_1("http://url1/");
  GURL url_2("http://url2/");
  base::string16 title_1(base::ASCIIToUTF16("title1"));
  base::string16 title_2(base::ASCIIToUTF16("title2"));

  MockTopSitesObserver observer;
  top_sites()->AddObserver(&observer);

  // TopSites has a new list of sites and should notify its observers.
  std::vector<MostVisitedURL> list_1;
  AppendMostVisitedURLwithTitle(url_1, title_1, &list_1);
  SetTopSites(list_1);
  EXPECT_TRUE(observer.is_notified());
  observer.ResetIsNotifiedState();
  EXPECT_FALSE(observer.is_notified());

  // list_1 and list_2 have different sizes. TopSites should notify its
  // observers.
  std::vector<MostVisitedURL> list_2;
  AppendMostVisitedURLwithTitle(url_1, title_1, &list_2);
  AppendMostVisitedURLwithTitle(url_2, title_2, &list_2);
  SetTopSites(list_2);
  EXPECT_TRUE(observer.is_notified());
  observer.ResetIsNotifiedState();
  EXPECT_FALSE(observer.is_notified());

  // list_1 and list_2 are exactly the same now. TopSites should not notify its
  // observers.
  AppendMostVisitedURLwithTitle(url_2, title_2, &list_1);
  SetTopSites(list_1);
  EXPECT_FALSE(observer.is_notified());

  // Change |url_2|'s title to |title_1| in list_2. The two lists are different
  // in titles now. TopSites should notify its observers.
  list_2.pop_back();
  AppendMostVisitedURLwithTitle(url_2, title_1, &list_2);
  SetTopSites(list_2);
  EXPECT_TRUE(observer.is_notified());

  top_sites()->RemoveObserver(&observer);
}

// Tests DiffMostVisited.
TEST_F(TopSitesImplTest, DiffMostVisited) {
  GURL stays_the_same("http://staysthesame/");
  GURL gets_added_1("http://getsadded1/");
  GURL gets_added_2("http://getsadded2/");
  GURL gets_deleted_1("http://getsdeleted1/");
  GURL gets_moved_1("http://getsmoved1/");

  std::vector<MostVisitedURL> old_list;
  AppendMostVisitedURL(stays_the_same, &old_list);  // 0  (unchanged)
  AppendMostVisitedURL(gets_deleted_1, &old_list);  // 1  (deleted)
  AppendMostVisitedURL(gets_moved_1, &old_list);    // 2  (moved to 3)

  std::vector<MostVisitedURL> new_list;
  AppendMostVisitedURL(stays_the_same, &new_list);  // 0  (unchanged)
  AppendMostVisitedURL(gets_added_1, &new_list);    // 1  (added)
  AppendMostVisitedURL(gets_added_2, &new_list);    // 2  (added)
  AppendMostVisitedURL(gets_moved_1, &new_list);    // 3  (moved from 2)

  history::TopSitesDelta delta;
  TopSitesImpl::DiffMostVisited(old_list, new_list, &delta);

  ASSERT_EQ(2u, delta.added.size());
  EXPECT_TRUE(gets_added_1 == delta.added[0].url.url);
  EXPECT_EQ(1, delta.added[0].rank);
  EXPECT_TRUE(gets_added_2 == delta.added[1].url.url);
  EXPECT_EQ(2, delta.added[1].rank);

  ASSERT_EQ(1u, delta.deleted.size());
  EXPECT_TRUE(gets_deleted_1 == delta.deleted[0].url);

  ASSERT_EQ(1u, delta.moved.size());
  EXPECT_TRUE(gets_moved_1 == delta.moved[0].url.url);
  EXPECT_EQ(3, delta.moved[0].rank);
}

// Tests DiffMostVisited with forced URLs.
TEST_F(TopSitesImplTest, DiffMostVisitedWithForced) {
  // Forced URLs.
  GURL stays_the_same_1("http://staysthesame1/");
  GURL new_last_forced_time("http://newlastforcedtime/");
  GURL stays_the_same_2("http://staysthesame2/");
  GURL move_to_nonforced("http://movetononforced/");
  GURL gets_added_1("http://getsadded1/");
  GURL gets_deleted_1("http://getsdeleted1/");
  // Non-forced URLs.
  GURL move_to_forced("http://movetoforced/");
  GURL stays_the_same_3("http://staysthesame3/");
  GURL gets_added_2("http://getsadded2/");
  GURL gets_deleted_2("http://getsdeleted2/");
  GURL gets_moved_1("http://getsmoved1/");

  std::vector<MostVisitedURL> old_list;
  AppendForcedMostVisitedURL(stays_the_same_1, 1000, &old_list);
  AppendForcedMostVisitedURL(new_last_forced_time, 2000, &old_list);
  AppendForcedMostVisitedURL(stays_the_same_2, 3000, &old_list);
  AppendForcedMostVisitedURL(move_to_nonforced, 4000, &old_list);
  AppendForcedMostVisitedURL(gets_deleted_1, 5000, &old_list);
  AppendMostVisitedURL(move_to_forced, &old_list);
  AppendMostVisitedURL(stays_the_same_3, &old_list);
  AppendMostVisitedURL(gets_deleted_2, &old_list);
  AppendMostVisitedURL(gets_moved_1, &old_list);

  std::vector<MostVisitedURL> new_list;
  AppendForcedMostVisitedURL(stays_the_same_1, 1000, &new_list);
  AppendForcedMostVisitedURL(stays_the_same_2, 3000, &new_list);
  AppendForcedMostVisitedURL(new_last_forced_time, 4000, &new_list);
  AppendForcedMostVisitedURL(gets_added_1, 5000, &new_list);
  AppendForcedMostVisitedURL(move_to_forced, 6000, &new_list);
  AppendMostVisitedURL(move_to_nonforced, &new_list);
  AppendMostVisitedURL(stays_the_same_3, &new_list);
  AppendMostVisitedURL(gets_added_2, &new_list);
  AppendMostVisitedURL(gets_moved_1, &new_list);

  TopSitesDelta delta;
  TopSitesImpl::DiffMostVisited(old_list, new_list, &delta);

  ASSERT_EQ(2u, delta.added.size());
  EXPECT_TRUE(gets_added_1 == delta.added[0].url.url);
  EXPECT_EQ(-1, delta.added[0].rank);
  EXPECT_TRUE(gets_added_2 == delta.added[1].url.url);
  EXPECT_EQ(2, delta.added[1].rank);

  ASSERT_EQ(2u, delta.deleted.size());
  EXPECT_TRUE(gets_deleted_1 == delta.deleted[0].url);
  EXPECT_TRUE(gets_deleted_2 == delta.deleted[1].url);

  ASSERT_EQ(3u, delta.moved.size());
  EXPECT_TRUE(new_last_forced_time == delta.moved[0].url.url);
  EXPECT_EQ(-1, delta.moved[0].rank);
  EXPECT_EQ(base::Time::FromJsTime(4000), delta.moved[0].url.last_forced_time);
  EXPECT_TRUE(move_to_forced == delta.moved[1].url.url);
  EXPECT_EQ(-1, delta.moved[1].rank);
  EXPECT_EQ(base::Time::FromJsTime(6000), delta.moved[1].url.last_forced_time);
  EXPECT_TRUE(move_to_nonforced == delta.moved[2].url.url);
  EXPECT_EQ(0, delta.moved[2].rank);
  EXPECT_TRUE(delta.moved[2].url.last_forced_time.is_null());
}

// Tests SetPageThumbnail.
TEST_F(TopSitesImplTest, SetPageThumbnail) {
  GURL url1a("http://google.com/");
  GURL url1b("http://www.google.com/");
  GURL url2("http://images.google.com/");
  GURL invalid_url("application://favicon/http://google.com/");

  std::vector<MostVisitedURL> list;
  AppendMostVisitedURL(url2, &list);

  MostVisitedURL mv;
  mv.url = url1b;
  mv.redirects.push_back(url1a);
  mv.redirects.push_back(url1b);
  list.push_back(mv);

  // Save our most visited data containing that one site.
  SetTopSites(list);

  // Create a dummy thumbnail.
  gfx::Image thumbnail(CreateBitmap(SK_ColorWHITE));

  base::Time now = base::Time::Now();
  ThumbnailScore low_score(1.0, true, true, now);
  ThumbnailScore medium_score(0.5, true, true, now);
  ThumbnailScore high_score(0.0, true, true, now);

  // Setting the thumbnail for invalid pages should fail.
  EXPECT_FALSE(
      top_sites()->SetPageThumbnail(invalid_url, thumbnail, medium_score));

  // Setting the thumbnail for url2 should succeed, lower scores shouldn't
  // replace it, higher scores should.
  EXPECT_TRUE(top_sites()->SetPageThumbnail(url2, thumbnail, medium_score));
  EXPECT_FALSE(top_sites()->SetPageThumbnail(url2, thumbnail, low_score));
  EXPECT_TRUE(top_sites()->SetPageThumbnail(url2, thumbnail, high_score));

  // Set on the redirect source should succeed. It should be replacable by
  // the same score on the redirect destination, which in turn should not
  // be replaced by the source again.
  EXPECT_TRUE(top_sites()->SetPageThumbnail(url1a, thumbnail, medium_score));
  EXPECT_TRUE(top_sites()->SetPageThumbnail(url1b, thumbnail, medium_score));
  EXPECT_FALSE(top_sites()->SetPageThumbnail(url1a, thumbnail, medium_score));
}

// Makes sure a thumbnail is correctly removed when the page is removed.
TEST_F(TopSitesImplTest, ThumbnailRemoved) {
  GURL url("http://google.com/");

  // Configure top sites with 'google.com'.
  std::vector<MostVisitedURL> list;
  AppendMostVisitedURL(url, &list);
  SetTopSites(list);

  // Create a dummy thumbnail.
  gfx::Image thumbnail(CreateBitmap(SK_ColorRED));

  base::Time now = base::Time::Now();
  ThumbnailScore low_score(1.0, true, true, now);
  ThumbnailScore medium_score(0.5, true, true, now);
  ThumbnailScore high_score(0.0, true, true, now);

  // Set the thumbnail.
  EXPECT_TRUE(top_sites()->SetPageThumbnail(url, thumbnail, medium_score));

  // Make sure the thumbnail was actually set.
  scoped_refptr<base::RefCountedMemory> result;
  EXPECT_TRUE(top_sites()->GetPageThumbnail(url, false, &result));
  EXPECT_TRUE(ThumbnailEqualsBytes(thumbnail, result.get()));

  // Reset the thumbnails and make sure we don't get it back.
  SetTopSites(MostVisitedURLList());
  EXPECT_FALSE(top_sites()->GetPageThumbnail(url, false, &result));
  // Recreating the TopSites object should also not bring it back.
  RefreshTopSitesAndRecreate();
  EXPECT_FALSE(top_sites()->GetPageThumbnail(url, false, &result));
}

// Tests GetPageThumbnail.
TEST_F(TopSitesImplTest, GetPageThumbnail) {
  MostVisitedURLList url_list;
  MostVisitedURL url1;
  url1.url = GURL("http://asdf.com");
  url1.redirects.push_back(url1.url);
  url_list.push_back(url1);

  MostVisitedURL url2;
  url2.url = GURL("http://gmail.com");
  url2.redirects.push_back(url2.url);
  url2.redirects.push_back(GURL("http://mail.google.com"));
  url_list.push_back(url2);

  SetTopSites(url_list);

  // Create a dummy thumbnail.
  gfx::Image thumbnail(CreateBitmap(SK_ColorWHITE));
  ThumbnailScore score(0.5, true, true, base::Time::Now());

  scoped_refptr<base::RefCountedMemory> result;
  EXPECT_TRUE(top_sites()->SetPageThumbnail(url1.url, thumbnail, score));
  EXPECT_TRUE(top_sites()->GetPageThumbnail(url1.url, false, &result));

  EXPECT_TRUE(top_sites()->SetPageThumbnail(GURL("http://gmail.com"),
                                            thumbnail, score));
  EXPECT_TRUE(top_sites()->GetPageThumbnail(GURL("http://gmail.com"),
                                            false,
                                            &result));
  // Get a thumbnail via a redirect.
  EXPECT_TRUE(top_sites()->GetPageThumbnail(GURL("http://mail.google.com"),
                                            false,
                                            &result));

  EXPECT_TRUE(top_sites()->SetPageThumbnail(GURL("http://mail.google.com"),
                                            thumbnail, score));
  EXPECT_TRUE(top_sites()->GetPageThumbnail(url2.url, false, &result));

  EXPECT_TRUE(ThumbnailEqualsBytes(thumbnail, result.get()));
}

// Tests GetMostVisitedURLs.
TEST_F(TopSitesImplTest, GetMostVisited) {
  GURL news("http://news.google.com/");
  GURL google("http://google.com/");

  AddPageToHistory(news);
  AddPageToHistory(google);

  StartQueryForMostVisited();
  WaitForHistory();

  TopSitesQuerier querier;
  querier.QueryTopSites(top_sites(), false);

  ASSERT_EQ(1, querier.number_of_callbacks());

  // 2 extra prepopulated URLs.
  ASSERT_EQ(2u + GetPrepopulatedPages().size(), querier.urls().size());
  EXPECT_EQ(news, querier.urls()[0].url);
  EXPECT_EQ(google, querier.urls()[1].url);
  ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 2));
}

// Tests GetMostVisitedURLs with a redirect.
TEST_F(TopSitesImplTest, GetMostVisitedWithRedirect) {
  GURL bare("http://cnn.com/");
  GURL www("https://www.cnn.com/");
  GURL edition("https://edition.cnn.com/");

  AddPageToHistory(edition, base::ASCIIToUTF16("CNN"),
                   history::RedirectList{bare, www, edition},
                   base::Time::Now());
  AddPageToHistory(edition);

  StartQueryForMostVisited();
  WaitForHistory();

  TopSitesQuerier querier;
  querier.QueryTopSites(top_sites(), false);

  ASSERT_EQ(1, querier.number_of_callbacks());

  // This behavior is not desirable: even though edition.cnn.com is in the list
  // of top sites, and the the bare URL cnn.com is just a redirect to it, we're
  // returning both. Even worse, the NTP will show the same title, icon, and
  // thumbnail for the site, so to the user it looks like we just have the same
  // thing twice.  (https://crbug.com/567132)
  std::vector<GURL> expected_urls = {bare, edition};  // should be {edition}.

  for (const auto& prepopulated : GetPrepopulatedPages()) {
    expected_urls.push_back(prepopulated.most_visited.url);
  }
  std::vector<GURL> actual_urls;
  for (const auto& actual : querier.urls()) {
    actual_urls.push_back(actual.url);
  }
  EXPECT_THAT(actual_urls, ContainerEq(expected_urls));
}

// Makes sure changes done to top sites get mirrored to the db.
TEST_F(TopSitesImplTest, SaveToDB) {
  MostVisitedURL url;
  GURL asdf_url("http://asdf.com");
  base::string16 asdf_title(base::ASCIIToUTF16("ASDF"));
  GURL google_url("http://google.com");
  base::string16 google_title(base::ASCIIToUTF16("Google"));
  GURL news_url("http://news.google.com");
  base::string16 news_title(base::ASCIIToUTF16("Google News"));

  // Add asdf_url to history.
  AddPageToHistory(asdf_url, asdf_title);

  // Make TopSites reread from the db.
  StartQueryForMostVisited();
  WaitForHistory();

  // Add a thumbnail.
  gfx::Image tmp_bitmap(CreateBitmap(SK_ColorBLUE));
  ASSERT_TRUE(top_sites()->SetPageThumbnail(asdf_url, tmp_bitmap,
                                            ThumbnailScore()));

  RecreateTopSitesAndBlock();

  {
    TopSitesQuerier querier;
    querier.QueryTopSites(top_sites(), false);
    ASSERT_EQ(1u + GetPrepopulatedPages().size(), querier.urls().size());
    EXPECT_EQ(asdf_url, querier.urls()[0].url);
    EXPECT_EQ(asdf_title, querier.urls()[0].title);
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 1));

    scoped_refptr<base::RefCountedMemory> read_data;
    EXPECT_TRUE(top_sites()->GetPageThumbnail(asdf_url, false, &read_data));
    EXPECT_TRUE(ThumbnailEqualsBytes(tmp_bitmap, read_data.get()));
  }

  MostVisitedURL url2;
  url2.url = google_url;
  url2.title = google_title;
  url2.redirects.push_back(url2.url);

  AddPageToHistory(url2.url, url2.title);

  // Add new thumbnail at rank 0 and shift the other result to 1.
  ASSERT_TRUE(top_sites()->SetPageThumbnail(google_url,
                                            tmp_bitmap,
                                            ThumbnailScore()));

  // Make TopSites reread from the db.
  RefreshTopSitesAndRecreate();

  {
    TopSitesQuerier querier;
    querier.QueryTopSites(top_sites(), false);
    ASSERT_EQ(2u + GetPrepopulatedPages().size(), querier.urls().size());
    EXPECT_EQ(asdf_url, querier.urls()[0].url);
    EXPECT_EQ(asdf_title, querier.urls()[0].title);
    EXPECT_EQ(google_url, querier.urls()[1].url);
    EXPECT_EQ(google_title, querier.urls()[1].title);
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 2));
  }
}

// Makes sure forced URLs in top sites get mirrored to the db.
TEST_F(TopSitesImplTest, SaveForcedToDB) {
  MostVisitedURL url;
  GURL asdf_url("http://asdf.com");
  base::string16 asdf_title(base::ASCIIToUTF16("ASDF"));
  GURL google_url("http://google.com");
  base::string16 google_title(base::ASCIIToUTF16("Google"));
  GURL news_url("http://news.google.com");
  base::string16 news_title(base::ASCIIToUTF16("Google News"));

  // Add a number of forced URLs.
  std::vector<MostVisitedURL> list;
  AppendForcedMostVisitedURL(GURL("http://forced1"), 1000, &list);
  list[0].title = base::ASCIIToUTF16("forced1");
  AppendForcedMostVisitedURL(GURL("http://forced2"), 2000, &list);
  AppendForcedMostVisitedURL(GURL("http://forced3"), 3000, &list);
  AppendForcedMostVisitedURL(GURL("http://forced4"), 4000, &list);
  SetTopSites(list);

  // Add a thumbnail.
  gfx::Image red_thumbnail(CreateBitmap(SK_ColorRED));
  ASSERT_TRUE(top_sites()->SetPageThumbnail(
                  GURL("http://forced1"), red_thumbnail, ThumbnailScore()));

  // Get the original thumbnail for later comparison. Some compression can
  // happen in |top_sites| and we don't want to depend on that.
  scoped_refptr<base::RefCountedMemory> orig_thumbnail_data;
  ASSERT_TRUE(top_sites()->GetPageThumbnail(GURL("http://forced1"), false,
                                            &orig_thumbnail_data));

  // Force-flush the cache to ensure we don't reread from it inadvertently.
  EmptyThreadSafeCache();

  // Make TopSites reread from the db.
  StartQueryForMostVisited();
  WaitForHistory();

  TopSitesQuerier querier;
  querier.QueryAllTopSites(top_sites(), true, true);

  ASSERT_EQ(4u + GetPrepopulatedPages().size(), querier.urls().size());
  EXPECT_EQ(GURL("http://forced1"), querier.urls()[0].url);
  EXPECT_EQ(base::ASCIIToUTF16("forced1"), querier.urls()[0].title);
  scoped_refptr<base::RefCountedMemory> thumbnail_data;
  ASSERT_TRUE(top_sites()->GetPageThumbnail(GURL("http://forced1"), false,
                                            &thumbnail_data));
  ASSERT_TRUE(
      ThumbnailsAreEqual(orig_thumbnail_data.get(), thumbnail_data.get()));
  EXPECT_EQ(base::Time::FromJsTime(1000), querier.urls()[0].last_forced_time);
  EXPECT_EQ(GURL("http://forced2"), querier.urls()[1].url);
  EXPECT_EQ(base::Time::FromJsTime(2000), querier.urls()[1].last_forced_time);
  EXPECT_EQ(GURL("http://forced3"), querier.urls()[2].url);
  EXPECT_EQ(base::Time::FromJsTime(3000), querier.urls()[2].last_forced_time);
  EXPECT_EQ(GURL("http://forced4"), querier.urls()[3].url);
  EXPECT_EQ(base::Time::FromJsTime(4000), querier.urls()[3].last_forced_time);

  ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 4));
}

// More permutations of saving to db.
TEST_F(TopSitesImplTest, RealDatabase) {
  MostVisitedURL url;
  GURL asdf_url("http://asdf.com");
  base::string16 asdf_title(base::ASCIIToUTF16("ASDF"));
  GURL google1_url("http://google.com");
  GURL google2_url("http://google.com/redirect");
  GURL google3_url("http://www.google.com");
  base::string16 google_title(base::ASCIIToUTF16("Google"));
  GURL news_url("http://news.google.com");
  base::string16 news_title(base::ASCIIToUTF16("Google News"));

  url.url = asdf_url;
  url.title = asdf_title;
  url.redirects.push_back(url.url);
  gfx::Image asdf_thumbnail(CreateBitmap(SK_ColorRED));
  ASSERT_TRUE(top_sites()->SetPageThumbnail(
                  asdf_url, asdf_thumbnail, ThumbnailScore()));

  base::Time add_time(base::Time::Now());
  AddPageToHistory(url.url, url.title, url.redirects, add_time);

  RefreshTopSitesAndRecreate();

  {
    TopSitesQuerier querier;
    querier.QueryTopSites(top_sites(), false);

    ASSERT_EQ(1u + GetPrepopulatedPages().size(), querier.urls().size());
    EXPECT_EQ(asdf_url, querier.urls()[0].url);
    EXPECT_EQ(asdf_title, querier.urls()[0].title);
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 1));

    scoped_refptr<base::RefCountedMemory> read_data;
    EXPECT_TRUE(top_sites()->GetPageThumbnail(asdf_url, false, &read_data));
    EXPECT_TRUE(ThumbnailEqualsBytes(asdf_thumbnail, read_data.get()));
  }

  MostVisitedURL url2;
  url2.url = google3_url;
  url2.title = google_title;
  url2.redirects.push_back(google1_url);
  url2.redirects.push_back(google2_url);
  url2.redirects.push_back(google3_url);

  AddPageToHistory(google3_url, url2.title, url2.redirects,
                   add_time - base::TimeDelta::FromMinutes(1));
  // Add google twice so that it becomes the first visited site.
  AddPageToHistory(google3_url, url2.title, url2.redirects,
                   add_time - base::TimeDelta::FromMinutes(2));

  gfx::Image google_thumbnail(CreateBitmap(SK_ColorBLUE));
  ASSERT_TRUE(top_sites()->SetPageThumbnail(
                  url2.url, google_thumbnail, ThumbnailScore()));

  RefreshTopSitesAndRecreate();

  {
    scoped_refptr<base::RefCountedMemory> read_data;
    TopSitesQuerier querier;
    querier.QueryTopSites(top_sites(), false);

    ASSERT_EQ(2u + GetPrepopulatedPages().size(), querier.urls().size());
    EXPECT_EQ(google1_url, querier.urls()[0].url);
    EXPECT_EQ(google_title, querier.urls()[0].title);
    ASSERT_EQ(3u, querier.urls()[0].redirects.size());
    EXPECT_TRUE(top_sites()->GetPageThumbnail(google3_url, false, &read_data));
    EXPECT_TRUE(ThumbnailEqualsBytes(google_thumbnail, read_data.get()));

    EXPECT_EQ(asdf_url, querier.urls()[1].url);
    EXPECT_EQ(asdf_title, querier.urls()[1].title);
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 2));
  }

  gfx::Image weewar_bitmap(CreateBitmap(SK_ColorYELLOW));

  base::Time thumbnail_time(base::Time::Now());
  ThumbnailScore low_score(1.0, true, true, thumbnail_time);
  ThumbnailScore medium_score(0.5, true, true, thumbnail_time);
  ThumbnailScore high_score(0.0, true, true, thumbnail_time);

  // 1. Set to weewar. (Writes the thumbnail to the DB.)
  EXPECT_TRUE(top_sites()->SetPageThumbnail(google3_url,
                                            weewar_bitmap,
                                            medium_score));
  RefreshTopSitesAndRecreate();
  {
    scoped_refptr<base::RefCountedMemory> read_data;
    EXPECT_TRUE(top_sites()->GetPageThumbnail(google3_url, false, &read_data));
    EXPECT_TRUE(ThumbnailEqualsBytes(weewar_bitmap, read_data.get()));
  }

  gfx::Image green_bitmap(CreateBitmap(SK_ColorGREEN));

  // 2. Set to google - low score.
  EXPECT_FALSE(top_sites()->SetPageThumbnail(google3_url,
                                             green_bitmap,
                                             low_score));

  // 3. Set to google - high score.
  EXPECT_TRUE(top_sites()->SetPageThumbnail(google1_url,
                                            green_bitmap,
                                            high_score));

  // Check that the thumbnail was updated.
  RefreshTopSitesAndRecreate();
  {
    scoped_refptr<base::RefCountedMemory> read_data;
    EXPECT_TRUE(top_sites()->GetPageThumbnail(google3_url, false, &read_data));
    EXPECT_FALSE(ThumbnailEqualsBytes(weewar_bitmap, read_data.get()));
    EXPECT_TRUE(ThumbnailEqualsBytes(green_bitmap, read_data.get()));
  }
}

TEST_F(TopSitesImplTest, DeleteNotifications) {
  GURL google1_url("http://google.com");
  GURL google2_url("http://google.com/redirect");
  GURL google3_url("http://www.google.com");
  base::string16 google_title(base::ASCIIToUTF16("Google"));
  GURL news_url("http://news.google.com");
  base::string16 news_title(base::ASCIIToUTF16("Google News"));

  AddPageToHistory(google1_url, google_title);
  AddPageToHistory(news_url, news_title);

  RefreshTopSitesAndRecreate();

  {
    TopSitesQuerier querier;
    querier.QueryTopSites(top_sites(), false);

    ASSERT_EQ(GetPrepopulatedPages().size() + 2, querier.urls().size());
  }

  DeleteURL(news_url);

  // Wait for history to process the deletion.
  WaitForHistory();

  {
    TopSitesQuerier querier;
    querier.QueryTopSites(top_sites(), false);

    ASSERT_EQ(1u + GetPrepopulatedPages().size(), querier.urls().size());
    EXPECT_EQ(google_title, querier.urls()[0].title);
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 1));
  }

  // Now reload. This verifies topsites actually wrote the deletion to disk.
  RefreshTopSitesAndRecreate();

  {
    TopSitesQuerier querier;
    querier.QueryTopSites(top_sites(), false);

    ASSERT_EQ(1u + GetPrepopulatedPages().size(), querier.urls().size());
    EXPECT_EQ(google_title, querier.urls()[0].title);
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 1));
  }

  DeleteURL(google1_url);

  // Wait for history to process the deletion.
  WaitForHistory();

  {
    TopSitesQuerier querier;
    querier.QueryTopSites(top_sites(), false);

    ASSERT_EQ(GetPrepopulatedPages().size(), querier.urls().size());
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 0));
  }

  // Now reload. This verifies topsites actually wrote the deletion to disk.
  RefreshTopSitesAndRecreate();

  {
    TopSitesQuerier querier;
    querier.QueryTopSites(top_sites(), false);

    ASSERT_EQ(GetPrepopulatedPages().size(), querier.urls().size());
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 0));
  }
}

// Verifies that callbacks are notified correctly if requested before top sites
// has loaded.
TEST_F(TopSitesImplTest, NotifyCallbacksWhenLoaded) {
  // Recreate top sites. It won't be loaded now.
  ResetTopSites();

  EXPECT_FALSE(IsTopSitesLoaded());

  TopSitesQuerier querier1;
  TopSitesQuerier querier2;
  TopSitesQuerier querier3;

  // Starts the queries.
  querier1.QueryTopSites(top_sites(), false);
  querier2.QueryTopSites(top_sites(), false);
  querier3.QueryTopSites(top_sites(), false);

  // We shouldn't have gotten a callback.
  EXPECT_EQ(0, querier1.number_of_callbacks());
  EXPECT_EQ(0, querier2.number_of_callbacks());
  EXPECT_EQ(0, querier3.number_of_callbacks());

  // Wait for loading to complete.
  WaitTopSitesLoaded();

  // Now we should have gotten the callbacks.
  EXPECT_EQ(1, querier1.number_of_callbacks());
  EXPECT_EQ(GetPrepopulatedPages().size(), querier1.urls().size());
  EXPECT_EQ(1, querier2.number_of_callbacks());
  EXPECT_EQ(GetPrepopulatedPages().size(), querier2.urls().size());
  EXPECT_EQ(1, querier3.number_of_callbacks());
  EXPECT_EQ(GetPrepopulatedPages().size(), querier3.urls().size());

  // Reset the top sites.
  MostVisitedURLList pages;
  MostVisitedURL url;
  url.url = GURL("http://1.com/");
  url.redirects.push_back(url.url);
  pages.push_back(url);
  url.url = GURL("http://2.com/");
  url.redirects.push_back(url.url);
  pages.push_back(url);
  SetTopSites(pages);

  // Recreate top sites. It won't be loaded now.
  ResetTopSites();

  EXPECT_FALSE(IsTopSitesLoaded());

  TopSitesQuerier querier4;

  // Query again.
  querier4.QueryTopSites(top_sites(), false);

  // We shouldn't have gotten a callback.
  EXPECT_EQ(0, querier4.number_of_callbacks());

  // Wait for loading to complete.
  WaitTopSitesLoaded();

  // Now we should have gotten the callbacks.
  EXPECT_EQ(1, querier4.number_of_callbacks());
  ASSERT_EQ(2u + GetPrepopulatedPages().size(), querier4.urls().size());

  EXPECT_EQ("http://1.com/", querier4.urls()[0].url.spec());
  EXPECT_EQ("http://2.com/", querier4.urls()[1].url.spec());
  ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier4, 2));

  // Reset the top sites again, this time don't reload.
  url.url = GURL("http://3.com/");
  url.redirects.push_back(url.url);
  pages.push_back(url);
  SetTopSites(pages);

  // Query again.
  TopSitesQuerier querier5;
  querier5.QueryTopSites(top_sites(), true);

  EXPECT_EQ(1, querier5.number_of_callbacks());

  ASSERT_EQ(3u + GetPrepopulatedPages().size(), querier5.urls().size());
  EXPECT_EQ("http://1.com/", querier5.urls()[0].url.spec());
  EXPECT_EQ("http://2.com/", querier5.urls()[1].url.spec());
  EXPECT_EQ("http://3.com/", querier5.urls()[2].url.spec());
  ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier5, 3));
}

// Makes sure canceled requests are not notified.
TEST_F(TopSitesImplTest, CancelingRequestsForTopSites) {
  // Recreate top sites. It won't be loaded now.
  ResetTopSites();

  EXPECT_FALSE(IsTopSitesLoaded());

  TopSitesQuerier querier1;
  TopSitesQuerier querier2;

  // Starts the queries.
  querier1.QueryTopSites(top_sites(), false);
  querier2.QueryTopSites(top_sites(), false);

  // We shouldn't have gotten a callback.
  EXPECT_EQ(0, querier1.number_of_callbacks());
  EXPECT_EQ(0, querier2.number_of_callbacks());

  querier2.CancelRequest();

  // Wait for loading to complete.
  WaitTopSitesLoaded();

  // The first callback should succeed.
  EXPECT_EQ(1, querier1.number_of_callbacks());
  EXPECT_EQ(GetPrepopulatedPages().size(), querier1.urls().size());

  // And the canceled callback should not be notified.
  EXPECT_EQ(0, querier2.number_of_callbacks());
}

// Makes sure temporary thumbnails are copied over correctly.
TEST_F(TopSitesImplTest, AddTemporaryThumbnail) {
  GURL unknown_url("http://news.google.com/");
  GURL invalid_url("application://thumb/http://google.com/");
  GURL url1a("http://google.com/");
  GURL url1b("http://www.google.com/");

  // Create a dummy thumbnail.
  gfx::Image thumbnail(CreateBitmap(SK_ColorRED));

  ThumbnailScore medium_score(0.5, true, true, base::Time::Now());

  // Don't store thumbnails for Javascript URLs.
  EXPECT_FALSE(top_sites()->SetPageThumbnail(invalid_url,
                                             thumbnail,
                                             medium_score));
  // Store thumbnails for unknown (but valid) URLs temporarily - calls
  // AddTemporaryThumbnail.
  EXPECT_TRUE(top_sites()->SetPageThumbnail(unknown_url,
                                            thumbnail,
                                            medium_score));

  // We shouldn't get the thumnail back though (the url isn't in to sites yet).
  scoped_refptr<base::RefCountedMemory> out;
  EXPECT_FALSE(top_sites()->GetPageThumbnail(unknown_url, false, &out));
  // But we should be able to get the temporary page thumbnail score.
  ThumbnailScore out_score;
  EXPECT_TRUE(top_sites()->GetTemporaryPageThumbnailScore(unknown_url,
                                                          &out_score));
  EXPECT_TRUE(medium_score.Equals(out_score));

  std::vector<MostVisitedURL> list;

  MostVisitedURL mv;
  mv.url = unknown_url;
  mv.redirects.push_back(mv.url);
  mv.redirects.push_back(url1a);
  mv.redirects.push_back(url1b);
  list.push_back(mv);

  // Update URLs. This should result in using thumbnail.
  SetTopSites(list);

  ASSERT_TRUE(top_sites()->GetPageThumbnail(unknown_url, false, &out));
  EXPECT_TRUE(ThumbnailEqualsBytes(thumbnail, out.get()));
}

// Tests variations of blacklisting without testing prepopulated page
// blacklisting.
TEST_F(TopSitesImplTest, BlacklistingWithoutPrepopulated) {
  MostVisitedURLList pages;
  MostVisitedURL url, url1;
  url.url = GURL("http://bbc.com/");
  url.redirects.push_back(url.url);
  pages.push_back(url);
  url1.url = GURL("http://google.com/");
  url1.redirects.push_back(url1.url);
  pages.push_back(url1);

  SetTopSites(pages);
  EXPECT_FALSE(top_sites()->IsBlacklisted(GURL("http://bbc.com/")));

  // Blacklist google.com.
  top_sites()->AddBlacklistedURL(GURL("http://google.com/"));

  EXPECT_TRUE(top_sites()->HasBlacklistedItems());
  EXPECT_TRUE(top_sites()->IsBlacklisted(GURL("http://google.com/")));
  EXPECT_FALSE(top_sites()->IsBlacklisted(GURL("http://bbc.com/")));

  // Make sure the blacklisted site isn't returned in the results.
  {
    TopSitesQuerier q;
    q.QueryTopSites(top_sites(), true);
    EXPECT_EQ("http://bbc.com/", q.urls()[0].url.spec());
  }

  // Recreate top sites and make sure blacklisted url was correctly read.
  RecreateTopSitesAndBlock();
  {
    TopSitesQuerier q;
    q.QueryTopSites(top_sites(), true);
    EXPECT_EQ("http://bbc.com/", q.urls()[0].url.spec());
  }

  // Mark google as no longer blacklisted.
  top_sites()->RemoveBlacklistedURL(GURL("http://google.com/"));
  EXPECT_FALSE(top_sites()->HasBlacklistedItems());
  EXPECT_FALSE(top_sites()->IsBlacklisted(GURL("http://google.com/")));

  // Make sure google is returned now.
  {
    TopSitesQuerier q;
    q.QueryTopSites(top_sites(), true);
    EXPECT_EQ("http://bbc.com/", q.urls()[0].url.spec());
    EXPECT_EQ("http://google.com/", q.urls()[1].url.spec());
  }

  // Remove all blacklisted sites.
  top_sites()->ClearBlacklistedURLs();
  EXPECT_FALSE(top_sites()->HasBlacklistedItems());

  {
    TopSitesQuerier q;
    q.QueryTopSites(top_sites(), true);
    EXPECT_EQ("http://bbc.com/", q.urls()[0].url.spec());
    EXPECT_EQ("http://google.com/", q.urls()[1].url.spec());
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(q, 2));
  }
}

// Tests variations of blacklisting including blacklisting prepopulated pages.
// This test is disable for Android because Android does not have any
// prepopulated pages.
TEST_F(TopSitesImplTest, BlacklistingWithPrepopulated) {
  MostVisitedURLList pages;
  MostVisitedURL url, url1;
  url.url = GURL("http://bbc.com/");
  url.redirects.push_back(url.url);
  pages.push_back(url);
  url1.url = GURL("http://google.com/");
  url1.redirects.push_back(url1.url);
  pages.push_back(url1);

  SetTopSites(pages);
  EXPECT_FALSE(top_sites()->IsBlacklisted(GURL("http://bbc.com/")));

  // Blacklist google.com.
  top_sites()->AddBlacklistedURL(GURL("http://google.com/"));

  DCHECK_GE(GetPrepopulatedPages().size(), 1u);
  GURL prepopulate_url = GetPrepopulatedPages()[0].most_visited.url;

  EXPECT_TRUE(top_sites()->HasBlacklistedItems());
  EXPECT_TRUE(top_sites()->IsBlacklisted(GURL("http://google.com/")));
  EXPECT_FALSE(top_sites()->IsBlacklisted(GURL("http://bbc.com/")));
  EXPECT_FALSE(top_sites()->IsBlacklisted(prepopulate_url));

  // Make sure the blacklisted site isn't returned in the results.
  {
    TopSitesQuerier q;
    q.QueryTopSites(top_sites(), true);
    ASSERT_EQ(1u + GetPrepopulatedPages().size(), q.urls().size());
    EXPECT_EQ("http://bbc.com/", q.urls()[0].url.spec());
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(q, 1));
  }

  // Recreate top sites and make sure blacklisted url was correctly read.
  RecreateTopSitesAndBlock();
  {
    TopSitesQuerier q;
    q.QueryTopSites(top_sites(), true);
    ASSERT_EQ(1u + GetPrepopulatedPages().size(), q.urls().size());
    EXPECT_EQ("http://bbc.com/", q.urls()[0].url.spec());
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(q, 1));
  }

  // Blacklist one of the prepopulate urls.
  top_sites()->AddBlacklistedURL(prepopulate_url);
  EXPECT_TRUE(top_sites()->HasBlacklistedItems());

  // Make sure the blacklisted prepopulate url isn't returned.
  {
    TopSitesQuerier q;
    q.QueryTopSites(top_sites(), true);
    ASSERT_EQ(1u + GetPrepopulatedPages().size() - 1, q.urls().size());
    EXPECT_EQ("http://bbc.com/", q.urls()[0].url.spec());
    for (size_t i = 1; i < q.urls().size(); ++i)
      EXPECT_NE(prepopulate_url.spec(), q.urls()[i].url.spec());
  }

  // Mark google as no longer blacklisted.
  top_sites()->RemoveBlacklistedURL(GURL("http://google.com/"));
  EXPECT_TRUE(top_sites()->HasBlacklistedItems());
  EXPECT_FALSE(top_sites()->IsBlacklisted(GURL("http://google.com/")));

  // Make sure google is returned now.
  {
    TopSitesQuerier q;
    q.QueryTopSites(top_sites(), true);
    ASSERT_EQ(2u + GetPrepopulatedPages().size() - 1, q.urls().size());
    EXPECT_EQ("http://bbc.com/", q.urls()[0].url.spec());
    EXPECT_EQ("http://google.com/", q.urls()[1].url.spec());
    // Android has only one prepopulated page which has been blacklisted, so
    // only 2 urls are returned.
    if (q.urls().size() > 2)
      EXPECT_NE(prepopulate_url.spec(), q.urls()[2].url.spec());
    else
      EXPECT_EQ(1u, GetPrepopulatedPages().size());
  }

  // Remove all blacklisted sites.
  top_sites()->ClearBlacklistedURLs();
  EXPECT_FALSE(top_sites()->HasBlacklistedItems());

  {
    TopSitesQuerier q;
    q.QueryTopSites(top_sites(), true);
    ASSERT_EQ(2u + GetPrepopulatedPages().size(), q.urls().size());
    EXPECT_EQ("http://bbc.com/", q.urls()[0].url.spec());
    EXPECT_EQ("http://google.com/", q.urls()[1].url.spec());
    ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(q, 2));
  }
}

// Makes sure prepopulated pages exist.
TEST_F(TopSitesImplTest, AddPrepopulatedPages) {
  TopSitesQuerier q;
  q.QueryTopSites(top_sites(), true);
  EXPECT_EQ(GetPrepopulatedPages().size(), q.urls().size());
  ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(q, 0));

  MostVisitedURLList pages = q.urls();
  EXPECT_FALSE(AddPrepopulatedPages(&pages));

  EXPECT_EQ(GetPrepopulatedPages().size(), pages.size());
  q.set_urls(pages);
  ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(q, 0));
}

// Ensure calling SetTopSites with forced sites already in the DB works.
// This test both eviction and
TEST_F(TopSitesImplTest, SetForcedTopSites) {
  // Create forced elements in old URL list.
  MostVisitedURLList old_url_list;
  AppendForcedMostVisitedURL(GURL("http://oldforced/0"), 1000, &old_url_list);
  AppendForcedMostVisitedURL(GURL("http://oldforced/1"), 4000, &old_url_list);
  AppendForcedMostVisitedURL(GURL("http://oldforced/2"), 7000, &old_url_list);
  AppendForcedMostVisitedURL(GURL("http://oldforced/3"), 10000, &old_url_list);
  AppendForcedMostVisitedURL(GURL("http://oldforced/4"), 11000, &old_url_list);
  AppendForcedMostVisitedURL(GURL("http://oldforced/5"), 12000, &old_url_list);
  AppendForcedMostVisitedURL(GURL("http://oldforced/6"), 13000, &old_url_list);

  const size_t kNumOldForcedURLs = old_url_list.size();

  // Create forced elements in new URL list.
  MostVisitedURLList new_url_list;
  AppendForcedMostVisitedURL(GURL("http://newforced/0"), 2000, &new_url_list);
  AppendForcedMostVisitedURL(GURL("http://newforced/1"), 3000, &new_url_list);
  AppendForcedMostVisitedURL(GURL("http://newforced/2"), 5000, &new_url_list);
  AppendForcedMostVisitedURL(GURL("http://newforced/3"), 6000, &new_url_list);
  AppendForcedMostVisitedURL(GURL("http://newforced/4"), 8000, &new_url_list);
  AppendForcedMostVisitedURL(GURL("http://newforced/5"), 9000, &new_url_list);
  AppendForcedMostVisitedURL(GURL("http://newforced/6"), 14000, &new_url_list);
  AppendForcedMostVisitedURL(GURL("http://newforced/7"), 15000, &new_url_list);
  AppendForcedMostVisitedURL(GURL("http://newforced/8"), 16000, &new_url_list);

  const size_t kNonForcedTopSitesCount = TopSitesImpl::kNonForcedTopSitesNumber;
  const size_t kForcedTopSitesCount = TopSitesImpl::kForcedTopSitesNumber;

  // Setup a number non-forced URLs in both old and new list.
  for (size_t i = 0; i < kNonForcedTopSitesCount; ++i) {
    std::ostringstream url;
    url << "http://oldnonforced/" << i;
    AppendMostVisitedURL(GURL(url.str()), &old_url_list);
    url.str("");
    url << "http://newnonforced/" << i;
    AppendMostVisitedURL(GURL(url.str()), &new_url_list);
  }

  // Set the initial list of URLs.
  SetTopSites(old_url_list);

  TopSitesQuerier querier;
  // Query only non-forced URLs first.
  querier.QueryTopSites(top_sites(), false);
  ASSERT_EQ(kNonForcedTopSitesCount, querier.urls().size());

  // Check first URL.
  EXPECT_EQ("http://oldnonforced/0", querier.urls()[0].url.spec());

  // Query all URLs.
  querier.QueryAllTopSites(top_sites(), false, true);
  EXPECT_EQ(kNumOldForcedURLs + kNonForcedTopSitesCount, querier.urls().size());

  // Check first URLs.
  EXPECT_EQ("http://oldforced/0", querier.urls()[0].url.spec());
  EXPECT_EQ("http://oldnonforced/0",
            querier.urls()[kNumOldForcedURLs].url.spec());

  // Set the new list of URLs.
  SetTopSites(new_url_list);

  // Query all URLs.
  querier.QueryAllTopSites(top_sites(), false, true);

  // We should have reached the maximum of forced URLs.
  ASSERT_EQ(kForcedTopSitesCount + kNonForcedTopSitesCount,
            querier.urls().size());

  // Check forced URLs. They follow the order of timestamps above, smaller
  // timestamps since they were evicted.
  EXPECT_EQ("http://oldforced/2", querier.urls()[0].url.spec());
  EXPECT_EQ(7000, querier.urls()[0].last_forced_time.ToJsTime());
  EXPECT_EQ("http://newforced/4", querier.urls()[1].url.spec());
  EXPECT_EQ(8000, querier.urls()[1].last_forced_time.ToJsTime());
  EXPECT_EQ("http://newforced/5", querier.urls()[2].url.spec());
  EXPECT_EQ(9000, querier.urls()[2].last_forced_time.ToJsTime());
  EXPECT_EQ("http://oldforced/3", querier.urls()[3].url.spec());
  EXPECT_EQ(10000, querier.urls()[3].last_forced_time.ToJsTime());
  EXPECT_EQ("http://oldforced/4", querier.urls()[4].url.spec());
  EXPECT_EQ(11000, querier.urls()[4].last_forced_time.ToJsTime());
  EXPECT_EQ("http://oldforced/5", querier.urls()[5].url.spec());
  EXPECT_EQ(12000, querier.urls()[5].last_forced_time.ToJsTime());
  EXPECT_EQ("http://oldforced/6", querier.urls()[6].url.spec());
  EXPECT_EQ(13000, querier.urls()[6].last_forced_time.ToJsTime());
  EXPECT_EQ("http://newforced/6", querier.urls()[7].url.spec());
  EXPECT_EQ(14000, querier.urls()[7].last_forced_time.ToJsTime());
  EXPECT_EQ("http://newforced/7", querier.urls()[8].url.spec());
  EXPECT_EQ(15000, querier.urls()[8].last_forced_time.ToJsTime());
  EXPECT_EQ("http://newforced/8", querier.urls()[9].url.spec());
  EXPECT_EQ(16000, querier.urls()[9].last_forced_time.ToJsTime());

  // Check first and last non-forced URLs.
  EXPECT_EQ("http://newnonforced/0",
            querier.urls()[kForcedTopSitesCount].url.spec());
  EXPECT_TRUE(querier.urls()[kForcedTopSitesCount].last_forced_time.is_null());

  size_t non_forced_end_index = querier.urls().size() - 1;
  EXPECT_EQ("http://newnonforced/9",
            querier.urls()[non_forced_end_index].url.spec());
  EXPECT_TRUE(querier.urls()[non_forced_end_index].last_forced_time.is_null());
}

TEST_F(TopSitesImplTest, SetForcedTopSitesWithCollisions) {
  // Setup an old URL list in order to generate some collisions.
  MostVisitedURLList old_url_list;
  AppendForcedMostVisitedURL(GURL("http://url/0"), 1000, &old_url_list);
  // The following three will be evicted.
  AppendForcedMostVisitedURL(GURL("http://collision/0"), 4000, &old_url_list);
  AppendForcedMostVisitedURL(GURL("http://collision/1"), 6000, &old_url_list);
  AppendForcedMostVisitedURL(GURL("http://collision/2"), 7000, &old_url_list);
  // The following is evicted since all non-forced URLs are, therefore it
  // doesn't cause a collision.
  AppendMostVisitedURL(GURL("http://noncollision/0"), &old_url_list);
  SetTopSites(old_url_list);

  // Setup a new URL list that will cause collisions.
  MostVisitedURLList new_url_list;
  AppendForcedMostVisitedURL(GURL("http://collision/1"), 2000, &new_url_list);
  AppendForcedMostVisitedURL(GURL("http://url/2"), 3000, &new_url_list);
  AppendForcedMostVisitedURL(GURL("http://collision/0"), 5000, &new_url_list);
  AppendForcedMostVisitedURL(GURL("http://noncollision/0"), 9000,
                             &new_url_list);
  AppendMostVisitedURL(GURL("http://collision/2"), &new_url_list);
  AppendMostVisitedURL(GURL("http://url/3"), &new_url_list);
  SetTopSites(new_url_list);

  // Query all URLs.
  TopSitesQuerier querier;
  querier.QueryAllTopSites(top_sites(), false, true);

  // Check URLs. When collision occurs, the incoming one is always preferred.
  ASSERT_EQ(7u + GetPrepopulatedPages().size(), querier.urls().size());
  EXPECT_EQ("http://url/0", querier.urls()[0].url.spec());
  EXPECT_EQ(1000u, querier.urls()[0].last_forced_time.ToJsTime());
  EXPECT_EQ("http://collision/1", querier.urls()[1].url.spec());
  EXPECT_EQ(2000u, querier.urls()[1].last_forced_time.ToJsTime());
  EXPECT_EQ("http://url/2", querier.urls()[2].url.spec());
  EXPECT_EQ(3000u, querier.urls()[2].last_forced_time.ToJsTime());
  EXPECT_EQ("http://collision/0", querier.urls()[3].url.spec());
  EXPECT_EQ(5000u, querier.urls()[3].last_forced_time.ToJsTime());
  EXPECT_EQ("http://noncollision/0", querier.urls()[4].url.spec());
  EXPECT_EQ(9000u, querier.urls()[4].last_forced_time.ToJsTime());
  EXPECT_EQ("http://collision/2", querier.urls()[5].url.spec());
  EXPECT_TRUE(querier.urls()[5].last_forced_time.is_null());
  EXPECT_EQ("http://url/3", querier.urls()[6].url.spec());
  EXPECT_TRUE(querier.urls()[6].last_forced_time.is_null());
  ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 7));
}

TEST_F(TopSitesImplTest, SetTopSitesIdentical) {
  // Set the initial list of URLs.
  MostVisitedURLList url_list;
  AppendForcedMostVisitedURL(GURL("http://url/0"), 1000, &url_list);
  AppendMostVisitedURL(GURL("http://url/1"), &url_list);
  AppendMostVisitedURL(GURL("http://url/2"), &url_list);
  SetTopSites(url_list);

  // Set the new list of URLs to be exactly the same.
  SetTopSites(MostVisitedURLList(url_list));

  // Query all URLs.
  TopSitesQuerier querier;
  querier.QueryAllTopSites(top_sites(), false, true);

  // Check URLs. When collision occurs, the incoming one is always preferred.
  ASSERT_EQ(3u + GetPrepopulatedPages().size(), querier.urls().size());
  EXPECT_EQ("http://url/0", querier.urls()[0].url.spec());
  EXPECT_EQ(1000u, querier.urls()[0].last_forced_time.ToJsTime());
  EXPECT_EQ("http://url/1", querier.urls()[1].url.spec());
  EXPECT_EQ("http://url/2", querier.urls()[2].url.spec());
  ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 3));
}

TEST_F(TopSitesImplTest, SetTopSitesWithAlreadyExistingForcedURLs) {
  // Set the initial list of URLs.
  MostVisitedURLList old_url_list;
  AppendForcedMostVisitedURL(GURL("http://url/0/redir"), 1000, &old_url_list);
  AppendForcedMostVisitedURL(GURL("http://url/1"), 2000, &old_url_list);
  SetTopSites(old_url_list);

  // Setup a new URL list that will cause collisions.
  MostVisitedURLList new_url_list;
  AppendMostVisitedURLWithRedirect(GURL("http://url/0/redir"),
                                   GURL("http://url/0"), &new_url_list);
  AppendMostVisitedURL(GURL("http://url/1"), &new_url_list);
  SetTopSites(new_url_list);

  // Query all URLs.
  TopSitesQuerier querier;
  querier.QueryAllTopSites(top_sites(), false, true);

  // Check URLs. When collision occurs, the non-forced one is always preferred.
  ASSERT_EQ(2u + GetPrepopulatedPages().size(), querier.urls().size());
  EXPECT_EQ("http://url/0", querier.urls()[0].url.spec());
  EXPECT_EQ("http://url/0/redir", querier.urls()[0].redirects[0].spec());
  EXPECT_TRUE(querier.urls()[0].last_forced_time.is_null());
  EXPECT_EQ("http://url/1", querier.urls()[1].url.spec());
  EXPECT_TRUE(querier.urls()[1].last_forced_time.is_null());
  ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 2));
}

TEST_F(TopSitesImplTest, AddForcedURL) {
  // Set the initial list of URLs.
  MostVisitedURLList url_list;
  AppendForcedMostVisitedURL(GURL("http://forced/0"), 2000, &url_list);
  AppendForcedMostVisitedURL(GURL("http://forced/1"), 4000, &url_list);
  AppendMostVisitedURL(GURL("http://nonforced/0"), &url_list);
  AppendMostVisitedURL(GURL("http://nonforced/1"), &url_list);
  AppendMostVisitedURL(GURL("http://nonforced/2"), &url_list);
  SetTopSites(url_list);

  // Add forced sites here and there to exercise a couple of cases.
  EXPECT_TRUE(AddForcedURL(GURL("http://forced/2"),
                           base::Time::FromJsTime(5000)));
  EXPECT_TRUE(AddForcedURL(GURL("http://forced/3"),
                           base::Time::FromJsTime(1000)));
  EXPECT_TRUE(AddForcedURL(GURL("http://forced/4"),
                           base::Time::FromJsTime(3000)));

  // Check URLs.
  TopSitesQuerier querier;
  querier.QueryAllTopSites(top_sites(), false, true);
  ASSERT_EQ(8u + GetPrepopulatedPages().size(), querier.urls().size());
  EXPECT_EQ("http://forced/3", querier.urls()[0].url.spec());
  EXPECT_EQ(1000u, querier.urls()[0].last_forced_time.ToJsTime());
  EXPECT_EQ("http://forced/0", querier.urls()[1].url.spec());
  EXPECT_EQ(2000u, querier.urls()[1].last_forced_time.ToJsTime());
  EXPECT_EQ("http://forced/4", querier.urls()[2].url.spec());
  EXPECT_EQ(3000u, querier.urls()[2].last_forced_time.ToJsTime());
  EXPECT_EQ("http://forced/1", querier.urls()[3].url.spec());
  EXPECT_EQ(4000u, querier.urls()[3].last_forced_time.ToJsTime());
  EXPECT_EQ("http://forced/2", querier.urls()[4].url.spec());
  EXPECT_EQ(5000u, querier.urls()[4].last_forced_time.ToJsTime());
  EXPECT_EQ("http://nonforced/0", querier.urls()[5].url.spec());
  EXPECT_TRUE(querier.urls()[5].last_forced_time.is_null());
  EXPECT_EQ("http://nonforced/1", querier.urls()[6].url.spec());
  EXPECT_TRUE(querier.urls()[6].last_forced_time.is_null());
  EXPECT_EQ("http://nonforced/2", querier.urls()[7].url.spec());
  EXPECT_TRUE(querier.urls()[7].last_forced_time.is_null());
  ASSERT_NO_FATAL_FAILURE(ContainsPrepopulatePages(querier, 8));

  // Add some collisions with forced and non-forced. Non-forced URLs are never
  // expected to move.
  EXPECT_TRUE(AddForcedURL(GURL("http://forced/3"),
                           base::Time::FromJsTime(4000)));
  EXPECT_TRUE(AddForcedURL(GURL("http://forced/1"),
                            base::Time::FromJsTime(1000)));
  EXPECT_FALSE(AddForcedURL(GURL("http://nonforced/0"),
                            base::Time::FromJsTime(6000)));

  // Check relevant URLs.
  querier.QueryAllTopSites(top_sites(), false, true);
  ASSERT_EQ(8u + GetPrepopulatedPages().size(), querier.urls().size());
  EXPECT_EQ("http://forced/1", querier.urls()[0].url.spec());
  EXPECT_EQ(1000u, querier.urls()[0].last_forced_time.ToJsTime());
  EXPECT_EQ("http://forced/3", querier.urls()[3].url.spec());
  EXPECT_EQ(4000u, querier.urls()[3].last_forced_time.ToJsTime());
  EXPECT_EQ("http://nonforced/0", querier.urls()[5].url.spec());
  EXPECT_TRUE(querier.urls()[5].last_forced_time.is_null());

  // Add a timestamp collision and make sure things don't break.
  EXPECT_TRUE(AddForcedURL(GURL("http://forced/5"),
                           base::Time::FromJsTime(4000)));
  querier.QueryAllTopSites(top_sites(), false, true);
  ASSERT_EQ(9u + GetPrepopulatedPages().size(), querier.urls().size());
  EXPECT_EQ(4000u, querier.urls()[3].last_forced_time.ToJsTime());
  EXPECT_EQ(4000u, querier.urls()[4].last_forced_time.ToJsTime());
  // We don't care which order they get sorted in.
  if (querier.urls()[3].url.spec() == "http://forced/3") {
    EXPECT_EQ("http://forced/3", querier.urls()[3].url.spec());
    EXPECT_EQ("http://forced/5", querier.urls()[4].url.spec());
  } else {
    EXPECT_EQ("http://forced/5", querier.urls()[3].url.spec());
    EXPECT_EQ("http://forced/3", querier.urls()[4].url.spec());
  }

  // Make sure the thumbnail is not lost when the timestamp is updated.
  gfx::Image red_thumbnail(CreateBitmap(SK_ColorRED));
  ASSERT_TRUE(top_sites()->SetPageThumbnail(
                  GURL("http://forced/5"), red_thumbnail, ThumbnailScore()));

  // Get the original thumbnail for later comparison. Some compression can
  // happen in |top_sites| and we don't want to depend on that.
  scoped_refptr<base::RefCountedMemory> orig_thumbnail_data;
  ASSERT_TRUE(top_sites()->GetPageThumbnail(GURL("http://forced/5"), false,
                                            &orig_thumbnail_data));

  EXPECT_TRUE(AddForcedURL(GURL("http://forced/5"),
                           base::Time::FromJsTime(6000)));

  // Ensure the thumbnail is still there even if the timestamp changed.
  querier.QueryAllTopSites(top_sites(), false, true);
  EXPECT_EQ("http://forced/5", querier.urls()[5].url.spec());
  EXPECT_EQ(6000u, querier.urls()[5].last_forced_time.ToJsTime());
  scoped_refptr<base::RefCountedMemory> thumbnail_data;
  ASSERT_TRUE(top_sites()->GetPageThumbnail(GURL("http://forced/5"), false,
                                            &thumbnail_data));
  ASSERT_TRUE(
      ThumbnailsAreEqual(orig_thumbnail_data.get(), thumbnail_data.get()));
}

}  // namespace history
