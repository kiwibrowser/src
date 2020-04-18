// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/zero_suggest_provider.h"

#include <list>
#include <map>
#include <memory>
#include <string>

#include "base/metrics/field_trial.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "components/history/core/browser/top_sites.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/omnibox/browser/mock_autocomplete_provider_client.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/omnibox/browser/omnibox_pref_names.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "components/variations/entropy_provider.h"
#include "components/variations/variations_associated_data.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"

namespace {
class FakeEmptyTopSites : public history::TopSites {
 public:
  FakeEmptyTopSites() {
  }

  // history::TopSites:
  bool SetPageThumbnail(const GURL& url, const gfx::Image& thumbnail,
                        const ThumbnailScore& score) override {
    return false;
  }
  void GetMostVisitedURLs(const GetMostVisitedURLsCallback& callback,
                          bool include_forced_urls) override;
  bool GetPageThumbnail(const GURL& url, bool prefix_match,
                        scoped_refptr<base::RefCountedMemory>* bytes) override {
    return false;
  }
  bool GetPageThumbnailScore(const GURL& url, ThumbnailScore* score) override {
    return false;
  }
  bool GetTemporaryPageThumbnailScore(const GURL& url, ThumbnailScore* score)
      override {
    return false;
  }
  void SyncWithHistory() override {}
  bool HasBlacklistedItems() const override {
    return false;
  }
  void AddBlacklistedURL(const GURL& url) override {}
  void RemoveBlacklistedURL(const GURL& url) override {}
  bool IsBlacklisted(const GURL& url) override {
    return false;
  }
  void ClearBlacklistedURLs() override {}
  bool IsKnownURL(const GURL& url) override {
    return false;
  }
  bool IsNonForcedFull() override {
    return false;
  }
  bool IsForcedFull() override {
    return false;
  }
  bool loaded() const override {
    return false;
  }
  history::PrepopulatedPageList GetPrepopulatedPages() override {
    return history::PrepopulatedPageList();
  }
  bool AddForcedURL(const GURL& url, const base::Time& time) override {
    return false;
  }
  void OnNavigationCommitted(const GURL& url) override {}

  // RefcountedKeyedService:
  void ShutdownOnUIThread() override {}

  // Only runs a single callback, so that the test can specify a different
  // set per call.
  void RunACallback(const history::MostVisitedURLList& urls) {
    DCHECK(!callbacks.empty());
    callbacks.front().Run(urls);
    callbacks.pop_front();
  }

 protected:
  // A test-specific field for controlling when most visited callback is run
  // after top sites have been requested.
  std::list<GetMostVisitedURLsCallback> callbacks;

  ~FakeEmptyTopSites() override {}

  DISALLOW_COPY_AND_ASSIGN(FakeEmptyTopSites);
};

void FakeEmptyTopSites::GetMostVisitedURLs(
    const GetMostVisitedURLsCallback& callback,
    bool include_forced_urls)  {
  callbacks.push_back(callback);
}

class FakeAutocompleteProviderClient : public MockAutocompleteProviderClient {
 public:
  FakeAutocompleteProviderClient()
      : template_url_service_(new TemplateURLService(nullptr, 0)),
        top_sites_(new FakeEmptyTopSites()) {
    pref_service_.registry()->RegisterStringPref(
        omnibox::kZeroSuggestCachedResults, std::string());
  }

  bool SearchSuggestEnabled() const override { return true; }

  scoped_refptr<history::TopSites> GetTopSites() override { return top_sites_; }

  TemplateURLService* GetTemplateURLService() override {
    return template_url_service_.get();
  }

  TemplateURLService* GetTemplateURLService() const override {
    return template_url_service_.get();
  }

  PrefService* GetPrefs() override { return &pref_service_; }

