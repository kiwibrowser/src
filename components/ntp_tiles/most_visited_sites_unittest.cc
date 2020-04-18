// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_tiles/most_visited_sites.h"

#include <stddef.h>

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/callback_list.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/history/core/browser/top_sites.h"
#include "components/history/core/browser/top_sites_observer.h"
#include "components/ntp_tiles/constants.h"
#include "components/ntp_tiles/icon_cacher.h"
#include "components/ntp_tiles/json_unsafe_parser.h"
#include "components/ntp_tiles/popular_sites_impl.h"
#include "components/ntp_tiles/pref_names.h"
#include "components/ntp_tiles/section_type.h"
#include "components/ntp_tiles/switches.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_tiles {

// Defined for googletest. Must be defined in the same namespace.
void PrintTo(const NTPTile& tile, std::ostream* os) {
  *os << "{\"" << tile.title << "\", \"" << tile.url << "\", "
      << static_cast<int>(tile.source) << "}";
}

namespace {

using history::MostVisitedURL;
using history::MostVisitedURLList;
using history::TopSites;
using suggestions::ChromeSuggestion;
using suggestions::SuggestionsProfile;
using suggestions::SuggestionsService;
using testing::AtLeast;
using testing::AllOf;
using testing::AnyNumber;
using testing::ByMove;
using testing::Contains;
using testing::ElementsAre;
using testing::Eq;
using testing::Ge;
using testing::InSequence;
using testing::Invoke;
using testing::IsEmpty;
using testing::Key;
using testing::Mock;
using testing::Not;
using testing::Pair;
using testing::Return;
using testing::ReturnRef;
using testing::SaveArg;
using testing::SizeIs;
using testing::StrictMock;
using testing::_;

const char kHomePageUrl[] = "http://ho.me/";
const char kHomePageTitle[] = "Home";

std::string PrintTile(const std::string& title,
                      const std::string& url,
                      TileSource source) {
  return std::string("has title \"") + title + std::string("\" and url \"") +
         url + std::string("\" and source ") +
         testing::PrintToString(static_cast<int>(source));
}

MATCHER_P3(MatchesTile, title, url, source, PrintTile(title, url, source)) {
  return arg.title == base::ASCIIToUTF16(title) && arg.url == GURL(url) &&
         arg.source == source;
}

MATCHER_P3(FirstPersonalizedTileIs,
           title,
           url,
           source,
           std::string("first tile ") + PrintTile(title, url, source)) {
  if (arg.count(SectionType::PERSONALIZED) == 0) {
    return false;
  }
  const NTPTilesVector& tiles = arg.at(SectionType::PERSONALIZED);
  return !tiles.empty() && tiles[0].title == base::ASCIIToUTF16(title) &&
         tiles[0].url == GURL(url) && tiles[0].source == source;
}

// testing::InvokeArgument<N> does not work with base::Callback, fortunately
// gmock makes it simple to create action templates that do for the various
// possible numbers of arguments.
ACTION_TEMPLATE(InvokeCallbackArgument,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(p0)) {
  std::get<k>(args).Run(p0);
}

NTPTile MakeTile(const std::string& title,
                 const std::string& url,
                 TileSource source) {
  NTPTile tile;
  tile.title = base::ASCIIToUTF16(title);
  tile.url = GURL(url);
  tile.source = source;
  return tile;
}

ChromeSuggestion MakeSuggestion(const std::string& title,
                                const std::string& url) {
  ChromeSuggestion suggestion;
  suggestion.set_title(title);
  suggestion.set_url(url);
  return suggestion;
}

SuggestionsProfile MakeProfile(
    const std::vector<ChromeSuggestion>& suggestions) {
  SuggestionsProfile profile;
  for (const ChromeSuggestion& suggestion : suggestions) {
    *profile.add_suggestions() = suggestion;
  }
  return profile;
}

MostVisitedURL MakeMostVisitedURL(const std::string& title,
                                  const std::string& url) {
  MostVisitedURL result;
  result.title = base::ASCIIToUTF16(title);
  result.url = GURL(url);
  return result;
}

class MockTopSites : public TopSites {
 public:
  MOCK_METHOD0(ShutdownOnUIThread, void());
  MOCK_METHOD3(SetPageThumbnail,
               bool(const GURL& url,
                    const gfx::Image& thumbnail,
                    const ThumbnailScore& score));
  MOCK_METHOD3(SetPageThumbnailToJPEGBytes,
               bool(const GURL& url,
                    const base::RefCountedMemory* memory,
                    const ThumbnailScore& score));
  MOCK_METHOD2(GetMostVisitedURLs,
               void(const GetMostVisitedURLsCallback& callback,
                    bool include_forced_urls));
  MOCK_METHOD3(GetPageThumbnail,
               bool(const GURL& url,
                    bool prefix_match,
                    scoped_refptr<base::RefCountedMemory>* bytes));
  MOCK_METHOD2(GetPageThumbnailScore,
               bool(const GURL& url, ThumbnailScore* score));
  MOCK_METHOD2(GetTemporaryPageThumbnailScore,
               bool(const GURL& url, ThumbnailScore* score));
  MOCK_METHOD0(SyncWithHistory, void());
  MOCK_CONST_METHOD0(HasBlacklistedItems, bool());
  MOCK_METHOD1(AddBlacklistedURL, void(const GURL& url));
  MOCK_METHOD1(RemoveBlacklistedURL, void(const GURL& url));
  MOCK_METHOD1(IsBlacklisted, bool(const GURL& url));
  MOCK_METHOD0(ClearBlacklistedURLs, void());
  MOCK_METHOD0(StartQueryForMostVisited, base::CancelableTaskTracker::TaskId());
  MOCK_METHOD1(IsKnownURL, bool(const GURL& url));
  MOCK_CONST_METHOD1(GetCanonicalURLString,
                     const std::string&(const GURL& url));
  MOCK_METHOD0(IsNonForcedFull, bool());
  MOCK_METHOD0(IsForcedFull, bool());
  MOCK_CONST_METHOD0(loaded, bool());
  MOCK_METHOD0(GetPrepopulatedPages, history::PrepopulatedPageList());
  MOCK_METHOD2(AddForcedURL, bool(const GURL& url, const base::Time& time));
  MOCK_METHOD1(OnNavigationCommitted, void(const GURL& url));

  // Publicly expose notification to observers, since the implementation cannot
  // be overriden.
  using TopSites::NotifyTopSitesChanged;

 protected:
  ~MockTopSites() override = default;
};

class MockSuggestionsService : public SuggestionsService {
 public:
  MOCK_METHOD0(FetchSuggestionsData, bool());
  MOCK_CONST_METHOD0(GetSuggestionsDataFromCache,
                     base::Optional<SuggestionsProfile>());
  MOCK_METHOD1(AddCallback,
               std::unique_ptr<ResponseCallbackList::Subscription>(
                   const ResponseCallback& callback));
  MOCK_METHOD2(GetPageThumbnail,
               void(const GURL& url, const BitmapCallback& callback));
  MOCK_METHOD3(GetPageThumbnailWithURL,
               void(const GURL& url,
                    const GURL& thumbnail_url,
                    const BitmapCallback& callback));
  MOCK_METHOD1(BlacklistURL, bool(const GURL& candidate_url));
  MOCK_METHOD1(UndoBlacklistURL, bool(const GURL& url));
  MOCK_METHOD0(ClearBlacklist, void());
};

class MockMostVisitedSitesObserver : public MostVisitedSites::Observer {
 public:
  MOCK_METHOD1(OnURLsAvailable,
               void(const std::map<SectionType, NTPTilesVector>& sections));
  MOCK_METHOD1(OnIconMadeAvailable, void(const GURL& site_url));
};

class FakeHomePageClient : public MostVisitedSites::HomePageClient {
 public:
  FakeHomePageClient()
      : home_page_enabled_(false),
        ntp_is_homepage_(false),
        home_page_url_(kHomePageUrl) {}
  ~FakeHomePageClient() override {}

