// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/remote/json_request.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/json/json_writer.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/time/clock.h"
#include "base/values.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/ntp_snippets/category_info.h"
#include "components/ntp_snippets/features.h"
#include "components/ntp_snippets/remote/request_params.h"
#include "components/ntp_snippets/user_classifier.h"
#include "components/strings/grit/components_strings.h"
#include "components/variations/net/variations_http_headers.h"
#include "components/variations/variations_associated_data.h"
#include "net/base/load_flags.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "third_party/icu/source/common/unicode/uloc.h"
#include "third_party/icu/source/common/unicode/utypes.h"
#include "ui/base/l10n/l10n_util.h"

using language::UrlLanguageHistogram;
using net::URLFetcher;
using net::URLRequestContextGetter;
using net::HttpRequestHeaders;
using net::URLRequestStatus;

namespace ntp_snippets {

namespace internal {

namespace {

// Variation parameter for disabling the retry.
const char kBackground5xxRetriesName[] = "background_5xx_retries_count";

// Variation parameter for sending UrlLanguageHistogram info to the server.
const char kSendTopLanguagesName[] = "send_top_languages";

// Variation parameter for sending UserClassifier info to the server.
const char kSendUserClassName[] = "send_user_class";

int Get5xxRetryCount(bool interactive_request) {
  if (interactive_request) {
    return 2;
  }
  return std::max(0, variations::GetVariationParamByFeatureAsInt(
                         ntp_snippets::kArticleSuggestionsFeature,
                         kBackground5xxRetriesName, 0));
}

bool IsSendingTopLanguagesEnabled() {
  return variations::GetVariationParamByFeatureAsBool(
      ntp_snippets::kArticleSuggestionsFeature, kSendTopLanguagesName,
      /*default_value=*/true);
}

bool IsSendingUserClassEnabled() {
  return variations::GetVariationParamByFeatureAsBool(
      ntp_snippets::kArticleSuggestionsFeature, kSendUserClassName,
      /*default_value=*/true);
}

// Translate the BCP 47 |language_code| into a posix locale string.
std::string PosixLocaleFromBCP47Language(const std::string& language_code) {
  char locale[ULOC_FULLNAME_CAPACITY];
  UErrorCode error = U_ZERO_ERROR;
  // Translate the input to a posix locale.
  uloc_forLanguageTag(language_code.c_str(), locale, ULOC_FULLNAME_CAPACITY,
                      nullptr, &error);
  if (error != U_ZERO_ERROR) {
    DLOG(WARNING) << "Error in translating language code to a locale string: "
                  << error;
    return std::string();
  }
  return locale;
}

std::string ISO639FromPosixLocale(const std::string& locale) {
  char language[ULOC_LANG_CAPACITY];
  UErrorCode error = U_ZERO_ERROR;
  uloc_getLanguage(locale.c_str(), language, ULOC_LANG_CAPACITY, &error);
  if (error != U_ZERO_ERROR) {
    DLOG(WARNING)
        << "Error in translating locale string to a ISO639 language code: "
        << error;
    return std::string();
  }
  return language;
}

void AppendLanguageInfoToList(base::ListValue* list,
                              const UrlLanguageHistogram::LanguageInfo& info) {
  auto lang = std::make_unique<base::DictionaryValue>();
  lang->SetString("language", info.language_code);
  lang->SetDouble("frequency", info.frequency);
  list->Append(std::move(lang));
}

std::string GetUserClassString(UserClassifier::UserClass user_class) {
  switch (user_class) {
    case UserClassifier::UserClass::RARE_NTP_USER:
      return "RARE_NTP_USER";
    case UserClassifier::UserClass::ACTIVE_NTP_USER:
      return "ACTIVE_NTP_USER";
    case UserClassifier::UserClass::ACTIVE_SUGGESTIONS_CONSUMER:
      return "ACTIVE_SUGGESTIONS_CONSUMER";
  }
  NOTREACHED();
  return std::string();
}

}  // namespace

JsonRequest::JsonRequest(
    base::Optional<Category> exclusive_category,
    base::Clock* clock,  // Needed until destruction of the request.
    const ParseJSONCallback& callback)
    : exclusive_category_(exclusive_category),
      clock_(clock),
      parse_json_callback_(callback),
      weak_ptr_factory_(this) {
  creation_time_ = clock_->Now();
}

JsonRequest::~JsonRequest() {
  LOG_IF(DFATAL, !request_completed_callback_.is_null())
      << "The CompletionCallback was never called!";
}

void JsonRequest::Start(CompletedCallback callback) {
  request_completed_callback_ = std::move(callback);
  url_fetcher_->Start();
}

base::TimeDelta JsonRequest::GetFetchDuration() const {
  return clock_->Now() - creation_time_;
}

std::string JsonRequest::GetResponseString() const {
  std::string response;
  url_fetcher_->GetResponseAsString(&response);
  return response;
}

////////////////////////////////////////////////////////////////////////////////
// URLFetcherDelegate overrides
void JsonRequest::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(url_fetcher_.get(), source);
  const URLRequestStatus& status = url_fetcher_->GetStatus();
  int response = url_fetcher_->GetResponseCode();
  base::UmaHistogramSparse("NewTabPage.Snippets.FetchHttpResponseOrErrorCode",
                           status.is_success() ? response : status.error());