  void Classify(
      const base::string16& text,
      bool prefer_keyword,
      bool allow_exact_keyword_match,
      metrics::OmniboxEventProto::PageClassification page_classification,
      AutocompleteMatch* match,
      GURL* alternate_nav_url) override {
    // Populate enough of |match| to keep the ZeroSuggestProvider happy.
    match->type = AutocompleteMatchType::URL_WHAT_YOU_TYPED;
    match->destination_url = GURL(text);
  }

  const AutocompleteSchemeClassifier& GetSchemeClassifier() const override {
    return scheme_classifier_;
  }

 private:
  std::unique_ptr<TemplateURLService> template_url_service_;
  scoped_refptr<history::TopSites> top_sites_;
  TestingPrefServiceSimple pref_service_;
  TestSchemeClassifier scheme_classifier_;

  DISALLOW_COPY_AND_ASSIGN(FakeAutocompleteProviderClient);
};

}  // namespace

class ZeroSuggestProviderTest : public testing::Test,
                                public AutocompleteProviderListener {
 public:
  ZeroSuggestProviderTest();

  void SetUp() override;

 protected:
  // AutocompleteProviderListener:
  void OnProviderUpdate(bool updated_matches) override;

  void ResetFieldTrialList();

  void CreatePersonalizedFieldTrial();
  void CreateMostVisitedFieldTrial();

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Needed for OmniboxFieldTrial::ActivateStaticTrials().
  std::unique_ptr<base::FieldTrialList> field_trial_list_;

  net::TestURLFetcherFactory test_factory_;
  std::unique_ptr<FakeAutocompleteProviderClient> client_;
  scoped_refptr<ZeroSuggestProvider> provider_;
  TemplateURL* default_t_url_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ZeroSuggestProviderTest);
};

ZeroSuggestProviderTest::ZeroSuggestProviderTest() {
  ResetFieldTrialList();
}

void ZeroSuggestProviderTest::SetUp() {
  // Make sure that fetchers are automatically unregistered upon destruction.
  test_factory_.set_remove_fetcher_on_delete(true);
  client_.reset(new FakeAutocompleteProviderClient());

  TemplateURLService* turl_model = client_->GetTemplateURLService();
  turl_model->Load();

  TemplateURLData data;
  data.SetShortName(base::ASCIIToUTF16("t"));
  data.SetURL("https://www.google.com/?q={searchTerms}");
  data.suggestions_url = "https://www.google.com/complete/?q={searchTerms}";
  default_t_url_ = turl_model->Add(std::make_unique<TemplateURL>(data));
  turl_model->SetUserSelectedDefaultSearchProvider(default_t_url_);

  provider_ = ZeroSuggestProvider::Create(client_.get(), nullptr, this);
}

void ZeroSuggestProviderTest::OnProviderUpdate(bool updated_matches) {
}

void ZeroSuggestProviderTest::ResetFieldTrialList() {
  // Destroy the existing FieldTrialList before creating a new one to avoid
  // a DCHECK.
  field_trial_list_.reset();
  field_trial_list_.reset(new base::FieldTrialList(
      std::make_unique<variations::SHA1EntropyProvider>("foo")));
  variations::testing::ClearAllVariationParams();
}

void ZeroSuggestProviderTest::CreatePersonalizedFieldTrial() {
  std::map<std::string, std::string> params;
  params[std::string(OmniboxFieldTrial::kZeroSuggestVariantRule)] =
      "Personalized";
  variations::AssociateVariationParams(
      OmniboxFieldTrial::kBundledExperimentFieldTrialName, "A", params);
  base::FieldTrialList::CreateFieldTrial(
      OmniboxFieldTrial::kBundledExperimentFieldTrialName, "A");
}

void ZeroSuggestProviderTest::CreateMostVisitedFieldTrial() {
  std::map<std::string, std::string> params;
  params[std::string(OmniboxFieldTrial::kZeroSuggestVariantRule)] =
      "MostVisitedWithoutSERP";
  variations::AssociateVariationParams(
      OmniboxFieldTrial::kBundledExperimentFieldTrialName, "A", params);
  base::FieldTrialList::CreateFieldTrial(
      OmniboxFieldTrial::kBundledExperimentFieldTrialName, "A");
}