  bool IsHomePageEnabled() const override { return home_page_enabled_; }

  bool IsNewTabPageUsedAsHomePage() const override { return ntp_is_homepage_; }

  GURL GetHomePageUrl() const override { return home_page_url_; }

  void QueryHomePageTitle(TitleCallback title_callback) override {
    std::move(title_callback).Run(home_page_title_);
  }

  void SetHomePageEnabled(bool home_page_enabled) {
    home_page_enabled_ = home_page_enabled;
  }

  void SetNtpIsHomePage(bool ntp_is_homepage) {
    ntp_is_homepage_ = ntp_is_homepage;
  }

  void SetHomePageUrl(GURL home_page_url) { home_page_url_ = home_page_url; }

  void SetHomePageTitle(const base::Optional<base::string16>& home_page_title) {
    home_page_title_ = home_page_title;
  }

 private:
  bool home_page_enabled_;
  bool ntp_is_homepage_;
  GURL home_page_url_;
  base::Optional<base::string16> home_page_title_;
};

class MockIconCacher : public IconCacher {
 public:
  MOCK_METHOD3(StartFetchPopularSites,
               void(PopularSites::Site site,
                    const base::Closure& icon_available,
                    const base::Closure& preliminary_icon_available));
  MOCK_METHOD2(StartFetchMostLikely,
               void(const GURL& page_url, const base::Closure& icon_available));
};

class PopularSitesFactoryForTest {
 public:
  PopularSitesFactoryForTest(
      sync_preferences::TestingPrefServiceSyncable* pref_service)
      : prefs_(pref_service),
        url_fetcher_factory_(/*default_factory=*/nullptr),
        url_request_context_(new net::TestURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get())) {
    PopularSitesImpl::RegisterProfilePrefs(pref_service->registry());
  }

  void SeedWithSampleData() {
    prefs_->SetString(prefs::kPopularSitesOverrideCountry, "IN");
    prefs_->SetString(prefs::kPopularSitesOverrideVersion, "5");

    url_fetcher_factory_.ClearFakeResponses();
    url_fetcher_factory_.SetFakeResponse(
        GURL("https://www.gstatic.com/chrome/ntp/suggested_sites_IN_5.json"),
        R"([{
              "title": "PopularSite1",
              "url": "http://popularsite1/",
              "favicon_url": "http://popularsite1/favicon.ico"
            },
            {
              "title": "PopularSite2",
              "url": "http://popularsite2/",
              "favicon_url": "http://popularsite2/favicon.ico"
            },
           ])",
        net::HTTP_OK, net::URLRequestStatus::SUCCESS);

    url_fetcher_factory_.SetFakeResponse(
        GURL("https://www.gstatic.com/chrome/ntp/suggested_sites_US_5.json"),
        R"([{
              "title": "ESPN",
              "url": "http://www.espn.com",
              "favicon_url": "http://www.espn.com/favicon.ico"
            }, {
              "title": "Mobile",
              "url": "http://www.mobile.de",
              "favicon_url": "http://www.mobile.de/favicon.ico"
            }, {
              "title": "Google News",
              "url": "http://news.google.com",
              "favicon_url": "http://news.google.com/favicon.ico"
            },
           ])",
        net::HTTP_OK, net::URLRequestStatus::SUCCESS);

    url_fetcher_factory_.SetFakeResponse(
        GURL("https://www.gstatic.com/chrome/ntp/suggested_sites_IN_6.json"),
        R"([{
              "section": 1, // PERSONALIZED
              "sites": [{
                  "title": "PopularSite1",
                  "url": "http://popularsite1/",
                  "favicon_url": "http://popularsite1/favicon.ico"
                },
                {
                  "title": "PopularSite2",
                  "url": "http://popularsite2/",
                  "favicon_url": "http://popularsite2/favicon.ico"
                },
               ]
            },
            {
                "section": 4,  // NEWS
                "sites": [{
                    "large_icon_url": "https://news.google.com/icon.ico",
                    "title": "Google News",
                    "url": "https://news.google.com/"
                },
                {
                    "favicon_url": "https://news.google.com/icon.ico",
                    "title": "Google News Germany",
                    "url": "https://news.google.de/"
                }]
            },
            {
                "section": 2,  // SOCIAL
                "sites": [{
                    "large_icon_url": "https://ssl.gstatic.com/icon.png",
                    "title": "Google+",
                    "url": "https://plus.google.com/"
                }]
            },
            {
                "section": 3,  // ENTERTAINMENT
                "sites": [
                    // Intentionally empty site list.
                ]
            }
        ])",
        net::HTTP_OK, net::URLRequestStatus::SUCCESS);
  }

  std::unique_ptr<PopularSites> New() {
    return std::make_unique<PopularSitesImpl>(
        prefs_,
        /*template_url_service=*/nullptr,
        /*variations_service=*/nullptr, url_request_context_.get(),
        base::Bind(JsonUnsafeParser::Parse));
  }

 private:
  PrefService* prefs_;
  net::FakeURLFetcherFactory url_fetcher_factory_;
  scoped_refptr<net::TestURLRequestContextGetter> url_request_context_;
};

// CallbackList-like container without Subscription, mimicking the
// implementation in TopSites (which doesn't use base::CallbackList).
class TopSitesCallbackList {
 public:
  // Second argument declared to match the signature of GetMostVisitedURLs().
  void Add(const TopSites::GetMostVisitedURLsCallback& callback,
           testing::Unused) {
    callbacks_.push_back(callback);
  }

  void ClearAndNotify(const MostVisitedURLList& list) {
    std::vector<TopSites::GetMostVisitedURLsCallback> callbacks;
    callbacks.swap(callbacks_);
    for (const auto& callback : callbacks) {
      callback.Run(list);
    }
  }

  bool empty() const { return callbacks_.empty(); }

 private:
  std::vector<TopSites::GetMostVisitedURLsCallback> callbacks_;
};

}  // namespace

// Param specifies whether Popular Sites is enabled via variations.
class MostVisitedSitesTest : public ::testing::TestWithParam<bool> {
 protected:
  MostVisitedSitesTest()
      : popular_sites_factory_(&pref_service_),
        mock_top_sites_(new StrictMock<MockTopSites>()) {
    MostVisitedSites::RegisterProfilePrefs(pref_service_.registry());

    // Disable FaviconServer in most tests and override in specific tests.
    if (IsPopularSitesFeatureEnabled()) {
      feature_list_.InitWithFeatures(
          /*enabled_features=*/{kUsePopularSitesSuggestions},
          /*disabled_features=*/{kNtpMostLikelyFaviconsFromServerFeature});
      popular_sites_factory_.SeedWithSampleData();
    } else {
      feature_list_.InitWithFeatures(
          /*enabled_features=*/{},
          /*disabled_features=*/{kUsePopularSitesSuggestions,
                                 kNtpMostLikelyFaviconsFromServerFeature});
    }

    RecreateMostVisitedSites();
  }

