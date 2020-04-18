// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/contextual_search/contextual_search_delegate.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/json/json_string_value_serializer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/google/core/browser/google_util.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/util.h"
#include "components/variations/net/variations_http_headers.h"
#include "components/variations/variations_associated_data.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/search_engines/template_url_service_factory.h"
#include "ios/chrome/browser/ui/contextual_search/protos/client_discourse_context.pb.h"
#include "ios/web/public/web_thread.h"
#include "net/base/escape.h"
#include "net/base/url_util.h"
#include "net/url_request/url_fetcher.h"
#include "url/gurl.h"

namespace {

const char kContextualSearchFieldTrialName[] = "ContextualSearch";
const char kContextualSearchPreventPreload[] = "prevent_preload";
const char kContextualSearchResolverUrl[] = "contextual-search-resolver-url";
const char kContextualSearchResolverURLParamName[] = "resolver_url";
const char kContextualSearchResponseDisplayTextParam[] = "display_text";
const char kContextualSearchResponseMentionsParam[] = "mentions";
const char kContextualSearchResponseResolvedTermParam[] = "resolved_term";
const char kContextualSearchResponseSelectedTextParam[] = "selected_text";
const char kContextualSearchResponseSearchTermParam[] = "search_term";
const int kContextualSearchRequestVersion = 2;
const char kContextualSearchServerEndpoint[] = "_/contextualsearch?";
const char kDiscourseContextHeaderPrefix[] = "X-Additional-Discourse-Context: ";
const char kDoPreventPreloadValue[] = "1";
const char kXssiEscape[] = ")]}'\n";

// The version of the Contextual Cards API that we want to invoke.
const int kContextualCardsNoIntegration = 0;

const double kMinimumDelayBetweenRequestSeconds = 1;

// Decodes the given response from the search term resolution request and sets
// the value of the given search-term and display_text parameters.
void DecodeSearchTermsFromJsonResponse(const std::string& response,
                                       std::string* search_term,
                                       std::string* display_text,
                                       std::string* alternate_term,
                                       std::string* prevent_preload,
                                       int& start_offset,
                                       int& end_offset) {
  bool contains_xssi_escape = response.find(kXssiEscape) == 0;
  const std::string& proper_json =
      contains_xssi_escape ? response.substr(strlen(kXssiEscape)) : response;
  JSONStringValueDeserializer deserializer(proper_json);
  std::unique_ptr<base::Value> root(deserializer.Deserialize(NULL, NULL));

  if (root.get() != NULL && root->is_dict()) {
    base::DictionaryValue* dict =
        static_cast<base::DictionaryValue*>(root.get());
    dict->GetString(kContextualSearchPreventPreload, prevent_preload);
    dict->GetString(kContextualSearchResponseSearchTermParam, search_term);
    // For the display_text, if not present fall back to the "search_term".
    if (!dict->GetString(kContextualSearchResponseDisplayTextParam,
                         display_text)) {
      *display_text = *search_term;
    }
    // If either the selected text or the resolved term is not the search term,
    // use it as the alternate term.
    std::string selected_text;
    dict->GetString(kContextualSearchResponseSelectedTextParam, &selected_text);

    const base::ListValue* mentionsList;
    if (dict->GetList(kContextualSearchResponseMentionsParam, &mentionsList)) {
      DCHECK(mentionsList->GetSize() == 2);
      mentionsList->GetInteger(0, &start_offset);
      mentionsList->GetInteger(1, &end_offset);
    }

    if (selected_text != *search_term) {
      *alternate_term = selected_text;
    } else {
      std::string resolved_term;
      dict->GetString(kContextualSearchResponseResolvedTermParam,
                      &resolved_term);
      if (resolved_term != *search_term) {
        *alternate_term = resolved_term;
      }
    }
  }
}

}  // namespace

// URLFetcher ID, only used for tests: we only have one kind of fetcher.
const int ContextualSearchDelegate::kContextualSearchURLFetcherID = 1;

// Handles tasks for the ContextualSearchManager in a separable, testable way.
ContextualSearchDelegate::ContextualSearchDelegate(
    ios::ChromeBrowserState* browser_state,
    const ContextualSearchDelegate::SearchTermResolutionCallback&
        search_term_callback)
    : template_url_service_(
          ios::TemplateURLServiceFactory::GetForBrowserState(browser_state)),
      browser_state_(browser_state),
      search_term_callback_(search_term_callback),
      weak_ptr_factory_(this) {}