  if (!status.is_success()) {
    std::move(request_completed_callback_)
        .Run(/*result=*/nullptr, FetchResult::URL_REQUEST_STATUS_ERROR,
             /*error_details=*/base::StringPrintf(" %d", status.error()));
  } else if (response != net::HTTP_OK) {
    // TODO(jkrcal): https://crbug.com/609084
    // We need to deal with the edge case again where the auth
    // token expires just before we send the request (in which case we need to
    // fetch a new auth token). We should extract that into a common class
    // instead of adding it to every single class that uses auth tokens.
    std::move(request_completed_callback_)
        .Run(/*result=*/nullptr, FetchResult::HTTP_ERROR,
             /*error_details=*/base::StringPrintf(" %d", response));
  } else {
    ParseJsonResponse();
  }
}

void JsonRequest::ParseJsonResponse() {
  std::string json_string;
  bool stores_result_to_string =
      url_fetcher_->GetResponseAsString(&json_string);
  DCHECK(stores_result_to_string);

  parse_json_callback_.Run(
      json_string,
      base::Bind(&JsonRequest::OnJsonParsed, weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&JsonRequest::OnJsonError, weak_ptr_factory_.GetWeakPtr()));
}

void JsonRequest::OnJsonParsed(std::unique_ptr<base::Value> result) {
  std::move(request_completed_callback_)
      .Run(std::move(result), FetchResult::SUCCESS,
           /*error_details=*/std::string());
}

void JsonRequest::OnJsonError(const std::string& error) {
  std::string json_string;
  url_fetcher_->GetResponseAsString(&json_string);
  LOG(WARNING) << "Received invalid JSON (" << error << "): " << json_string;
  std::move(request_completed_callback_)
      .Run(/*result=*/nullptr, FetchResult::JSON_PARSE_ERROR,
           /*error_details=*/base::StringPrintf(" (error %s)", error.c_str()));
}

JsonRequest::Builder::Builder() : language_histogram_(nullptr) {}
JsonRequest::Builder::Builder(JsonRequest::Builder&&) = default;
JsonRequest::Builder::~Builder() = default;

std::unique_ptr<JsonRequest> JsonRequest::Builder::Build() const {
  DCHECK(!url_.is_empty());
  DCHECK(url_request_context_getter_);
  DCHECK(clock_);
  auto request = std::make_unique<JsonRequest>(params_.exclusive_category,
                                               clock_, parse_json_callback_);
  std::string body = BuildBody();
  std::string headers = BuildHeaders();
  request->url_fetcher_ = BuildURLFetcher(request.get(), headers, body);

  // Log the request for debugging network issues.
  VLOG(1) << "Sending a NTP snippets request to " << url_ << ":\n"
          << headers << "\n"
          << body;

  return request;
}

JsonRequest::Builder& JsonRequest::Builder::SetAuthentication(
    const std::string& account_id,
    const std::string& auth_header) {
  obfuscated_gaia_id_ = account_id;
  auth_header_ = auth_header;
  return *this;
}

JsonRequest::Builder& JsonRequest::Builder::SetLanguageHistogram(
    const language::UrlLanguageHistogram* language_histogram) {
  language_histogram_ = language_histogram;
  return *this;
}

JsonRequest::Builder& JsonRequest::Builder::SetParams(
    const RequestParams& params) {
  params_ = params;
  return *this;
}

JsonRequest::Builder& JsonRequest::Builder::SetParseJsonCallback(
    ParseJSONCallback callback) {
  parse_json_callback_ = callback;
  return *this;
}

JsonRequest::Builder& JsonRequest::Builder::SetClock(base::Clock* clock) {
  clock_ = clock;
  return *this;
}

JsonRequest::Builder& JsonRequest::Builder::SetUrl(const GURL& url) {
  url_ = url;
  return *this;
}

JsonRequest::Builder& JsonRequest::Builder::SetUrlRequestContextGetter(
    const scoped_refptr<net::URLRequestContextGetter>& context_getter) {
  url_request_context_getter_ = context_getter;
  return *this;
}

JsonRequest::Builder& JsonRequest::Builder::SetUserClassifier(
    const UserClassifier& user_classifier) {
  if (IsSendingUserClassEnabled()) {
    user_class_ = GetUserClassString(user_classifier.GetUserClass());
  }
  return *this;
}

std::string JsonRequest::Builder::BuildHeaders() const {
  net::HttpRequestHeaders headers;
  headers.SetHeader("Content-Type", "application/json; charset=UTF-8");
  if (!auth_header_.empty()) {
    headers.SetHeader("Authorization", auth_header_);
  }
  // Add X-Client-Data header with experiment IDs from field trials.
  // Note: It's OK to pass SignedIn::kNo if it's unknown, as it does not affect
  // transmission of experiments coming from the variations server.
  variations::AppendVariationHeaders(url_, variations::InIncognito::kNo,
                                     variations::SignedIn::kNo, &headers);
  return headers.ToString();
}

std::string JsonRequest::Builder::BuildBody() const {
  auto request = std::make_unique<base::DictionaryValue>();
  std::string user_locale = PosixLocaleFromBCP47Language(params_.language_code);
  if (!user_locale.empty()) {
    request->SetString("uiLanguage", user_locale);
  }

  request->SetString("priority", params_.interactive_request
                                     ? "USER_ACTION"
                                     : "BACKGROUND_PREFETCH");

  auto excluded = std::make_unique<base::ListValue>();
  for (const auto& id : params_.excluded_ids) {
    excluded->AppendString(id);
  }
  request->Set("excludedSuggestionIds", std::move(excluded));

  if (!user_class_.empty()) {
    request->SetString("userActivenessClass", user_class_);
  }

  language::UrlLanguageHistogram::LanguageInfo ui_language;
  language::UrlLanguageHistogram::LanguageInfo other_top_language;
  PrepareLanguages(&ui_language, &other_top_language);
  if (ui_language.frequency != 0 || other_top_language.frequency != 0) {
    auto language_list = std::make_unique<base::ListValue>();
    if (ui_language.frequency > 0) {
      AppendLanguageInfoToList(language_list.get(), ui_language);
    }
    if (other_top_language.frequency > 0) {
      AppendLanguageInfoToList(language_list.get(), other_top_language);
    }
    request->Set("topLanguages", std::move(language_list));
  }

  // TODO(vitaliii): Support count_to_fetch without requiring
  // |exclusive_category|.
  if (params_.exclusive_category.has_value()) {
    base::DictionaryValue exclusive_category_parameters;
    exclusive_category_parameters.SetInteger(
        "id", params_.exclusive_category->remote_id());
    exclusive_category_parameters.SetInteger("numSuggestions",
                                             params_.count_to_fetch);
    base::ListValue category_parameters;
    category_parameters.GetList().push_back(
        std::move(exclusive_category_parameters));
    request->SetKey("categoryParameters", std::move(category_parameters));
  }

  std::string request_json;
  bool success = base::JSONWriter::WriteWithOptions(
      *request, base::JSONWriter::OPTIONS_PRETTY_PRINT, &request_json);
  DCHECK(success);
  return request_json;
}

std::unique_ptr<net::URLFetcher> JsonRequest::Builder::BuildURLFetcher(
    net::URLFetcherDelegate* delegate,
    const std::string& headers,
    const std::string& body) const {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("ntp_snippets_fetch", R"(
        semantics {
          sender: "New Tab Page Content Suggestions Fetch"
          description:
            "Chromium can show content suggestions (e.g. news articles) on the "
            "New Tab page. For signed-in users, these may be personalized "
            "based on the user's synced browsing history."
          trigger:
            "Triggered periodically in the background, or upon explicit user "
            "request."
          data:
            "The Chromium UI language, as well as a second language the user "
            "understands, based on language::UrlLanguageHistogram. For "
            "signed-in users, the requests is authenticated."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings now (but is requested "
            "to be implemented in crbug.com/695129)."
          chrome_policy {
            NTPContentSuggestionsEnabled {
              policy_options {mode: MANDATORY}
              NTPContentSuggestionsEnabled: false
            }
          }
        })");
  std::unique_ptr<net::URLFetcher> url_fetcher = net::URLFetcher::Create(
      url_, net::URLFetcher::POST, delegate, traffic_annotation);
  url_fetcher->SetRequestContext(url_request_context_getter_.get());
  url_fetcher->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                            net::LOAD_DO_NOT_SAVE_COOKIES);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      url_fetcher.get(),
      data_use_measurement::DataUseUserData::NTP_SNIPPETS_SUGGESTIONS);