  void RecreateMostVisitedSites() {
    // We use StrictMock to make sure the object is not used unless Popular
    // Sites is enabled.
    auto icon_cacher = std::make_unique<StrictMock<MockIconCacher>>();
    icon_cacher_ = icon_cacher.get();

    if (IsPopularSitesFeatureEnabled()) {
      // Populate Popular Sites' internal cache by mimicking a past usage of
      // PopularSitesImpl.
      auto tmp_popular_sites = popular_sites_factory_.New();
      base::RunLoop loop;
      bool save_success = false;
      tmp_popular_sites->MaybeStartFetch(
          /*force_download=*/true,
          base::Bind(
              [](bool* save_success, base::RunLoop* loop, bool success) {
                *save_success = success;
                loop->Quit();
              },
              &save_success, &loop));
      loop.Run();
      EXPECT_TRUE(save_success);

      // With PopularSites enabled, blacklist is exercised.
      EXPECT_CALL(*mock_top_sites_, IsBlacklisted(_))
          .WillRepeatedly(Return(false));
      // Mock icon cacher never replies, and we also don't verify whether the
      // code uses it correctly.
      EXPECT_CALL(*icon_cacher, StartFetchPopularSites(_, _, _))
          .Times(AtLeast(0));
    }

    EXPECT_CALL(*icon_cacher, StartFetchMostLikely(_, _)).Times(AtLeast(0));

    most_visited_sites_ = std::make_unique<MostVisitedSites>(
        &pref_service_, mock_top_sites_, &mock_suggestions_service_,
        popular_sites_factory_.New(), std::move(icon_cacher),
        /*supervisor=*/nullptr);
  }

  bool IsPopularSitesFeatureEnabled() const { return GetParam(); }

  bool VerifyAndClearExpectations() {
    base::RunLoop().RunUntilIdle();
    const bool success =
        Mock::VerifyAndClearExpectations(mock_top_sites_.get()) &&
        Mock::VerifyAndClearExpectations(&mock_suggestions_service_) &&
        Mock::VerifyAndClearExpectations(&mock_observer_);
    // For convenience, restore the expectations for IsBlacklisted().
    if (IsPopularSitesFeatureEnabled()) {
      EXPECT_CALL(*mock_top_sites_, IsBlacklisted(_))
          .WillRepeatedly(Return(false));
    }
    return success;
  }

  FakeHomePageClient* RegisterNewHomePageClient() {
    auto home_page_client = std::make_unique<FakeHomePageClient>();
    FakeHomePageClient* raw_client_ptr = home_page_client.get();
    most_visited_sites_->SetHomePageClient(std::move(home_page_client));
    return raw_client_ptr;
  }

  void DisableRemoteSuggestions() {
    EXPECT_CALL(mock_suggestions_service_, AddCallback(_))
        .Times(AnyNumber())
        .WillRepeatedly(Invoke(&suggestions_service_callbacks_,
                               &SuggestionsService::ResponseCallbackList::Add));
    EXPECT_CALL(mock_suggestions_service_, GetSuggestionsDataFromCache())
        .Times(AnyNumber())
        .WillRepeatedly(Return(SuggestionsProfile()));  // Empty cache.
    EXPECT_CALL(mock_suggestions_service_, FetchSuggestionsData())
        .Times(AnyNumber())
        .WillRepeatedly(Return(true));
  }

  base::CallbackList<SuggestionsService::ResponseCallback::RunType>
      suggestions_service_callbacks_;
  TopSitesCallbackList top_sites_callbacks_;

  base::MessageLoop message_loop_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  PopularSitesFactoryForTest popular_sites_factory_;
  scoped_refptr<StrictMock<MockTopSites>> mock_top_sites_;
  StrictMock<MockSuggestionsService> mock_suggestions_service_;
  StrictMock<MockMostVisitedSitesObserver> mock_observer_;
  std::unique_ptr<MostVisitedSites> most_visited_sites_;
  base::test::ScopedFeatureList feature_list_;
  MockIconCacher* icon_cacher_;
};

TEST_P(MostVisitedSitesTest, ShouldStartNoCallInConstructor) {
  // No call to mocks expected by the mere fact of instantiating
  // MostVisitedSites.
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldRefreshBothBackends) {
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(mock_suggestions_service_, FetchSuggestionsData());
  most_visited_sites_->Refresh();
}

TEST_P(MostVisitedSitesTest, ShouldIncludeTileForHomePage) {
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(MostVisitedURLList{}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_observer_, OnURLsAvailable(FirstPersonalizedTileIs(
                                  "", kHomePageUrl, TileSource::HOMEPAGE)));
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldNotIncludeHomePageWithoutClient) {
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(MostVisitedURLList{}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(mock_observer_,
              OnURLsAvailable(Contains(
                  Pair(SectionType::PERSONALIZED,
                       Not(Contains(MatchesTile("", kHomePageUrl,
                                                TileSource::HOMEPAGE)))))));
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldIncludeHomeTileWithUrlBeforeQueryingName) {
  // Because the query time for the real name might take a while, provide the
  // home tile with URL as title immediately and update the tiles as soon as the
  // real title was found.
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);
  home_page_client->SetHomePageTitle(base::UTF8ToUTF16(kHomePageTitle));
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(MostVisitedURLList{}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));
  {
    testing::Sequence seq;
    EXPECT_CALL(mock_observer_,
                OnURLsAvailable(Contains(
                    Pair(SectionType::PERSONALIZED,
                         Not(Contains(MatchesTile("", kHomePageUrl,
                                                  TileSource::HOMEPAGE)))))));
    EXPECT_CALL(mock_observer_,
                OnURLsAvailable(Contains(
                    Pair(SectionType::PERSONALIZED,
                         Not(Contains(MatchesTile(kHomePageTitle, kHomePageUrl,
                                                  TileSource::HOMEPAGE)))))));
  }
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldUpdateHomePageTileOnHomePageStateChanged) {
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);
  DisableRemoteSuggestions();

  // Ensure that home tile is available as usual.
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(MostVisitedURLList{}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_observer_, OnURLsAvailable(FirstPersonalizedTileIs(
                                  "", kHomePageUrl, TileSource::HOMEPAGE)));
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  base::RunLoop().RunUntilIdle();
  VerifyAndClearExpectations();

  // Disable home page and rebuild _without_ Resync. The tile should be gone.
  home_page_client->SetHomePageEnabled(false);
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(MostVisitedURLList{}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory()).Times(0);
  EXPECT_CALL(mock_observer_, OnURLsAvailable(Not(FirstPersonalizedTileIs(
                                  "", kHomePageUrl, TileSource::HOMEPAGE))));
  most_visited_sites_->OnHomePageStateChanged();
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldNotIncludeHomePageIfNoTileRequested) {
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(MostVisitedURLList{}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(SectionType::PERSONALIZED, IsEmpty()))));
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/0);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldReturnHomePageIfOneTileRequested) {
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(
          (MostVisitedURLList{MakeMostVisitedURL("Site 1", "http://site1/")})));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(
          SectionType::PERSONALIZED,
          ElementsAre(MatchesTile("", kHomePageUrl, TileSource::HOMEPAGE))))));
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/1);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldReplaceLastTileWithHomePageWhenFull) {
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>((MostVisitedURLList{
          MakeMostVisitedURL("Site 1", "http://site1/"),
          MakeMostVisitedURL("Site 2", "http://site2/"),
          MakeMostVisitedURL("Site 3", "http://site3/"),
          MakeMostVisitedURL("Site 4", "http://site4/"),
          MakeMostVisitedURL("Site 5", "http://site5/"),
      })));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));
  std::map<SectionType, NTPTilesVector> sections;
  EXPECT_CALL(mock_observer_, OnURLsAvailable(_))
      .WillOnce(SaveArg<0>(&sections));
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/4);
  base::RunLoop().RunUntilIdle();
  ASSERT_THAT(sections, Contains(Key(SectionType::PERSONALIZED)));
  NTPTilesVector tiles = sections.at(SectionType::PERSONALIZED);
  ASSERT_THAT(tiles.size(), Ge(4ul));
  // Assert that the home page is appended as the final tile.
  EXPECT_THAT(tiles[3], MatchesTile("", kHomePageUrl, TileSource::HOMEPAGE));
}