TEST_F(ZeroSuggestProviderTest, TestDoesNotReturnMatchesForPrefix) {
  CreatePersonalizedFieldTrial();

  std::string url("http://www.cnn.com/");
  AutocompleteInput input(base::ASCIIToUTF16(url),
                          metrics::OmniboxEventProto::OTHER,
                          TestSchemeClassifier());
  input.set_current_url(GURL(url));

  // Set up the pref to cache the response from the previous run.
  std::string json_response("[\"\",[\"search1\", \"search2\", \"search3\"],"
      "[],[],{\"google:suggestrelevance\":[602, 601, 600],"
      "\"google:verbatimrelevance\":1300}]");
  PrefService* prefs = client_->GetPrefs();
  prefs->SetString(omnibox::kZeroSuggestCachedResults, json_response);

  provider_->Start(input, false);

  // Expect that matches don't get populated out of cache because we are not
  // in zero suggest mode.
  EXPECT_TRUE(provider_->matches().empty());

  // Expect that fetcher did not get created.
  net::TestURLFetcher* fetcher = test_factory_.GetFetcherByID(1);
  EXPECT_FALSE(fetcher);
}

TEST_F(ZeroSuggestProviderTest, TestStartWillStopForSomeInput) {
  CreatePersonalizedFieldTrial();

  std::string input_url("http://www.cnn.com/");
  AutocompleteInput input(base::ASCIIToUTF16(input_url),
                          metrics::OmniboxEventProto::OTHER,
                          TestSchemeClassifier());
  input.set_current_url(GURL(input_url));
  input.set_from_omnibox_focus(true);

  provider_->Start(input, false);
  EXPECT_FALSE(provider_->done_);

  // Make sure input stops the provider.
  input.set_from_omnibox_focus(false);
  provider_->Start(input, false);
  EXPECT_TRUE(provider_->done_);

  // Make sure invalid input stops the provider.
  input.set_from_omnibox_focus(true);
  provider_->Start(input, false);
  EXPECT_FALSE(provider_->done_);
  AutocompleteInput input2;
  provider_->Start(input2, false);
  EXPECT_TRUE(provider_->done_);
}

TEST_F(ZeroSuggestProviderTest, TestMostVisitedCallback) {
  CreateMostVisitedFieldTrial();

  std::string current_url("http://www.foxnews.com/");
  std::string input_url("http://www.cnn.com/");
  AutocompleteInput input(base::ASCIIToUTF16(input_url),
                          metrics::OmniboxEventProto::OTHER,
                          TestSchemeClassifier());
  input.set_current_url(GURL(current_url));
  input.set_from_omnibox_focus(true);
  history::MostVisitedURLList urls;
  history::MostVisitedURL url(GURL("http://foo.com/"),
                              base::ASCIIToUTF16("Foo"));
  urls.push_back(url);

  provider_->Start(input, false);
  EXPECT_TRUE(provider_->matches().empty());
  scoped_refptr<history::TopSites> top_sites = client_->GetTopSites();
  static_cast<FakeEmptyTopSites*>(top_sites.get())->RunACallback(urls);
  // Should have verbatim match + most visited url match.
  EXPECT_EQ(2U, provider_->matches().size());
  provider_->Stop(false, false);

  provider_->Start(input, false);
  provider_->Stop(false, false);
  EXPECT_TRUE(provider_->matches().empty());
  // Most visited results arriving after Stop() has been called, ensure they
  // are not displayed.
  static_cast<FakeEmptyTopSites*>(top_sites.get())->RunACallback(urls);
  EXPECT_TRUE(provider_->matches().empty());

  history::MostVisitedURLList urls2;
  urls2.push_back(history::MostVisitedURL(GURL("http://bar.com/"),
                                          base::ASCIIToUTF16("Bar")));
  urls2.push_back(history::MostVisitedURL(GURL("http://zinga.com/"),
                                          base::ASCIIToUTF16("Zinga")));
  provider_->Start(input, false);
  provider_->Stop(false, false);
  provider_->Start(input, false);
  static_cast<FakeEmptyTopSites*>(top_sites.get())->RunACallback(urls);
  // Stale results should get rejected.
  EXPECT_TRUE(provider_->matches().empty());
  static_cast<FakeEmptyTopSites*>(top_sites.get())->RunACallback(urls2);
  EXPECT_FALSE(provider_->matches().empty());
  provider_->Stop(false, false);
}

