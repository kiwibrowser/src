// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/webstore/webstore_provider.h"

#include <string>
#include <utility>

#include "ash/public/cpp/app_list/tokenized_string.h"
#include "ash/public/cpp/app_list/tokenized_string_match.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/search/common/json_response_fetcher.h"
#include "chrome/browser/ui/app_list/search/search_webstore_result.h"
#include "chrome/browser/ui/app_list/search/webstore/webstore_result.h"
#include "extensions/common/extension_urls.h"
#include "url/gurl.h"

namespace app_list {

namespace {

const char kKeyResults[] = "results";
const char kKeyId[] = "id";
const char kKeyLocalizedName[] = "localized_name";
const char kKeyIconUrl[] = "icon_url";
const char kKeyIsPaid[] = "is_paid";
const char kKeyItemType[] = "item_type";

const char kPlatformAppType[] = "platform_app";
const char kHostedAppType[] = "hosted_app";
const char kLegacyPackagedAppType[] = "legacy_packaged_app";

// Converts the item type string from the web store to an
// extensions::Manifest::Type.
extensions::Manifest::Type ParseItemType(const std::string& item_type_str) {
  if (base::LowerCaseEqualsASCII(item_type_str, kPlatformAppType))
    return extensions::Manifest::TYPE_PLATFORM_APP;

  if (base::LowerCaseEqualsASCII(item_type_str, kLegacyPackagedAppType))
    return extensions::Manifest::TYPE_LEGACY_PACKAGED_APP;

  if (base::LowerCaseEqualsASCII(item_type_str, kHostedAppType))
    return extensions::Manifest::TYPE_HOSTED_APP;

  return extensions::Manifest::TYPE_UNKNOWN;
}

}  // namespace

WebstoreProvider::WebstoreProvider(Profile* profile,
                                   AppListControllerDelegate* controller)
    : WebserviceSearchProvider(profile),
      controller_(controller),
      query_pending_(false) {
}

WebstoreProvider::~WebstoreProvider() {}

void WebstoreProvider::Start(const base::string16& query) {
  if (webstore_search_)
    webstore_search_->Stop();

  ClearResults();
  if (!IsValidQuery(query)) {
    query_.clear();
    return;
  }

  query_ = base::UTF16ToUTF8(query);
  const CacheResult result = cache_->Get(WebserviceCache::WEBSTORE, query_);
  if (result.second) {
    ProcessWebstoreSearchResults(result.second);
    if (!webstore_search_fetched_callback_.is_null())
      webstore_search_fetched_callback_.Run();
    if (result.first == FRESH)
      return;
  }

  if (!webstore_search_) {
    webstore_search_.reset(new JSONResponseFetcher(
        base::Bind(&WebstoreProvider::OnWebstoreSearchFetched,
                   base::Unretained(this)),
        profile_));
  }

  query_pending_ = true;
  StartThrottledQuery(base::Bind(&WebstoreProvider::StartQuery,
                                 base::Unretained(this)));

  // Add a placeholder result which when clicked will run the user's query in a
  // browser. This placeholder is removed when the search results arrive.
  Add(std::make_unique<SearchWebstoreResult>(profile_, controller_, query_));
}

void WebstoreProvider::StartQuery() {
  // |query_| can be NULL when the query is scheduled but then canceled.
  if (!webstore_search_ || query_.empty())
    return;

  webstore_search_->Start(extension_urls::GetWebstoreJsonSearchUrl(
      query_, g_browser_process->GetApplicationLocale()));
}

void WebstoreProvider::OnWebstoreSearchFetched(
    std::unique_ptr<base::DictionaryValue> json) {
  ProcessWebstoreSearchResults(json.get());
  cache_->Put(WebserviceCache::WEBSTORE, query_, std::move(json));
  query_pending_ = false;

  if (!webstore_search_fetched_callback_.is_null())
    webstore_search_fetched_callback_.Run();
}

void WebstoreProvider::ProcessWebstoreSearchResults(
    const base::DictionaryValue* json) {
  const base::ListValue* result_list = NULL;
  if (!json ||
      !json->GetList(kKeyResults, &result_list) ||
      !result_list ||
      result_list->empty()) {
    return;
  }

  bool first_result = true;
  TokenizedString query(base::UTF8ToUTF16(query_));
  for (base::ListValue::const_iterator it = result_list->begin();
       it != result_list->end();
       ++it) {
    const base::DictionaryValue* dict;
    if (!it->GetAsDictionary(&dict))
      continue;

    std::unique_ptr<ChromeSearchResult> result(CreateResult(query, *dict));
    if (!result)
      continue;

    if (first_result) {
      // Clears "search in webstore" place holder results.
      ClearResults();
      first_result = false;
    }

    Add(std::move(result));
  }
}

std::unique_ptr<ChromeSearchResult> WebstoreProvider::CreateResult(
    const TokenizedString& query,
    const base::DictionaryValue& dict) {
  std::string app_id;
  std::string localized_name;
  std::string icon_url_string;
  bool is_paid = false;
  if (!dict.GetString(kKeyId, &app_id) ||
      !dict.GetString(kKeyLocalizedName, &localized_name) ||
      !dict.GetString(kKeyIconUrl, &icon_url_string) ||
      !dict.GetBoolean(kKeyIsPaid, &is_paid)) {
    return nullptr;
  }

  // If an app is already installed, don't show it in results.
  if (controller_->IsExtensionInstalled(profile_, app_id))
    return nullptr;

  GURL icon_url(icon_url_string);
  if (!icon_url.is_valid())
    return nullptr;

  std::string item_type_string;
  dict.GetString(kKeyItemType, &item_type_string);
  extensions::Manifest::Type item_type = ParseItemType(item_type_string);

  // Calculate the relevance score by matching the query with the title. Results
  // with a match score of 0 are discarded. This will also be used to set the
  // title tags (highlighting which parts of the title matched the search
  // query).
  TokenizedString title(base::UTF8ToUTF16(localized_name));
  TokenizedStringMatch match;
  if (!match.Calculate(query, title))
    return nullptr;

  std::unique_ptr<ChromeSearchResult> result = std::make_unique<WebstoreResult>(
      profile_, app_id, icon_url, is_paid, item_type, controller_);
  result->UpdateFromMatch(title, match);
  return result;
}

}  // namespace app_list
