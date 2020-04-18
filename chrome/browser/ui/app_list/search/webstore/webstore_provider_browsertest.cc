// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/webstore/webstore_provider.h"

#include <stddef.h>

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "ash/public/cpp/app_list/app_list_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/app_list/search/chrome_search_result.h"
#include "chrome/browser/ui/app_list/search/webstore/webstore_result.h"
#include "chrome/browser/ui/app_list/test/test_app_list_controller_delegate.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension_test_util.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/extension_urls.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

using content::BrowserThread;
using extensions::Manifest;
using net::test_server::BasicHttpResponse;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;

namespace app_list {
namespace test {
namespace {

const char kWebstoreUrlPlaceholder[] = "[webstore_url]";

// Mock results.
const char kOneResult[] =
    "{"
    "\"search_url\": \"http://host/search\","
    "\"results\":["
    "  {"
    "    \"id\": \"app1_id\","
    "    \"localized_name\": \"Fun App\","
    "    \"icon_url\": \"http://host/icon\","
    "    \"is_paid\": false"
    "  }"
    "]}";

const char kThreeResults[] =
    "{"
    "\"search_url\": \"http://host/search\","
    "\"results\":["
    "  {"
    "    \"id\": \"app1_id\","
    "    \"localized_name\": \"Mystery App\","
    "    \"icon_url\": \"http://host/icon1\","
    "    \"is_paid\": true,"
    "    \"item_type\": \"PLATFORM_APP\""
    "  },"
    "  {"
    "    \"id\": \"app2_id\","
    "    \"localized_name\": \"App Mystère\","
    "    \"icon_url\": \"http://host/icon2\","
    "    \"is_paid\": false,"
    "    \"item_type\": \"HOSTED_APP\""
    "  },"
    "  {"
    "    \"id\": \"app3_id\","
    "    \"localized_name\": \"Mistero App\","
    "    \"icon_url\": \"http://host/icon3\","
    "    \"is_paid\": false,"
    "    \"item_type\": \"LEGACY_PACKAGED_APP\""
    "  }"
    "]}";

struct ParsedSearchResult {
  const char* id;
  const char* title;
  const char* icon_url;
  bool is_paid;
  Manifest::Type item_type;
  size_t num_actions;
};

// Expected results from a search for "fun" on kOneResult.
ParsedSearchResult kParsedOneResult[] = {{"app1_id",
                                          "[Fun] App",
                                          "http://host/icon",
                                          false,
                                          Manifest::TYPE_UNKNOWN,
                                          1}};

ParsedSearchResult kParsedOneStoreSearchResult[] = {{kWebstoreUrlPlaceholder,
                                                     "fun",
                                                     "http://host/icon",
                                                     false,
                                                     Manifest::TYPE_UNKNOWN,
                                                     0}};

// Expected results from a search for "app" on kThreeResults.
ParsedSearchResult kParsedThreeResultsApp[] = {
    {"app1_id",
     "Mystery [App]",
     "http://host/icon1",
     true,
     Manifest::TYPE_PLATFORM_APP,
     1},
    {"app2_id",
     "[App] Mystère",
     "http://host/icon2",
     false,
     Manifest::TYPE_HOSTED_APP,
     1},
    {"app3_id",
     "Mistero [App]",
     "http://host/icon3",
     false,
     Manifest::TYPE_LEGACY_PACKAGED_APP,
     1}};

// Expected results from a search for "myst" on kThreeResults.
ParsedSearchResult kParsedThreeResultsMyst[] = {{"app1_id",
                                                 "[Myst]ery App",
                                                 "http://host/icon1",
                                                 true,
                                                 Manifest::TYPE_PLATFORM_APP,
                                                 1},
                                                {"app2_id",
                                                 "App [Myst]ère",
                                                 "http://host/icon2",
                                                 false,
                                                 Manifest::TYPE_HOSTED_APP,
                                                 1}};

class AppListControllerDelegateForTest
    : public ::test::TestAppListControllerDelegate {
 public:
  AppListControllerDelegateForTest() {}
  ~AppListControllerDelegateForTest() override {}

  void MockInstallApp(const std::string& id) { installed_ids_.insert(id); }

  bool IsExtensionInstalled(Profile*, const std::string& app_id) override {
    return installed_ids_.find(app_id) != installed_ids_.end();
  }

 private:
  std::set<std::string> installed_ids_;
};

}  // namespace

class WebstoreProviderTest : public InProcessBrowserTest {
 public:
  WebstoreProviderTest() {}
  ~WebstoreProviderTest() override {}