TEST_P(MostVisitedSitesTest, ShouldAppendHomePageWhenNotFull) {
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>((MostVisitedURLList{
          MakeMostVisitedURL("Site 1", "http://site1/"),
          MakeMostVisitedURL("Site 2", "http://site2/"),
          MakeMostVisitedURL("Site 3", "http://site3/"),
          MakeMostVisitedURL("Site 4", "http://site4/"),
          MakeMostVisitedURL("Site 5", "http://site5/"),
      })));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));
  std::map<SectionType, NTPTilesVector> sections;
  EXPECT_CALL(mock_observer_, OnURLsAvailable(_))
      .WillOnce(SaveArg<0>(&sections));
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/8);
  base::RunLoop().RunUntilIdle();
  ASSERT_THAT(sections, Contains(Key(SectionType::PERSONALIZED)));
  NTPTilesVector tiles = sections.at(SectionType::PERSONALIZED);
  ASSERT_THAT(tiles.size(), Ge(6ul));
  // Assert that the home page is appended as the final tile.
  EXPECT_THAT(tiles[5], MatchesTile("", kHomePageUrl, TileSource::HOMEPAGE));
}

TEST_P(MostVisitedSitesTest, ShouldDeduplicateHomePageWithTopSites) {
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(
          (MostVisitedURLList{MakeMostVisitedURL("Site 1", "http://site1/"),
                              MakeMostVisitedURL("", kHomePageUrl)})));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(
          SectionType::PERSONALIZED,
          AllOf(Contains(MatchesTile("", kHomePageUrl, TileSource::HOMEPAGE)),
                Not(Contains(
                    MatchesTile("", kHomePageUrl, TileSource::TOP_SITES))))))));
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldNotIncludeHomePageIfItIsNewTabPage) {
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);
  home_page_client->SetNtpIsHomePage(true);
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(MostVisitedURLList{}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_observer_,
              OnURLsAvailable(Contains(
                  Pair(SectionType::PERSONALIZED,
                       Not(Contains(MatchesTile("", kHomePageUrl,
                                                TileSource::HOMEPAGE)))))));
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldNotIncludeHomePageIfThereIsNone) {
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(false);
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(MostVisitedURLList{}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_observer_,
              OnURLsAvailable(Contains(
                  Pair(SectionType::PERSONALIZED,
                       Not(Contains(MatchesTile("", kHomePageUrl,
                                                TileSource::HOMEPAGE)))))));
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldNotIncludeHomePageIfEmptyUrl) {
  const std::string kEmptyHomePageUrl;
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);
  home_page_client->SetHomePageUrl(GURL(kEmptyHomePageUrl));
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(MostVisitedURLList{}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(kEmptyHomePageUrl)))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_observer_,
              OnURLsAvailable(Not(FirstPersonalizedTileIs(
                  "", kEmptyHomePageUrl, TileSource::HOMEPAGE))));
  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldNotIncludeHomePageIfBlacklisted) {
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(
          (MostVisitedURLList{MakeMostVisitedURL("", kHomePageUrl)})));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber())
      .WillRepeatedly(Return(false));

  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(mock_observer_,
              OnURLsAvailable(Contains(
                  Pair(SectionType::PERSONALIZED,
                       Not(Contains(MatchesTile("", kHomePageUrl,
                                                TileSource::HOMEPAGE)))))));

  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldPinHomePageAgainIfBlacklistingUndone) {
  FakeHomePageClient* home_page_client = RegisterNewHomePageClient();
  home_page_client->SetHomePageEnabled(true);

  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillOnce(InvokeCallbackArgument<0>(
          (MostVisitedURLList{MakeMostVisitedURL("", kHomePageUrl)})));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(mock_observer_,
              OnURLsAvailable(Contains(
                  Pair(SectionType::PERSONALIZED,
                       Not(Contains(MatchesTile("", kHomePageUrl,
                                                TileSource::HOMEPAGE)))))));

  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  base::RunLoop().RunUntilIdle();
  VerifyAndClearExpectations();

  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillOnce(InvokeCallbackArgument<0>(MostVisitedURLList{}));
  EXPECT_CALL(*mock_top_sites_, IsBlacklisted(Eq(GURL(kHomePageUrl))))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(
          SectionType::PERSONALIZED,
          Contains(MatchesTile("", kHomePageUrl, TileSource::HOMEPAGE))))));

  most_visited_sites_->OnBlockedSitesChanged();
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesTest, ShouldInformSuggestionSourcesWhenBlacklisting) {
  EXPECT_CALL(*mock_top_sites_, AddBlacklistedURL(Eq(GURL(kHomePageUrl))))
      .Times(1);
  EXPECT_CALL(mock_suggestions_service_, BlacklistURL(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber());
  most_visited_sites_->AddOrRemoveBlacklistedUrl(GURL(kHomePageUrl),
                                                 /*add_url=*/true);
  EXPECT_CALL(*mock_top_sites_, RemoveBlacklistedURL(Eq(GURL(kHomePageUrl))))
      .Times(1);
  EXPECT_CALL(mock_suggestions_service_,
              UndoBlacklistURL(Eq(GURL(kHomePageUrl))))
      .Times(AnyNumber());
  most_visited_sites_->AddOrRemoveBlacklistedUrl(GURL(kHomePageUrl),
                                                 /*add_url=*/false);
}

TEST_P(MostVisitedSitesTest, ShouldContainSiteExplorationsWhenFeatureEnabled) {
  base::test::ScopedFeatureList feature_list;
  std::map<SectionType, NTPTilesVector> sections;
  feature_list.InitAndEnableFeature(kSiteExplorationUiFeature);
  pref_service_.SetString(prefs::kPopularSitesOverrideVersion, "6");
  RecreateMostVisitedSites();  // Refills cache with version 6 popular sites.
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(
          MostVisitedURLList{MakeMostVisitedURL("Site 1", "http://site1/")}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(mock_observer_, OnURLsAvailable(_))
      .WillOnce(SaveArg<0>(&sections));

  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  base::RunLoop().RunUntilIdle();

  if (!IsPopularSitesFeatureEnabled()) {
    EXPECT_THAT(
        sections,
        Contains(Pair(SectionType::PERSONALIZED,
                      ElementsAre(MatchesTile("Site 1", "http://site1/",
                                              TileSource::TOP_SITES)))));
    return;
  }
  const auto& expected_sections =
      most_visited_sites_->popular_sites()->sections();
  ASSERT_THAT(expected_sections.size(), Ge(2ul));
  EXPECT_THAT(sections.size(), Eq(expected_sections.size()));
  EXPECT_THAT(
      sections,
      AllOf(Contains(Pair(
                SectionType::PERSONALIZED,
                ElementsAre(MatchesTile("Site 1", "http://site1/",
                                        TileSource::TOP_SITES),
                            MatchesTile("PopularSite1", "http://popularsite1/",
                                        TileSource::POPULAR),
                            MatchesTile("PopularSite2", "http://popularsite2/",
                                        TileSource::POPULAR)))),
            Contains(Pair(SectionType::NEWS, SizeIs(2ul))),
            Contains(Pair(SectionType::SOCIAL, SizeIs(1ul))),
            Contains(Pair(_, IsEmpty()))));
}

TEST_P(MostVisitedSitesTest,
       ShouldDeduplicatePopularSitesWithMostVisitedIffHostAndTitleMatches) {
  pref_service_.SetString(prefs::kPopularSitesOverrideCountry, "US");
  RecreateMostVisitedSites();  // Refills cache with ESPN and Google News.
  DisableRemoteSuggestions();
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(MostVisitedURLList{
          MakeMostVisitedURL("ESPN", "http://espn.com/"),
          MakeMostVisitedURL("Mobile", "http://m.mobile.de/"),
          MakeMostVisitedURL("Google", "http://www.google.com/")}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  std::map<SectionType, NTPTilesVector> sections;
  EXPECT_CALL(mock_observer_, OnURLsAvailable(_))
      .WillOnce(SaveArg<0>(&sections));

  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/6);
  base::RunLoop().RunUntilIdle();
  ASSERT_THAT(sections, Contains(Key(SectionType::PERSONALIZED)));
  EXPECT_THAT(sections.at(SectionType::PERSONALIZED),
              Contains(MatchesTile("Google", "http://www.google.com/",
                                   TileSource::TOP_SITES)));
  if (IsPopularSitesFeatureEnabled()) {
    EXPECT_THAT(sections.at(SectionType::PERSONALIZED),
                Contains(MatchesTile("Google News", "http://news.google.com/",
                                     TileSource::POPULAR)));
  }
  EXPECT_THAT(sections.at(SectionType::PERSONALIZED),
              AllOf(Contains(MatchesTile("ESPN", "http://espn.com/",
                                         TileSource::TOP_SITES)),
                    Contains(MatchesTile("Mobile", "http://m.mobile.de/",
                                         TileSource::TOP_SITES)),
                    Not(Contains(MatchesTile("ESPN", "http://www.espn.com/",
                                             TileSource::POPULAR))),
                    Not(Contains(MatchesTile("Mobile", "http://www.mobile.de/",
                                             TileSource::POPULAR)))));
}

TEST_P(MostVisitedSitesTest, ShouldHandleTopSitesCacheHit) {
  // If cached, TopSites returns the tiles synchronously, running the callback
  // even before the function returns.
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillRepeatedly(InvokeCallbackArgument<0>(
          MostVisitedURLList{MakeMostVisitedURL("Site 1", "http://site1/")}));

  InSequence seq;
  EXPECT_CALL(mock_suggestions_service_, AddCallback(_))
      .WillOnce(Invoke(&suggestions_service_callbacks_,
                       &SuggestionsService::ResponseCallbackList::Add));
  EXPECT_CALL(mock_suggestions_service_, GetSuggestionsDataFromCache())
      .WillOnce(Return(SuggestionsProfile()));  // Empty cache.
  if (IsPopularSitesFeatureEnabled()) {
    EXPECT_CALL(
        mock_observer_,
        OnURLsAvailable(Contains(Pair(
            SectionType::PERSONALIZED,
            ElementsAre(
                MatchesTile("Site 1", "http://site1/", TileSource::TOP_SITES),
                MatchesTile("PopularSite1", "http://popularsite1/",
                            TileSource::POPULAR),
                MatchesTile("PopularSite2", "http://popularsite2/",
                            TileSource::POPULAR))))));
  } else {
    EXPECT_CALL(mock_observer_,
                OnURLsAvailable(Contains(
                    Pair(SectionType::PERSONALIZED,
                         ElementsAre(MatchesTile("Site 1", "http://site1/",
                                                 TileSource::TOP_SITES))))));
  }
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  EXPECT_CALL(mock_suggestions_service_, FetchSuggestionsData())
      .WillOnce(Return(true));

  most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                  /*num_sites=*/3);
  VerifyAndClearExpectations();
  EXPECT_FALSE(suggestions_service_callbacks_.empty());
  CHECK(top_sites_callbacks_.empty());

  // Update by TopSites is propagated.
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillOnce(InvokeCallbackArgument<0>(
          MostVisitedURLList{MakeMostVisitedURL("Site 2", "http://site2/")}));
  if (IsPopularSitesFeatureEnabled()) {
    EXPECT_CALL(*mock_top_sites_, IsBlacklisted(_))
        .WillRepeatedly(Return(false));
  }
  EXPECT_CALL(mock_observer_, OnURLsAvailable(_));
  mock_top_sites_->NotifyTopSitesChanged(
      history::TopSitesObserver::ChangeReason::MOST_VISITED);
  base::RunLoop().RunUntilIdle();
}

INSTANTIATE_TEST_CASE_P(MostVisitedSitesTest,
                        MostVisitedSitesTest,
                        ::testing::Bool());

TEST(MostVisitedSitesTest, ShouldDeduplicateDomainWithNoWwwDomain) {
  EXPECT_TRUE(MostVisitedSites::IsHostOrMobilePageKnown({"www.mobile.de"},
                                                        "mobile.de"));
  EXPECT_TRUE(MostVisitedSites::IsHostOrMobilePageKnown({"mobile.de"},
                                                        "www.mobile.de"));
  EXPECT_TRUE(MostVisitedSites::IsHostOrMobilePageKnown({"mobile.co.uk"},
                                                        "www.mobile.co.uk"));
}

TEST(MostVisitedSitesTest, ShouldDeduplicateDomainByRemovingMobilePrefixes) {
  EXPECT_TRUE(
      MostVisitedSites::IsHostOrMobilePageKnown({"bbc.co.uk"}, "m.bbc.co.uk"));
  EXPECT_TRUE(
      MostVisitedSites::IsHostOrMobilePageKnown({"m.bbc.co.uk"}, "bbc.co.uk"));
  EXPECT_TRUE(MostVisitedSites::IsHostOrMobilePageKnown({"cnn.com"},
                                                        "edition.cnn.com"));
  EXPECT_TRUE(MostVisitedSites::IsHostOrMobilePageKnown({"edition.cnn.com"},
                                                        "cnn.com"));
  EXPECT_TRUE(
      MostVisitedSites::IsHostOrMobilePageKnown({"cnn.com"}, "mobile.cnn.com"));
  EXPECT_TRUE(
      MostVisitedSites::IsHostOrMobilePageKnown({"mobile.cnn.com"}, "cnn.com"));
}

TEST(MostVisitedSitesTest, ShouldDeduplicateDomainByReplacingMobilePrefixes) {
  EXPECT_TRUE(MostVisitedSites::IsHostOrMobilePageKnown({"www.bbc.co.uk"},
                                                        "m.bbc.co.uk"));
  EXPECT_TRUE(MostVisitedSites::IsHostOrMobilePageKnown({"m.mobile.de"},
                                                        "www.mobile.de"));
  EXPECT_TRUE(MostVisitedSites::IsHostOrMobilePageKnown({"www.cnn.com"},
                                                        "edition.cnn.com"));
  EXPECT_TRUE(MostVisitedSites::IsHostOrMobilePageKnown({"mobile.cnn.com"},
                                                        "www.cnn.com"));
}

class MostVisitedSitesWithCacheHitTest : public MostVisitedSitesTest {
 public:
  // Constructor sets the common expectations for the case where suggestions
  // service has cached results when the observer is registered.
  MostVisitedSitesWithCacheHitTest() {
    InSequence seq;
    EXPECT_CALL(mock_suggestions_service_, AddCallback(_))
        .WillOnce(Invoke(&suggestions_service_callbacks_,
                         &SuggestionsService::ResponseCallbackList::Add));
    EXPECT_CALL(mock_suggestions_service_, GetSuggestionsDataFromCache())
        .WillOnce(Return(MakeProfile({
            MakeSuggestion("Site 1", "http://site1/"),
            MakeSuggestion("Site 2", "http://site2/"),
            MakeSuggestion("Site 3", "http://site3/"),
        })));

    if (IsPopularSitesFeatureEnabled()) {
      EXPECT_CALL(
          mock_observer_,
          OnURLsAvailable(Contains(Pair(
              SectionType::PERSONALIZED,
              ElementsAre(MatchesTile("Site 1", "http://site1/",
                                      TileSource::SUGGESTIONS_SERVICE),
                          MatchesTile("Site 2", "http://site2/",
                                      TileSource::SUGGESTIONS_SERVICE),
                          MatchesTile("Site 3", "http://site3/",
                                      TileSource::SUGGESTIONS_SERVICE),
                          MatchesTile("PopularSite1", "http://popularsite1/",
                                      TileSource::POPULAR))))));
    } else {
      EXPECT_CALL(
          mock_observer_,
          OnURLsAvailable(Contains(Pair(
              SectionType::PERSONALIZED,
              ElementsAre(MatchesTile("Site 1", "http://site1/",
                                      TileSource::SUGGESTIONS_SERVICE),
                          MatchesTile("Site 2", "http://site2/",
                                      TileSource::SUGGESTIONS_SERVICE),
                          MatchesTile("Site 3", "http://site3/",
                                      TileSource::SUGGESTIONS_SERVICE))))));
    }
    EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
    EXPECT_CALL(mock_suggestions_service_, FetchSuggestionsData())
        .WillOnce(Return(true));

    most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                    /*num_sites=*/4);
    VerifyAndClearExpectations();

    EXPECT_FALSE(suggestions_service_callbacks_.empty());
    EXPECT_TRUE(top_sites_callbacks_.empty());
  }
};

