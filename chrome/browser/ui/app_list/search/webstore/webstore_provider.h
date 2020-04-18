// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_WEBSTORE_WEBSTORE_PROVIDER_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_WEBSTORE_WEBSTORE_PROVIDER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "chrome/browser/ui/app_list/search/common/webservice_search_provider.h"

class AppListControllerDelegate;
class ChromeSearchResult;

namespace base {
class DictionaryValue;
}

namespace app_list {

namespace test {
class WebstoreProviderTest;
}

class JSONResponseFetcher;
class TokenizedString;

// WebstoreProvider fetches search results from the web store server.
// A "Search in web store" result will be returned if the server does not
// return any results.
class WebstoreProvider : public WebserviceSearchProvider{
 public:
  WebstoreProvider(Profile* profile, AppListControllerDelegate* controller);
  ~WebstoreProvider() override;

  // SearchProvider overrides:
  void Start(const base::string16& query) override;

 private:
  friend class app_list::test::WebstoreProviderTest;

  // Start the search request with |query_|.
  void StartQuery();

  void OnWebstoreSearchFetched(std::unique_ptr<base::DictionaryValue> json);
  void ProcessWebstoreSearchResults(const base::DictionaryValue* json);
  std::unique_ptr<ChromeSearchResult> CreateResult(
      const TokenizedString& query,
      const base::DictionaryValue& dict);

  void set_webstore_search_fetched_callback(const base::Closure& callback) {
    webstore_search_fetched_callback_ = callback;
  }

  AppListControllerDelegate* controller_;
  std::unique_ptr<JSONResponseFetcher> webstore_search_;
  base::Closure webstore_search_fetched_callback_;

  // The current query.
  std::string query_;

  // Whether there is currently a query pending.
  bool query_pending_;

  DISALLOW_COPY_AND_ASSIGN(WebstoreProvider);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_WEBSTORE_WEBSTORE_PROVIDER_H_
