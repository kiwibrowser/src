// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/omnibox/chrome_omnibox_navigation_observer.h"

#include <unordered_map>
#include <vector>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/search_engines/template_url_service_factory_test_util.h"
#include "chrome/test/base/testing_profile.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/web_contents_tester.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"
#include "testing/gtest/include/gtest/gtest.h"

// Exactly like net::FakeURLFetcher except has GetURL() return the redirect
// destination.  (Normal FakeURLFetchers return the requested URL in response
// to GetURL(), not the current URL the fetcher is processing.)
class RedirectedURLFetcher : public net::FakeURLFetcher {
 public:
  RedirectedURLFetcher(const GURL& url,
                       const GURL& destination_url,
                       net::URLFetcherDelegate* d,
                       const std::string& response_data,
                       net::HttpStatusCode response_code,
                       net::URLRequestStatus::Status status)
      : net::FakeURLFetcher(url, d, response_data, response_code, status),
        destination_url_(destination_url) {}

  const GURL& GetURL() const override { return destination_url_; }

 private:
  GURL destination_url_;

  DISALLOW_COPY_AND_ASSIGN(RedirectedURLFetcher);
};

// Used for constructing a FakeURLFetcher with a server-side redirect.  Server-
// side redirects are provided using response headers.  FakeURLFetcherFactory
// do not provide the ability to deliver response headers; this class does.
class RedirectURLFetcherFactory : public net::URLFetcherFactory {
 public:
  RedirectURLFetcherFactory() : net::URLFetcherFactory() {}

  // Sets this factory to, in response to a request for |origin|, return a 301
  // (redirection) response with the appropriate response headers to indicate a
  // redirect to |destination|.
  void SetRedirectLocation(const std::string& origin,
                           const std::string& destination) {
    redirections_[origin] = destination;
  }

  // net::URLFetcherFactory:
  std::unique_ptr<net::URLFetcher> CreateURLFetcher(
      int id,
      const GURL& url,
      net::URLFetcher::RequestType request_type,
      net::URLFetcherDelegate* delegate,
      net::NetworkTrafficAnnotationTag traffic_annotation) override {
    const std::string& url_spec = url.spec();
    EXPECT_TRUE(redirections_.find(url_spec) != redirections_.end())
        << url_spec;
    auto* fetcher = new RedirectedURLFetcher(
        url, GURL(redirections_[url_spec]), delegate, std::string(),
        net::HTTP_MOVED_PERMANENTLY, net::URLRequestStatus::CANCELED);
    std::string headers = "HTTP/1.0 301 Moved Permanently\nLocation: " +
                          redirections_[url_spec] + "\n";
    fetcher->set_response_headers(scoped_refptr<net::HttpResponseHeaders>(
        new net::HttpResponseHeaders(headers)));
    return std::unique_ptr<net::URLFetcher>(fetcher);
  }

 private:
  std::unordered_map<std::string, std::string> redirections_;

  DISALLOW_COPY_AND_ASSIGN(RedirectURLFetcherFactory);
};

// A trival ChromeOmniboxNavigationObserver that keeps track of whether
// CreateAlternateNavInfoBar() has been called.
class MockChromeOmniboxNavigationObserver
    : public ChromeOmniboxNavigationObserver {
 public:
  MockChromeOmniboxNavigationObserver(
      Profile* profile,
      const base::string16& text,
      const AutocompleteMatch& match,
      const AutocompleteMatch& alternate_nav_match,
      bool* displayed_infobar)
      : ChromeOmniboxNavigationObserver(profile,
                                        text,
                                        match,
                                        alternate_nav_match),
        displayed_infobar_(displayed_infobar) {
    *displayed_infobar_ = false;
  }

 protected:
  void CreateAlternateNavInfoBar() override { *displayed_infobar_ = true; }

 private:
  // True if CreateAlternateNavInfoBar was called.  This cannot be kept in
  // memory within this class because this class is automatically deleted when
  // all fetchers finish (before the test can query this value), hence the
  // pointer.
  bool* displayed_infobar_;

  DISALLOW_COPY_AND_ASSIGN(MockChromeOmniboxNavigationObserver);
};