TEST_P(MostVisitedSitesWithCacheHitTest, ShouldFavorSuggestionsServiceCache) {
  // Constructor sets basic expectations for a suggestions service cache hit.
}

TEST_P(MostVisitedSitesWithCacheHitTest,
       ShouldPropagateUpdateBySuggestionsService) {
  EXPECT_CALL(mock_observer_,
              OnURLsAvailable(Contains(Pair(
                  SectionType::PERSONALIZED,
                  ElementsAre(MatchesTile("Site 4", "http://site4/",
                                          TileSource::SUGGESTIONS_SERVICE),
                              MatchesTile("Site 5", "http://site5/",
                                          TileSource::SUGGESTIONS_SERVICE),
                              MatchesTile("Site 6", "http://site6/",
                                          TileSource::SUGGESTIONS_SERVICE),
                              MatchesTile("Site 7", "http://site7/",
                                          TileSource::SUGGESTIONS_SERVICE))))));
  suggestions_service_callbacks_.Notify(
      MakeProfile({MakeSuggestion("Site 4", "http://site4/"),
                   MakeSuggestion("Site 5", "http://site5/"),
                   MakeSuggestion("Site 6", "http://site6/"),
                   MakeSuggestion("Site 7", "http://site7/")}));
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesWithCacheHitTest, ShouldTruncateList) {
  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(SectionType::PERSONALIZED, SizeIs(4)))));
  suggestions_service_callbacks_.Notify(
      MakeProfile({MakeSuggestion("Site 4", "http://site4/"),
                   MakeSuggestion("Site 5", "http://site5/"),
                   MakeSuggestion("Site 6", "http://site6/"),
                   MakeSuggestion("Site 7", "http://site7/"),
                   MakeSuggestion("Site 8", "http://site8/")}));
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesWithCacheHitTest,
       ShouldCompleteWithPopularSitesIffEnabled) {
  if (IsPopularSitesFeatureEnabled()) {
    EXPECT_CALL(
        mock_observer_,
        OnURLsAvailable(Contains(
            Pair(SectionType::PERSONALIZED,
                 ElementsAre(MatchesTile("Site 4", "http://site4/",
                                         TileSource::SUGGESTIONS_SERVICE),
                             MatchesTile("PopularSite1", "http://popularsite1/",
                                         TileSource::POPULAR),
                             MatchesTile("PopularSite2", "http://popularsite2/",
                                         TileSource::POPULAR))))));
  } else {
    EXPECT_CALL(
        mock_observer_,
        OnURLsAvailable(Contains(
            Pair(SectionType::PERSONALIZED,
                 ElementsAre(MatchesTile("Site 4", "http://site4/",
                                         TileSource::SUGGESTIONS_SERVICE))))));
  }
  suggestions_service_callbacks_.Notify(
      MakeProfile({MakeSuggestion("Site 4", "http://site4/")}));
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesWithCacheHitTest,
       ShouldSwitchToTopSitesIfEmptyUpdateBySuggestionsService) {
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillOnce(Invoke(&top_sites_callbacks_, &TopSitesCallbackList::Add));
  suggestions_service_callbacks_.Notify(SuggestionsProfile());
  VerifyAndClearExpectations();

  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(
          SectionType::PERSONALIZED,
          ElementsAre(
              MatchesTile("Site 4", "http://site4/", TileSource::TOP_SITES),
              MatchesTile("Site 5", "http://site5/", TileSource::TOP_SITES),
              MatchesTile("Site 6", "http://site6/", TileSource::TOP_SITES),
              MatchesTile("Site 7", "http://site7/",
                          TileSource::TOP_SITES))))));
  top_sites_callbacks_.ClearAndNotify(
      {MakeMostVisitedURL("Site 4", "http://site4/"),
       MakeMostVisitedURL("Site 5", "http://site5/"),
       MakeMostVisitedURL("Site 6", "http://site6/"),
       MakeMostVisitedURL("Site 7", "http://site7/")});
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesWithCacheHitTest, ShouldFetchFaviconsIfEnabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(kNtpMostLikelyFaviconsFromServerFeature);

  EXPECT_CALL(mock_observer_, OnURLsAvailable(_));
  EXPECT_CALL(*icon_cacher_, StartFetchMostLikely(GURL("http://site4/"), _));

  suggestions_service_callbacks_.Notify(
      MakeProfile({MakeSuggestion("Site 4", "http://site4/")}));
  base::RunLoop().RunUntilIdle();
}