  url_fetcher->SetExtraRequestHeaders(headers);
  url_fetcher->SetUploadData("application/json", body);

  // Fetchers are sometimes cancelled because a network change was detected.
  url_fetcher->SetAutomaticallyRetryOnNetworkChanges(3);
  url_fetcher->SetMaxRetriesOn5xx(
      Get5xxRetryCount(params_.interactive_request));
  return url_fetcher;
}

void JsonRequest::Builder::PrepareLanguages(
    language::UrlLanguageHistogram::LanguageInfo* ui_language,
    language::UrlLanguageHistogram::LanguageInfo* other_top_language) const {
  // TODO(jkrcal): Add language model factory for iOS and add fakes to tests so
  // that |language_histogram| is never nullptr. Remove this check and add a
  // DCHECK into the constructor.
  if (!language_histogram_ || !IsSendingTopLanguagesEnabled()) {
    return;
  }

  // TODO(jkrcal): Is this back-and-forth converting necessary?
  ui_language->language_code = ISO639FromPosixLocale(
      PosixLocaleFromBCP47Language(params_.language_code));
  ui_language->frequency =
      language_histogram_->GetLanguageFrequency(ui_language->language_code);

  std::vector<UrlLanguageHistogram::LanguageInfo> top_languages =
      language_histogram_->GetTopLanguages();
  for (const UrlLanguageHistogram::LanguageInfo& info : top_languages) {
    if (info.language_code != ui_language->language_code) {
      *other_top_language = info;

      // Report to UMA how important the UI language is.
      DCHECK_GT(other_top_language->frequency, 0)
          << "GetTopLanguages() should not return languages with 0 frequency";
      float ratio_ui_in_both_languages =
          ui_language->frequency /
          (ui_language->frequency + other_top_language->frequency);
      UMA_HISTOGRAM_PERCENTAGE(
          "NewTabPage.Languages.UILanguageRatioInTwoTopLanguages",
          ratio_ui_in_both_languages * 100);
      break;
    }
  }
}

}  // namespace internal

}  // namespace ntp_snippets