class ChromeOmniboxNavigationObserverTest : public testing::Test {
 protected:
  ChromeOmniboxNavigationObserverTest() {}
  ~ChromeOmniboxNavigationObserverTest() override {}

  content::NavigationController* navigation_controller() {
    return &(web_contents_->GetController());
  }

  TestingProfile* profile() { return &profile_; }
  TemplateURLService* model() {
    return TemplateURLServiceFactory::GetForProfile(&profile_);
  }

  // Functions that return the name of certain search keywords that are part
  // of the TemplateURLService attached to this profile.
  static base::string16 auto_generated_search_keyword() {
    return base::ASCIIToUTF16("auto_generated_search_keyword");
  }
  static base::string16 non_auto_generated_search_keyword() {
    return base::ASCIIToUTF16("non_auto_generated_search_keyword");
  }
  static base::string16 default_search_keyword() {
    return base::ASCIIToUTF16("default_search_keyword");
  }
  static base::string16 prepopulated_search_keyword() {
    return base::ASCIIToUTF16("prepopulated_search_keyword");
  }
  static base::string16 policy_search_keyword() {
    return base::ASCIIToUTF16("policy_search_keyword");
  }

 private:
  // testing::Test:
  void SetUp() override;

  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  TestingProfile profile_;
  std::unique_ptr<content::WebContents> web_contents_;

  DISALLOW_COPY_AND_ASSIGN(ChromeOmniboxNavigationObserverTest);
};

void ChromeOmniboxNavigationObserverTest::SetUp() {
  web_contents_ = content::WebContentsTester::CreateTestWebContents(
      profile(), content::SiteInstance::Create(profile()));

  InfoBarService::CreateForWebContents(web_contents_.get());

  // Set up a series of search engines for later testing.
  TemplateURLServiceFactoryTestUtil factory_util(profile());
  factory_util.VerifyLoad();

  TemplateURLData auto_gen_turl;
  auto_gen_turl.SetKeyword(auto_generated_search_keyword());
  auto_gen_turl.safe_for_autoreplace = true;
  factory_util.model()->Add(std::make_unique<TemplateURL>(auto_gen_turl));

  TemplateURLData non_auto_gen_turl;
  non_auto_gen_turl.SetKeyword(non_auto_generated_search_keyword());
  factory_util.model()->Add(std::make_unique<TemplateURL>(non_auto_gen_turl));

  TemplateURLData default_turl;
  default_turl.SetKeyword(default_search_keyword());
  factory_util.model()->SetUserSelectedDefaultSearchProvider(
      factory_util.model()->Add(std::make_unique<TemplateURL>(default_turl)));

  TemplateURLData prepopulated_turl;
  prepopulated_turl.SetKeyword(prepopulated_search_keyword());
  prepopulated_turl.prepopulate_id = 1;
  factory_util.model()->Add(std::make_unique<TemplateURL>(prepopulated_turl));

  TemplateURLData policy_turl;
  policy_turl.SetKeyword(policy_search_keyword());
  policy_turl.created_by_policy = true;
  factory_util.model()->Add(std::make_unique<TemplateURL>(policy_turl));
}

TEST_F(ChromeOmniboxNavigationObserverTest, LoadStateAfterPendingNavigation) {
  std::unique_ptr<ChromeOmniboxNavigationObserver> observer =
      std::make_unique<ChromeOmniboxNavigationObserver>(
          profile(), base::ASCIIToUTF16("test text"), AutocompleteMatch(),
          AutocompleteMatch());
  EXPECT_EQ(ChromeOmniboxNavigationObserver::LOAD_NOT_SEEN,
            observer->load_state());

  std::unique_ptr<content::NavigationEntry> entry =
      content::NavigationController::CreateNavigationEntry(
          GURL(), content::Referrer(), ui::PAGE_TRANSITION_FROM_ADDRESS_BAR,
          false, std::string(), profile(),
          nullptr /* blob_url_loader_factory */);

  content::NotificationService::current()->Notify(
      content::NOTIFICATION_NAV_ENTRY_PENDING,
      content::Source<content::NavigationController>(navigation_controller()),
      content::Details<content::NavigationEntry>(entry.get()));

  // A pending navigation notification should synchronously update the load
  // state to pending.
  EXPECT_EQ(ChromeOmniboxNavigationObserver::LOAD_PENDING,
            observer->load_state());
}