INSTANTIATE_TEST_CASE_P(MostVisitedSitesWithCacheHitTest,
                        MostVisitedSitesWithCacheHitTest,
                        ::testing::Bool());

class MostVisitedSitesWithEmptyCacheTest : public MostVisitedSitesTest {
 public:
  // Constructor sets the common expectations for the case where suggestions
  // service doesn't have cached results when the observer is registered.
  MostVisitedSitesWithEmptyCacheTest() {
    InSequence seq;
    EXPECT_CALL(mock_suggestions_service_, AddCallback(_))
        .WillOnce(Invoke(&suggestions_service_callbacks_,
                         &SuggestionsService::ResponseCallbackList::Add));
    EXPECT_CALL(mock_suggestions_service_, GetSuggestionsDataFromCache())
        .WillOnce(Return(SuggestionsProfile()));  // Empty cache.
    EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
        .WillOnce(Invoke(&top_sites_callbacks_, &TopSitesCallbackList::Add));
    EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
    EXPECT_CALL(mock_suggestions_service_, FetchSuggestionsData())
        .WillOnce(Return(true));

    most_visited_sites_->SetMostVisitedURLsObserver(&mock_observer_,
                                                    /*num_sites=*/3);
    VerifyAndClearExpectations();

    EXPECT_FALSE(suggestions_service_callbacks_.empty());
    EXPECT_FALSE(top_sites_callbacks_.empty());
  }
};

TEST_P(MostVisitedSitesWithEmptyCacheTest,
       ShouldQueryTopSitesAndSuggestionsService) {
  // Constructor sets basic expectations for a suggestions service cache miss.
}