ContextualSearchDelegate::~ContextualSearchDelegate() {}

void ContextualSearchDelegate::PostSearchTermRequest(
    std::shared_ptr<ContextualSearchContext> context) {
  context_ = context;
  if (request_pending_) {
    return;
  }
  request_pending_ = true;

  base::TimeDelta interval =
      base::TimeDelta::FromSecondsD(kMinimumDelayBetweenRequestSeconds);
  base::Time now = base::Time::Now();
  if (now > last_request_startup_time_ + interval) {
    StartPendingSearchTermRequest();
  } else {
    base::TimeDelta delay = last_request_startup_time_ + interval - now;
    web::WebThread::PostDelayedTask(
        web::WebThread::UI, FROM_HERE,
        base::Bind(&ContextualSearchDelegate::StartPendingSearchTermRequest,
                   weak_ptr_factory_.GetWeakPtr()),
        delay);
  }
}

void ContextualSearchDelegate::StartPendingSearchTermRequest() {
  if (!request_pending_ || !context_)
    return;
  request_pending_ = false;
  last_request_startup_time_ = base::Time::Now();
  if (context_->HasSurroundingText()) {
    RequestServerSearchTerm();
  } else {
    RequestLocalSearchTerm();
  }
}

void ContextualSearchDelegate::RequestLocalSearchTerm() {
  SearchResolution resolution;
  resolution.is_invalid = false;
  resolution.response_code = 200;  // HTTP success.
  resolution.search_term = context_->selected_text;
  resolution.display_text = context_->selected_text;
  resolution.alternate_term = context_->selected_text;
  resolution.prevent_preload = false;
  resolution.start_offset = -1;
  resolution.end_offset = -1;
  search_term_callback_.Run(resolution);
}

void ContextualSearchDelegate::RequestServerSearchTerm() {
  GURL request_url(BuildRequestUrl());
  DCHECK(request_url.is_valid());

  // Reset will delete any previous fetcher, and we won't get any callback.
  search_term_fetcher_ = net::URLFetcher::Create(
      kContextualSearchURLFetcherID, request_url, net::URLFetcher::GET, this);
  search_term_fetcher_->SetRequestContext(browser_state_->GetRequestContext());

  // Add Chrome experiment state to the request headers.
  net::HttpRequestHeaders headers;
  // Note: It's OK to pass SignedIn::kNo if it's unknown, as it does not affect
  // transmission of experiments coming from the variations server.
  variations::AppendVariationHeaders(search_term_fetcher_->GetOriginalURL(),
                                     browser_state_->IsOffTheRecord()
                                         ? variations::InIncognito::kYes
                                         : variations::InIncognito::kNo,
                                     variations::SignedIn::kNo, &headers);
  search_term_fetcher_->SetExtraRequestHeaders(headers.ToString());

  SetDiscourseContextAndAddToHeader(*context_);

  search_term_fetcher_->Start();
}

void ContextualSearchDelegate::CancelSearchTermRequest() {
  search_term_fetcher_.reset();
  context_.reset();
}

// Adapted from /chrome/browser/search_engines/template_url_service_android.cc
GURL ContextualSearchDelegate::GetURLForResolvedSearch(
    SearchResolution resolution,
    bool should_prefetch) {
  GURL url;
  if (!resolution.search_term.empty()) {
    url = GetDefaultSearchURLForSearchTerms(
        template_url_service_, base::UTF8ToUTF16(resolution.search_term));
    if (google_util::IsGoogleSearchUrl(url)) {
      url = net::AppendQueryParameter(url, "ctxs", "2");
      if (should_prefetch) {
        // Indicate that the search page is being prefetched.
        url = net::AppendQueryParameter(url, "pf", "c");
      }

      if (!resolution.alternate_term.empty()) {
        url = net::AppendQueryParameter(url, "ctxsl_alternate_term",
                                        resolution.alternate_term);
      }
    }
  }
  return url;
}