TEST_F(ChromeOmniboxNavigationObserverTest, DeleteBrokenCustomSearchEngines) {
  struct TestData {
    base::string16 keyword;
    int status_code;
    bool expect_exists;
  };
  std::vector<TestData> cases = {
      {auto_generated_search_keyword(), 200, true},
      {auto_generated_search_keyword(), 404, false},
      {non_auto_generated_search_keyword(), 404, true},
      {default_search_keyword(), 404, true},
      {prepopulated_search_keyword(), 404, true},
      {policy_search_keyword(), 404, true}};

  base::string16 query = base::ASCIIToUTF16(" text");
  for (size_t i = 0; i < cases.size(); ++i) {
    SCOPED_TRACE("case #" + base::IntToString(i));
    // The keyword should always exist at the beginning.
    EXPECT_TRUE(model()->GetTemplateURLForKeyword(cases[i].keyword) != nullptr);

    AutocompleteMatch match;
    match.keyword = cases[i].keyword;
    // |observer| gets deleted by observer->NavigationEntryCommitted().
    ChromeOmniboxNavigationObserver* observer =
        new ChromeOmniboxNavigationObserver(profile(), cases[i].keyword + query,
                                            match, AutocompleteMatch());
    auto navigation_entry =
        content::NavigationController::CreateNavigationEntry(
            GURL(), content::Referrer(), ui::PAGE_TRANSITION_FROM_ADDRESS_BAR,
            false, std::string(), profile(),
            nullptr /* blob_url_loader_factory */);
    content::LoadCommittedDetails details;
    details.http_status_code = cases[i].status_code;
    details.entry = navigation_entry.get();
    observer->NavigationEntryCommitted(details);
    EXPECT_EQ(cases[i].expect_exists,
              model()->GetTemplateURLForKeyword(cases[i].keyword) != nullptr);
  }

  // Also run a URL navigation that results in a 404 through the system to make
  // sure nothing crashes for regular URL navigations.
  // |observer| gets deleted by observer->NavigationEntryCommitted().
  ChromeOmniboxNavigationObserver* observer =
      new ChromeOmniboxNavigationObserver(
          profile(), base::ASCIIToUTF16("url navigation"), AutocompleteMatch(),
          AutocompleteMatch());
  auto navigation_entry = content::NavigationController::CreateNavigationEntry(
      GURL(), content::Referrer(), ui::PAGE_TRANSITION_FROM_ADDRESS_BAR, false,
      std::string(), profile(), nullptr /* blob_url_loader_factory */);
  content::LoadCommittedDetails details;
  details.http_status_code = 404;
  details.entry = navigation_entry.get();
  observer->NavigationEntryCommitted(details);
}