TEST_F(ZeroSuggestProviderTest, TestMostVisitedNavigateToSearchPage) {
  CreateMostVisitedFieldTrial();

  std::string current_url("http://www.foxnews.com/");
  std::string input_url("http://www.cnn.com/");
  AutocompleteInput input(base::ASCIIToUTF16(input_url),
                          metrics::OmniboxEventProto::OTHER,
                          TestSchemeClassifier());
  input.set_current_url(GURL(current_url));
  input.set_from_omnibox_focus(true);
  history::MostVisitedURLList urls;
  history::MostVisitedURL url(GURL("http://foo.com/"),
                              base::ASCIIToUTF16("Foo"));
  urls.push_back(url);

  provider_->Start(input, false);
  EXPECT_TRUE(provider_->matches().empty());
  // Stop() doesn't always get called.

  std::string search_url("https://www.google.com/?q=flowers");
  AutocompleteInput srp_input(
      base::ASCIIToUTF16(search_url),
      metrics::OmniboxEventProto::SEARCH_RESULT_PAGE_NO_SEARCH_TERM_REPLACEMENT,
      TestSchemeClassifier());
  srp_input.set_current_url(GURL(search_url));
  srp_input.set_from_omnibox_focus(true);

  provider_->Start(srp_input, false);
  EXPECT_TRUE(provider_->matches().empty());
  // Most visited results arriving after a new request has been started.
  scoped_refptr<history::TopSites> top_sites = client_->GetTopSites();
  static_cast<FakeEmptyTopSites*>(top_sites.get())->RunACallback(urls);
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(ZeroSuggestProviderTest, TestPsuggestZeroSuggestCachingFirstRun) {
  CreatePersonalizedFieldTrial();
  EXPECT_CALL(*client_, IsAuthenticated())
      .WillRepeatedly(testing::Return(true));

  // Ensure the cache is empty.
  PrefService* prefs = client_->GetPrefs();
  prefs->SetString(omnibox::kZeroSuggestCachedResults, std::string());

  std::string url("http://www.cnn.com/");
  AutocompleteInput input(base::ASCIIToUTF16(url),
                          metrics::OmniboxEventProto::OTHER,
                          TestSchemeClassifier());
  input.set_current_url(GURL(url));
  input.set_from_omnibox_focus(true);

  provider_->Start(input, false);

  EXPECT_TRUE(prefs->GetString(omnibox::kZeroSuggestCachedResults).empty());
  EXPECT_TRUE(provider_->matches().empty());

  net::TestURLFetcher* fetcher = test_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(200);
  std::string json_response("[\"\",[\"search1\", \"search2\", \"search3\"],"
      "[],[],{\"google:suggestrelevance\":[602, 601, 600],"
      "\"google:verbatimrelevance\":1300}]");
  fetcher->SetResponseString(json_response);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(4U, provider_->matches().size());  // 3 results + verbatim
  EXPECT_EQ(json_response,
            prefs->GetString(omnibox::kZeroSuggestCachedResults));
}

TEST_F(ZeroSuggestProviderTest, TestPsuggestZeroSuggestHasCachedResults) {
  CreatePersonalizedFieldTrial();
  EXPECT_CALL(*client_, IsAuthenticated())
      .WillRepeatedly(testing::Return(true));

  std::string url("http://www.cnn.com/");
  AutocompleteInput input(base::ASCIIToUTF16(url),
                          metrics::OmniboxEventProto::OTHER,
                          TestSchemeClassifier());
  input.set_current_url(GURL(url));
  input.set_from_omnibox_focus(true);

  // Set up the pref to cache the response from the previous run.
  std::string json_response("[\"\",[\"search1\", \"search2\", \"search3\"],"
      "[],[],{\"google:suggestrelevance\":[602, 601, 600],"
      "\"google:verbatimrelevance\":1300}]");
  PrefService* prefs = client_->GetPrefs();
  prefs->SetString(omnibox::kZeroSuggestCachedResults, json_response);

  provider_->Start(input, false);

  // Expect that matches get populated synchronously out of the cache.
  ASSERT_EQ(4U, provider_->matches().size());
  EXPECT_EQ(base::ASCIIToUTF16("search1"), provider_->matches()[1].contents);
  EXPECT_EQ(base::ASCIIToUTF16("search2"), provider_->matches()[2].contents);
  EXPECT_EQ(base::ASCIIToUTF16("search3"), provider_->matches()[3].contents);

  net::TestURLFetcher* fetcher = test_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(200);
  std::string json_response2("[\"\",[\"search4\", \"search5\", \"search6\"],"
      "[],[],{\"google:suggestrelevance\":[602, 601, 600],"
      "\"google:verbatimrelevance\":1300}]");
  fetcher->SetResponseString(json_response2);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  base::RunLoop().RunUntilIdle();

  // Expect the same 4 results after the response has been handled.
  ASSERT_EQ(4U, provider_->matches().size());
  EXPECT_EQ(base::ASCIIToUTF16("search1"), provider_->matches()[1].contents);
  EXPECT_EQ(base::ASCIIToUTF16("search2"), provider_->matches()[2].contents);
  EXPECT_EQ(base::ASCIIToUTF16("search3"), provider_->matches()[3].contents);

  // Expect the new results have been stored.
  EXPECT_EQ(json_response2,
            prefs->GetString(omnibox::kZeroSuggestCachedResults));
}

TEST_F(ZeroSuggestProviderTest, TestPsuggestZeroSuggestReceivedEmptyResults) {
  CreatePersonalizedFieldTrial();
  EXPECT_CALL(*client_, IsAuthenticated())
      .WillRepeatedly(testing::Return(true));

  std::string url("http://www.cnn.com/");
  AutocompleteInput input(base::ASCIIToUTF16(url),
                          metrics::OmniboxEventProto::OTHER,
                          TestSchemeClassifier());
  input.set_current_url(GURL(url));
  input.set_from_omnibox_focus(true);

  // Set up the pref to cache the response from the previous run.
  std::string json_response("[\"\",[\"search1\", \"search2\", \"search3\"],"
      "[],[],{\"google:suggestrelevance\":[602, 601, 600],"
      "\"google:verbatimrelevance\":1300}]");
  PrefService* prefs = client_->GetPrefs();
  prefs->SetString(omnibox::kZeroSuggestCachedResults, json_response);

  provider_->Start(input, false);

  // Expect that matches get populated synchronously out of the cache.
  ASSERT_EQ(4U, provider_->matches().size());
  EXPECT_EQ(base::ASCIIToUTF16("search1"), provider_->matches()[1].contents);
  EXPECT_EQ(base::ASCIIToUTF16("search2"), provider_->matches()[2].contents);
  EXPECT_EQ(base::ASCIIToUTF16("search3"), provider_->matches()[3].contents);

  net::TestURLFetcher* fetcher = test_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(200);
  std::string empty_response("[\"\",[],[],[],{}]");
  fetcher->SetResponseString(empty_response);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  base::RunLoop().RunUntilIdle();

  // Expect that the matches have been cleared.
  ASSERT_TRUE(provider_->matches().empty());

  // Expect the new results have been stored.
  EXPECT_EQ(empty_response,
            prefs->GetString(omnibox::kZeroSuggestCachedResults));
}