TEST_P(MostVisitedSitesWithEmptyCacheTest,
       ShouldCompleteWithPopularSitesIffEnabled) {
  if (IsPopularSitesFeatureEnabled()) {
    EXPECT_CALL(
        mock_observer_,
        OnURLsAvailable(Contains(
            Pair(SectionType::PERSONALIZED,
                 ElementsAre(MatchesTile("Site 4", "http://site4/",
                                         TileSource::SUGGESTIONS_SERVICE),
                             MatchesTile("PopularSite1", "http://popularsite1/",
                                         TileSource::POPULAR),
                             MatchesTile("PopularSite2", "http://popularsite2/",
                                         TileSource::POPULAR))))));
  } else {
    EXPECT_CALL(
        mock_observer_,
        OnURLsAvailable(Contains(
            Pair(SectionType::PERSONALIZED,
                 ElementsAre(MatchesTile("Site 4", "http://site4/",
                                         TileSource::SUGGESTIONS_SERVICE))))));
  }
  suggestions_service_callbacks_.Notify(
      MakeProfile({MakeSuggestion("Site 4", "http://site4/")}));
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesWithEmptyCacheTest,
       ShouldIgnoreTopSitesIfSuggestionsServiceFaster) {
  // Reply from suggestions service triggers and update to our observer.
  EXPECT_CALL(mock_observer_,
              OnURLsAvailable(Contains(Pair(
                  SectionType::PERSONALIZED,
                  ElementsAre(MatchesTile("Site 1", "http://site1/",
                                          TileSource::SUGGESTIONS_SERVICE),
                              MatchesTile("Site 2", "http://site2/",
                                          TileSource::SUGGESTIONS_SERVICE),
                              MatchesTile("Site 3", "http://site3/",
                                          TileSource::SUGGESTIONS_SERVICE))))));
  suggestions_service_callbacks_.Notify(
      MakeProfile({MakeSuggestion("Site 1", "http://site1/"),
                   MakeSuggestion("Site 2", "http://site2/"),
                   MakeSuggestion("Site 3", "http://site3/")}));
  VerifyAndClearExpectations();

  // Reply from top sites is ignored (i.e. not reported to observer).
  top_sites_callbacks_.ClearAndNotify(
      {MakeMostVisitedURL("Site 4", "http://site4/")});
  VerifyAndClearExpectations();

  // Update by TopSites is also ignored.
  mock_top_sites_->NotifyTopSitesChanged(
      history::TopSitesObserver::ChangeReason::MOST_VISITED);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesWithEmptyCacheTest,
       ShouldExposeTopSitesIfSuggestionsServiceFasterButEmpty) {
  // Empty reply from suggestions service causes no update to our observer.
  suggestions_service_callbacks_.Notify(SuggestionsProfile());
  VerifyAndClearExpectations();

  // Reply from top sites is propagated to observer.
  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(
          SectionType::PERSONALIZED,
          ElementsAre(
              MatchesTile("Site 1", "http://site1/", TileSource::TOP_SITES),
              MatchesTile("Site 2", "http://site2/", TileSource::TOP_SITES),
              MatchesTile("Site 3", "http://site3/",
                          TileSource::TOP_SITES))))));
  top_sites_callbacks_.ClearAndNotify(
      {MakeMostVisitedURL("Site 1", "http://site1/"),
       MakeMostVisitedURL("Site 2", "http://site2/"),
       MakeMostVisitedURL("Site 3", "http://site3/")});
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesWithEmptyCacheTest,
       ShouldFavorSuggestionsServiceAlthoughSlower) {
  // Reply from top sites is propagated to observer.
  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(
          SectionType::PERSONALIZED,
          ElementsAre(
              MatchesTile("Site 1", "http://site1/", TileSource::TOP_SITES),
              MatchesTile("Site 2", "http://site2/", TileSource::TOP_SITES),
              MatchesTile("Site 3", "http://site3/",
                          TileSource::TOP_SITES))))));
  top_sites_callbacks_.ClearAndNotify(
      {MakeMostVisitedURL("Site 1", "http://site1/"),
       MakeMostVisitedURL("Site 2", "http://site2/"),
       MakeMostVisitedURL("Site 3", "http://site3/")});
  VerifyAndClearExpectations();

  // Reply from suggestions service overrides top sites.
  InSequence seq;
  EXPECT_CALL(mock_observer_,
              OnURLsAvailable(Contains(Pair(
                  SectionType::PERSONALIZED,
                  ElementsAre(MatchesTile("Site 4", "http://site4/",
                                          TileSource::SUGGESTIONS_SERVICE),
                              MatchesTile("Site 5", "http://site5/",
                                          TileSource::SUGGESTIONS_SERVICE),
                              MatchesTile("Site 6", "http://site6/",
                                          TileSource::SUGGESTIONS_SERVICE))))));
  suggestions_service_callbacks_.Notify(
      MakeProfile({MakeSuggestion("Site 4", "http://site4/"),
                   MakeSuggestion("Site 5", "http://site5/"),
                   MakeSuggestion("Site 6", "http://site6/")}));
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesWithEmptyCacheTest,
       ShouldIgnoreSuggestionsServiceIfSlowerAndEmpty) {
  // Reply from top sites is propagated to observer.
  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(
          SectionType::PERSONALIZED,
          ElementsAre(
              MatchesTile("Site 1", "http://site1/", TileSource::TOP_SITES),
              MatchesTile("Site 2", "http://site2/", TileSource::TOP_SITES),
              MatchesTile("Site 3", "http://site3/",
                          TileSource::TOP_SITES))))));
  top_sites_callbacks_.ClearAndNotify(
      {MakeMostVisitedURL("Site 1", "http://site1/"),
       MakeMostVisitedURL("Site 2", "http://site2/"),
       MakeMostVisitedURL("Site 3", "http://site3/")});
  VerifyAndClearExpectations();

  // Reply from suggestions service is empty and thus ignored.
  suggestions_service_callbacks_.Notify(SuggestionsProfile());
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesWithEmptyCacheTest, ShouldPropagateUpdateByTopSites) {
  // Reply from top sites is propagated to observer.
  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(
          SectionType::PERSONALIZED,
          ElementsAre(
              MatchesTile("Site 1", "http://site1/", TileSource::TOP_SITES),
              MatchesTile("Site 2", "http://site2/", TileSource::TOP_SITES),
              MatchesTile("Site 3", "http://site3/",
                          TileSource::TOP_SITES))))));
  top_sites_callbacks_.ClearAndNotify(
      {MakeMostVisitedURL("Site 1", "http://site1/"),
       MakeMostVisitedURL("Site 2", "http://site2/"),
       MakeMostVisitedURL("Site 3", "http://site3/")});
  VerifyAndClearExpectations();

  // Reply from suggestions service is empty and thus ignored.
  suggestions_service_callbacks_.Notify(SuggestionsProfile());
  VerifyAndClearExpectations();
  EXPECT_TRUE(top_sites_callbacks_.empty());

  // Update from top sites is propagated to observer.
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
      .WillOnce(InvokeCallbackArgument<0>(
          MostVisitedURLList{MakeMostVisitedURL("Site 4", "http://site4/"),
                             MakeMostVisitedURL("Site 5", "http://site5/"),
                             MakeMostVisitedURL("Site 6", "http://site6/")}));
  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(
          SectionType::PERSONALIZED,
          ElementsAre(
              MatchesTile("Site 4", "http://site4/", TileSource::TOP_SITES),
              MatchesTile("Site 5", "http://site5/", TileSource::TOP_SITES),
              MatchesTile("Site 6", "http://site6/",
                          TileSource::TOP_SITES))))));
  mock_top_sites_->NotifyTopSitesChanged(
      history::TopSitesObserver::ChangeReason::MOST_VISITED);
  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesWithEmptyCacheTest,
       ShouldSendEmptyListIfBothTopSitesAndSuggestionsServiceEmpty) {
  if (IsPopularSitesFeatureEnabled()) {
    EXPECT_CALL(
        mock_observer_,
        OnURLsAvailable(Contains(
            Pair(SectionType::PERSONALIZED,
                 ElementsAre(MatchesTile("PopularSite1", "http://popularsite1/",
                                         TileSource::POPULAR),
                             MatchesTile("PopularSite2", "http://popularsite2/",
                                         TileSource::POPULAR))))));
  } else {
    // The Android NTP doesn't finish initialization until it gets tiles, so a
    // 0-tile notification is always needed.
    EXPECT_CALL(mock_observer_, OnURLsAvailable(ElementsAre(Pair(
                                    SectionType::PERSONALIZED, IsEmpty()))));
  }
  suggestions_service_callbacks_.Notify(SuggestionsProfile());
  top_sites_callbacks_.ClearAndNotify(MostVisitedURLList{});

  base::RunLoop().RunUntilIdle();
}