  // InProcessBrowserTest overrides:
  void SetUpOnMainThread() override {
    embedded_test_server()->RegisterRequestHandler(
        base::Bind(&WebstoreProviderTest::HandleRequest,
                   base::Unretained(this)));
    ASSERT_TRUE(embedded_test_server()->Start());
    // Minor hack: the gallery URL is expected not to end with a slash. Just
    // append "path" to maintain this.
    const GURL gallery_url(embedded_test_server()->base_url().spec() + "path");
    extension_test_util::SetGalleryURL(gallery_url);

    mock_controller_.reset(new AppListControllerDelegateForTest);
    webstore_provider_.reset(new WebstoreProvider(
        ProfileManager::GetActiveUserProfile(), mock_controller_.get()));
    webstore_provider_->set_webstore_search_fetched_callback(
        base::Bind(&WebstoreProviderTest::OnSearchResultsFetched,
                   base::Unretained(this)));
    // TODO(mukai): add test cases for throttling.
    webstore_provider_->set_use_throttling(false);
  }

  void RunQuery(const std::string& query,
                const std::string& mock_server_response) {
    mock_server_response_ = mock_server_response;
    webstore_provider_->Start(base::UTF8ToUTF16(query));

    if (webstore_provider_->query_pending_ && !mock_server_response.empty()) {
      DCHECK(!run_loop_);
      run_loop_.reset(new base::RunLoop);
      run_loop_->Run();
      run_loop_.reset();

      mock_server_response_.clear();
    }
  }

  std::string GetResultTitles() const {
    std::string results;
    for (SearchProvider::Results::const_iterator it =
             webstore_provider_->results().begin();
         it != webstore_provider_->results().end();
         ++it) {
      if (!results.empty())
        results += ',';
      results += base::UTF16ToUTF8((*it)->title());
    }
    return results;
  }

  void VerifyResults(const ParsedSearchResult* expected_results,
                     size_t expected_result_size) {
    ASSERT_EQ(expected_result_size, webstore_provider_->results().size());
    for (size_t i = 0; i < expected_result_size; ++i) {
      const ChromeSearchResult* result =
          (webstore_provider_->results()[i]).get();
      // A search for an installed app will return a general webstore search
      // instead of an app in the webstore.
      if (!strcmp(expected_results[i].id, kWebstoreUrlPlaceholder)) {
        EXPECT_EQ(
            extension_urls::GetWebstoreSearchPageUrl(expected_results[i].title)
                .spec(),
            result->id());
      } else {
        EXPECT_EQ(
            WebstoreResult::GetResultIdFromExtensionId(expected_results[i].id),
            result->id());

        const WebstoreResult* webstore_result =
            static_cast<const WebstoreResult*>(result);
        EXPECT_EQ(expected_results[i].id, webstore_result->app_id());
        EXPECT_EQ(expected_results[i].icon_url,
                  webstore_result->icon_url().spec());
        EXPECT_EQ(expected_results[i].is_paid, webstore_result->is_paid());
        EXPECT_EQ(expected_results[i].item_type, webstore_result->item_type());
      }

      EXPECT_EQ(std::string(expected_results[i].title),
                ChromeSearchResult::TagsDebugStringForTest(
                    base::UTF16ToUTF8(result->title()), result->title_tags()));

      // Ensure the number of action buttons is appropriate for the item type.
      EXPECT_EQ(expected_results[i].num_actions, result->actions().size());
    }
  }

  void RunQueryAndVerify(const std::string& query,
                         const std::string& mock_server_response,
                         const ParsedSearchResult* expected_results,
                         size_t expected_result_size) {
    RunQuery(query, mock_server_response);
    VerifyResults(expected_results, expected_result_size);
  }

  WebstoreProvider* webstore_provider() { return webstore_provider_.get(); }
  AppListControllerDelegateForTest* mock_controller() {
    return mock_controller_.get();
  }

 private:
  std::unique_ptr<HttpResponse> HandleRequest(const HttpRequest& request) {
    std::unique_ptr<BasicHttpResponse> response(new BasicHttpResponse);

    if (request.relative_url.find("/jsonsearch?") != std::string::npos) {
      if (mock_server_response_ == "ERROR_NOT_FOUND") {
        response->set_code(net::HTTP_NOT_FOUND);
      } else if (mock_server_response_ == "ERROR_INTERNAL_SERVER_ERROR") {
        response->set_code(net::HTTP_INTERNAL_SERVER_ERROR);
      } else {
        response->set_code(net::HTTP_OK);
        response->set_content(mock_server_response_);
      }
    }

    return std::move(response);
  }