TEST_F(ChromeOmniboxNavigationObserverTest, AlternateNavInfoBar) {
  struct Response {
    const std::string requested_url;
    const bool net_request_successful;
    const int http_response_code;
    // Only needed if |http_response_code| is a redirection.
    const std::string redirected_url;
  };
  const Response kNoResponse = {std::string(), false, 0, std::string()};

  // All of these test cases assume the alternate nav URL is http://example/.
  struct Case {
    const Response responses[2];
    const bool expected_alternate_nav_bar_shown;
  } cases[] = {
      // The only response provided is a net error.
      {{{"http://example/", false, 0, std::string()}, kNoResponse}, false},
      // The response connected to a valid page.
      {{{"http://example/", true, 200, std::string()}, kNoResponse}, true},
      // The response connected to an error page.
      {{{"http://example/", true, 404, std::string()}, kNoResponse}, false},

      // The response redirected to same host, just http->https, with a path
      // change as well.  In this case the second URL should not fetched; Chrome
      // will optimistically assume the destination will return a valid page and
      // display the infobar.
      {{{"http://example/", true, 301, "https://example/path"}, kNoResponse},
       true},
      // Ditto, making sure it still holds when the final destination URL
      // returns a valid status code.
      {{{"http://example/", true, 301, "https://example/path"},
        {"https://example/path", true, 200, std::string()}},
       true},

      // The response redirected to an entirely different host.  In these cases,
      // no URL should be fetched against this second host; again Chrome will
      // optimistically assume the destination will return a valid page and
      // display the infobar.
      {{{"http://example/", true, 301, "http://new-destination/"}, kNoResponse},
       true},
      // Ditto, making sure it still holds when the final destination URL
      // returns a valid status code.
      {{{"http://example/", true, 301, "http://new-destination/"},
        {"http://new-destination/", true, 200, std::string()}},
       true},

      // The response redirected to same host, just http->https, with no other
      // changes.  In these cases, Chrome will fetch the second URL.
      // The second URL response returned a valid page.
      {{{"http://example/", true, 301, "https://example/"},
        {"https://example/", true, 200}},
       true},
      // The second URL response returned an error page.
      {{{"http://example/", true, 301, "https://example/"},
        {"https://example/", true, 404}},
       false},
      // The second URL response returned a net error.
      {{{"http://example/", true, 301, "https://example/"},
        {"https://example/", false, 0}},
       false},
      // The second URL response redirected again.
      {{{"http://example/", true, 301, "https://example/"},
        {"https://example/", true, 301, "https://example/root"}},
       true},
  };
  for (size_t i = 0; i < arraysize(cases); ++i) {
    SCOPED_TRACE("case #" + base::IntToString(i));
    const Case& test_case = cases[i];

    // Set the URL request responses.
    RedirectURLFetcherFactory redirecter_factory;
    net::FakeURLFetcherFactory factory(&redirecter_factory);
    for (size_t j = 0; (j < 2) && !test_case.responses[j].requested_url.empty();
         ++j) {
      if (!test_case.responses[j].redirected_url.empty()) {
        redirecter_factory.SetRedirectLocation(
            test_case.responses[j].requested_url,
            test_case.responses[j].redirected_url);
      } else {
        // Not a redirected URL.  Used the regular FakeURLFetcherFactory
        // interface.
        factory.SetFakeResponse(GURL(test_case.responses[j].requested_url),
                                std::string(),  // empty response
                                static_cast<net::HttpStatusCode>(
                                    test_case.responses[j].http_response_code),
                                test_case.responses[j].net_request_successful
                                    ? net::URLRequestStatus::SUCCESS
                                    : net::URLRequestStatus::FAILED);
      }
    }

    // Create the alternate nav match and the observer.
    // |observer| gets deleted automatically after all fetchers complete.
    AutocompleteMatch alternate_nav_match;
    alternate_nav_match.destination_url = GURL("http://example/");
    bool displayed_infobar;
    ChromeOmniboxNavigationObserver* observer =
        new MockChromeOmniboxNavigationObserver(
            profile(), base::ASCIIToUTF16("example"), AutocompleteMatch(),
            alternate_nav_match, &displayed_infobar);

    // Send the observer NAV_ENTRY_PENDING to get the URL fetcher to start.
    auto navigation_entry =
        content::NavigationController::CreateNavigationEntry(
            GURL(), content::Referrer(), ui::PAGE_TRANSITION_FROM_ADDRESS_BAR,
            false, std::string(), profile(),
            nullptr /* blob_url_loader_factory */);
    content::NotificationService::current()->Notify(
        content::NOTIFICATION_NAV_ENTRY_PENDING,
        content::Source<content::NavigationController>(navigation_controller()),
        content::Details<content::NavigationEntry>(navigation_entry.get()));

    // Make sure the fetcher(s) have finished.
    base::RunLoop().RunUntilIdle();

    // Send the observer NavigationEntryCommitted() to get it to display the
    // infobar if needed.
    content::LoadCommittedDetails details;
    details.http_status_code = 200;
    details.entry = navigation_entry.get();
    observer->NavigationEntryCommitted(details);

    // See if AlternateNavInfoBarDelegate::Create() was called.
    EXPECT_EQ(test_case.expected_alternate_nav_bar_shown, displayed_infobar);
  }
}