void ContextualSearchDelegate::OnURLFetchComplete(
    const net::URLFetcher* source) {
  DCHECK(source == search_term_fetcher_.get());
  SearchResolution resolution;
  std::string prevent_preload;
  resolution.response_code = source->GetResponseCode();
  if (source->GetStatus().is_success() && resolution.response_code == 200) {
    std::string response;
    bool has_string_response = source->GetResponseAsString(&response);
    DCHECK(has_string_response);
    if (has_string_response) {
      resolution.start_offset = -1;
      resolution.end_offset = -1;
      DecodeSearchTermsFromJsonResponse(
          response, &resolution.search_term, &resolution.display_text,
          &resolution.alternate_term, &prevent_preload, resolution.start_offset,
          resolution.end_offset);
    }
  }
  resolution.is_invalid =
      resolution.response_code == net::URLFetcher::RESPONSE_CODE_INVALID;
  resolution.prevent_preload = prevent_preload == kDoPreventPreloadValue;

  search_term_callback_.Run(resolution);
}

// TODO(donnd): use HTTP headers for the context instead of CGI params in GET.
// See https://code.google.com/p/chromium/issues/detail?id=341762
GURL ContextualSearchDelegate::BuildRequestUrl() {
  // TODO(jeremycho): Confirm this is the right way to handle TemplateURL fails.
  if (!template_url_service_ ||
      !template_url_service_->GetDefaultSearchProvider()) {
    return GURL();
  }

  std::string selected_text_escaped(
      net::EscapeQueryParamValue(context_->selected_text, true));
  std::string base_page_url = context_->page_url.spec();

  std::string request =
      GetSearchTermResolutionUrlString(selected_text_escaped, base_page_url);

  return GURL(request);
}

std::string ContextualSearchDelegate::GetSearchTermResolutionUrlString(
    const std::string& selected_text,
    const std::string& base_page_url) {
  const TemplateURL* template_url =
      template_url_service_->GetDefaultSearchProvider();

  TemplateURLRef::SearchTermsArgs search_terms_args =
      TemplateURLRef::SearchTermsArgs(base::string16());

  TemplateURLRef::SearchTermsArgs::ContextualSearchParams params(
      kContextualSearchRequestVersion, kContextualCardsNoIntegration,
      std::string());

  search_terms_args.contextual_search_params = params;

  std::string request(
      template_url->contextual_search_url_ref().ReplaceSearchTerms(
          search_terms_args, template_url_service_->search_terms_data(), NULL));

  // The switch/param should be the URL up to and including the endpoint.
  std::string replacement_url;
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          kContextualSearchResolverUrl)) {
    replacement_url =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            kContextualSearchResolverUrl);
  } else {
    std::string param_value = variations::GetVariationParamValue(
        kContextualSearchFieldTrialName, kContextualSearchResolverURLParamName);
    if (!param_value.empty())
      replacement_url = param_value;
  }

  // If a replacement URL was specified above, do the substitution.
  if (!replacement_url.empty()) {
    size_t pos = request.find(kContextualSearchServerEndpoint);
    if (pos != std::string::npos) {
      request.replace(0, pos + strlen(kContextualSearchServerEndpoint),
                      replacement_url);
    }
  }
  return request;
}

void ContextualSearchDelegate::SetDiscourseContextAndAddToHeader(
    const ContextualSearchContext& context) {
  discourse_context::ClientDiscourseContext proto;
  discourse_context::Display* display = proto.add_display();
  display->set_uri(context.page_url.spec());

  discourse_context::Media* media = display->mutable_media();
  media->set_mime_type(context.encoding);

  discourse_context::Selection* selection = display->mutable_selection();
  selection->set_content(net::EscapeQueryParamValue(
      base::UTF16ToUTF8(context.surrounding_text), true));
  selection->set_start(context.start_offset);
  selection->set_end(context.end_offset);

  std::string serialized;
  proto.SerializeToString(&serialized);

  std::string encoded_context;
  base::Base64Encode(serialized, &encoded_context);
  // The server memoizer expects a web-safe encoding.
  std::replace(encoded_context.begin(), encoded_context.end(), '+', '-');
  std::replace(encoded_context.begin(), encoded_context.end(), '/', '_');
  search_term_fetcher_->AddExtraRequestHeader(kDiscourseContextHeaderPrefix +
                                              encoded_context);
}