TEST_P(MostVisitedSitesWithEmptyCacheTest,
       ShouldNotifyOnceIfTopSitesUnchanged) {
  EXPECT_CALL(
      mock_observer_,
      OnURLsAvailable(Contains(Pair(
          SectionType::PERSONALIZED,
          ElementsAre(
              MatchesTile("Site 1", "http://site1/", TileSource::TOP_SITES),
              MatchesTile("Site 2", "http://site2/", TileSource::TOP_SITES),
              MatchesTile("Site 3", "http://site3/",
                          TileSource::TOP_SITES))))));

  suggestions_service_callbacks_.Notify(SuggestionsProfile());

  top_sites_callbacks_.ClearAndNotify(
      {MakeMostVisitedURL("Site 1", "http://site1/"),
       MakeMostVisitedURL("Site 2", "http://site2/"),
       MakeMostVisitedURL("Site 3", "http://site3/")});
  base::RunLoop().RunUntilIdle();

  for (int i = 0; i < 4; ++i) {
    EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_, false))
        .WillOnce(Invoke(&top_sites_callbacks_, &TopSitesCallbackList::Add));
    mock_top_sites_->NotifyTopSitesChanged(
        history::TopSitesObserver::ChangeReason::MOST_VISITED);
    EXPECT_FALSE(top_sites_callbacks_.empty());
    top_sites_callbacks_.ClearAndNotify(
        {MakeMostVisitedURL("Site 1", "http://site1/"),
         MakeMostVisitedURL("Site 2", "http://site2/"),
         MakeMostVisitedURL("Site 3", "http://site3/")});
    base::RunLoop().RunUntilIdle();
  }
}

TEST_P(MostVisitedSitesWithEmptyCacheTest,
       ShouldNotifyOnceIfSuggestionsUnchanged) {
  EXPECT_CALL(mock_observer_,
              OnURLsAvailable(Contains(Pair(
                  SectionType::PERSONALIZED,
                  ElementsAre(MatchesTile("Site 1", "http://site1/",
                                          TileSource::SUGGESTIONS_SERVICE),
                              MatchesTile("Site 2", "http://site2/",
                                          TileSource::SUGGESTIONS_SERVICE),
                              MatchesTile("Site 3", "http://site3/",
                                          TileSource::SUGGESTIONS_SERVICE))))));

  for (int i = 0; i < 5; ++i) {
    suggestions_service_callbacks_.Notify(
        MakeProfile({MakeSuggestion("Site 1", "http://site1/"),
                     MakeSuggestion("Site 2", "http://site2/"),
                     MakeSuggestion("Site 3", "http://site3/")}));
  }
}

INSTANTIATE_TEST_CASE_P(MostVisitedSitesWithEmptyCacheTest,
                        MostVisitedSitesWithEmptyCacheTest,
                        ::testing::Bool());

// This a test for MostVisitedSites::MergeTiles(...) method, and thus has the
// same scope as the method itself. This tests merging popular sites with
// personal tiles.
// More important things out of the scope of testing presently:
// - Removing blacklisted tiles.
// - Correct host extraction from the URL.
// - Ensuring personal tiles are not duplicated in popular tiles.
TEST(MostVisitedSitesMergeTest, ShouldMergeTilesWithPersonalOnly) {
  std::vector<NTPTile> personal_tiles{
      MakeTile("Site 1", "https://www.site1.com/", TileSource::TOP_SITES),
      MakeTile("Site 2", "https://www.site2.com/", TileSource::TOP_SITES),
      MakeTile("Site 3", "https://www.site3.com/", TileSource::TOP_SITES),
      MakeTile("Site 4", "https://www.site4.com/", TileSource::TOP_SITES),
  };
  // Without any popular tiles, the result after merge should be the personal
  // tiles.
  EXPECT_THAT(MostVisitedSites::MergeTiles(std::move(personal_tiles),
                                           /*whitelist_tiles=*/NTPTilesVector(),
                                           /*popular_tiles=*/NTPTilesVector()),
              ElementsAre(MatchesTile("Site 1", "https://www.site1.com/",
                                      TileSource::TOP_SITES),
                          MatchesTile("Site 2", "https://www.site2.com/",
                                      TileSource::TOP_SITES),
                          MatchesTile("Site 3", "https://www.site3.com/",
                                      TileSource::TOP_SITES),
                          MatchesTile("Site 4", "https://www.site4.com/",
                                      TileSource::TOP_SITES)));
}

TEST(MostVisitedSitesMergeTest, ShouldMergeTilesWithPopularOnly) {
  std::vector<NTPTile> popular_tiles{
      MakeTile("Site 1", "https://www.site1.com/", TileSource::POPULAR),
      MakeTile("Site 2", "https://www.site2.com/", TileSource::POPULAR),
      MakeTile("Site 3", "https://www.site3.com/", TileSource::POPULAR),
      MakeTile("Site 4", "https://www.site4.com/", TileSource::POPULAR),
  };
  // Without any personal tiles, the result after merge should be the popular
  // tiles.
  EXPECT_THAT(
      MostVisitedSites::MergeTiles(/*personal_tiles=*/NTPTilesVector(),
                                   /*whitelist_tiles=*/NTPTilesVector(),
                                   /*popular_tiles=*/std::move(popular_tiles)),
      ElementsAre(
          MatchesTile("Site 1", "https://www.site1.com/", TileSource::POPULAR),
          MatchesTile("Site 2", "https://www.site2.com/", TileSource::POPULAR),
          MatchesTile("Site 3", "https://www.site3.com/", TileSource::POPULAR),
          MatchesTile("Site 4", "https://www.site4.com/",
                      TileSource::POPULAR)));
}

TEST(MostVisitedSitesMergeTest, ShouldMergeTilesFavoringPersonalOverPopular) {
  std::vector<NTPTile> popular_tiles{
      MakeTile("Site 1", "https://www.site1.com/", TileSource::POPULAR),
      MakeTile("Site 2", "https://www.site2.com/", TileSource::POPULAR),
  };
  std::vector<NTPTile> personal_tiles{
      MakeTile("Site 3", "https://www.site3.com/", TileSource::TOP_SITES),
      MakeTile("Site 4", "https://www.site4.com/", TileSource::TOP_SITES),
  };
  EXPECT_THAT(
      MostVisitedSites::MergeTiles(std::move(personal_tiles),
                                   /*whitelist_tiles=*/NTPTilesVector(),
                                   /*popular_tiles=*/std::move(popular_tiles)),
      ElementsAre(
          MatchesTile("Site 3", "https://www.site3.com/",
                      TileSource::TOP_SITES),
          MatchesTile("Site 4", "https://www.site4.com/",
                      TileSource::TOP_SITES),
          MatchesTile("Site 1", "https://www.site1.com/", TileSource::POPULAR),
          MatchesTile("Site 2", "https://www.site2.com/",
                      TileSource::POPULAR)));
}

}  // namespace ntp_tiles