  void OnSearchResultsFetched() {
    if (run_loop_)
      run_loop_->Quit();
  }

  std::unique_ptr<base::RunLoop> run_loop_;

  std::string mock_server_response_;

  std::unique_ptr<WebstoreProvider> webstore_provider_;

  std::unique_ptr<AppListControllerDelegateForTest> mock_controller_;

  DISALLOW_COPY_AND_ASSIGN(WebstoreProviderTest);
};

IN_PROC_BROWSER_TEST_F(WebstoreProviderTest, Basic) {
  struct {
    const char* query;
    const char* mock_server_response;
    const char* expected_result_titles;
    const ParsedSearchResult* expected_results;
    size_t expected_result_size;
  } kTestCases[] = {
      // Note: If a search results in an error, or returns 0 results, we expect
      // the webstore provider to leave a placeholder "search in web store"
      // result with the same title as the search query. So all cases where
      // |expected_result_titles| == |query| means we are expecting an error.

      // A search that returns 0 results.
      {"synchronous", "", "synchronous", nullptr, 0},
      // Getting an error response from the server (note: the responses
      // "ERROR_NOT_FOUND" and "ERROR_INTERNAL_SERVER_ERROR" are treated
      // specially by HandleResponse).
      {"404", "ERROR_NOT_FOUND", "404", nullptr, 0},
      {"500", "ERROR_INTERNAL_SERVER_ERROR", "500", nullptr, 0},
      // Getting bad JSON from the server.
      {"bad json", "invalid json", "bad json", nullptr, 0},
      // Good results. Note that the search term appears in all of the result
      // titles.
      {"fun", kOneResult, "Fun App", kParsedOneResult, 1},
      {"app",
       kThreeResults,
       "Mystery App,App Mystère,Mistero App",
       kParsedThreeResultsApp,
       3},
      // Search where one of the results does not include the query term. Only
      // the results with a title matching the query should be selected.
      {"myst",
       kThreeResults,
       "Mystery App,App Mystère",
       kParsedThreeResultsMyst,
       2},
  };

  for (size_t i = 0; i < arraysize(kTestCases); ++i) {
    if (kTestCases[i].expected_result_titles) {
      RunQuery(kTestCases[i].query, kTestCases[i].mock_server_response);
      EXPECT_EQ(kTestCases[i].expected_result_titles, GetResultTitles())
          << "Case " << i << ": q=" << kTestCases[i].query;

      if (kTestCases[i].expected_results) {
        VerifyResults(kTestCases[i].expected_results,
                      kTestCases[i].expected_result_size);
      }
    }
  }
}

IN_PROC_BROWSER_TEST_F(WebstoreProviderTest, NoSearchForSensitiveData) {
  // None of the following input strings should be accepted because they may
  // contain private data.
  const char* inputs[] = {
      // file: scheme is bad.
      "file://filename",
      "FILE://filename",
      // URLs with usernames, ports, queries or refs are bad.
      "http://username:password@hostname/",
      "http://www.example.com:1000",
      "http://foo:1000",
      "http://hostname/?query=q",
      "http://hostname/path#ref",
      // A https URL with path is bad.
      "https://hostname/path",
  };

  for (size_t i = 0; i < arraysize(inputs); ++i) {
    RunQueryAndVerify(inputs[i], kOneResult, nullptr, 0);
  }
}

IN_PROC_BROWSER_TEST_F(WebstoreProviderTest, NoSearchForShortQueries) {
  RunQueryAndVerify("f", kOneResult, nullptr, 0);
  RunQueryAndVerify("fu", kOneResult, nullptr, 0);
  RunQueryAndVerify("fun", kOneResult, kParsedOneResult, 1);
}

IN_PROC_BROWSER_TEST_F(WebstoreProviderTest, SearchCache) {
  RunQueryAndVerify("fun", kOneResult, kParsedOneResult, 1);

  // No result is provided but the provider gets the result from the cache.
  RunQueryAndVerify("fun", "", kParsedOneResult, 1);
}

IN_PROC_BROWSER_TEST_F(WebstoreProviderTest, IgnoreInstalledApps) {
  mock_controller()->MockInstallApp("app1_id");
  RunQueryAndVerify("fun", kOneResult, kParsedOneStoreSearchResult, 1);
}

}  // namespace test
}  // namespace app_list
