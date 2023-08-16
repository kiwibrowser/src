// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/search_provider.h"

#include <stddef.h>
#include <algorithm>
#include <cmath>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/feature_list.h"
#include "base/i18n/break_iterator.h"
#include "base/i18n/case_conversion.h"
#include "base/json/json_string_value_serializer.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/rand_util.h"
#include "base/android/sys_utils.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "components/google/core/browser/google_pref_names.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/history/core/browser/in_memory_database.h"
#include "components/history/core/browser/keyword_search_term.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/keyword_provider.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/omnibox/browser/suggestion_answer.h"
#include "components/omnibox/browser/url_prefix.h"
#include "components/search/search.h"
#include "components/search_engines/template_url_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/url_formatter/url_formatter.h"
#include "components/variations/net/variations_http_headers.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"
#include "third_party/metrics_proto/omnibox_input_type.pb.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/url_constants.h"
#include "url/url_util.h"

// Helpers --------------------------------------------------------------------

namespace {

// We keep track in a histogram how many suggest requests we send, how
// many suggest requests we invalidate (e.g., due to a user typing
// another character), and how many replies we receive.
// *** ADD NEW ENUMS AFTER ALL PREVIOUSLY DEFINED ONES! ***
//     (excluding the end-of-list enum value)
// We do not want values of existing enums to change or else it screws
// up the statistics.
enum SuggestRequestsHistogramValue {
  REQUEST_SENT = 1,
  REQUEST_INVALIDATED,
  REPLY_RECEIVED,
  MAX_SUGGEST_REQUEST_HISTOGRAM_VALUE
};

// The verbatim score for an input which is not an URL.
const int kNonURLVerbatimRelevance = 1300;

// Increments the appropriate value in the histogram by one.
void LogOmniboxSuggestRequest(
    SuggestRequestsHistogramValue request_value) {
  UMA_HISTOGRAM_ENUMERATION("Omnibox.SuggestRequests", request_value,
                            MAX_SUGGEST_REQUEST_HISTOGRAM_VALUE);
}

bool HasMultipleWords(const base::string16& text) {
  base::i18n::BreakIterator i(text, base::i18n::BreakIterator::BREAK_WORD);
  bool found_word = false;
  if (i.Init()) {
    while (i.Advance()) {
      if (i.IsWord()) {
        if (found_word)
          return true;
        found_word = true;
      }
    }
  }
  return false;
}

}  // namespace

// SearchProvider::Providers --------------------------------------------------

SearchProvider::Providers::Providers(TemplateURLService* template_url_service)
    : template_url_service_(template_url_service) {}

const TemplateURL* SearchProvider::Providers::GetDefaultProviderURL() const {
  if (default_provider_.empty())
    return nullptr;
  DCHECK(template_url_service_);
  return template_url_service_->GetTemplateURLForKeyword(default_provider_);
}

const TemplateURL* SearchProvider::Providers::GetBangsProviderURL() const {
  DCHECK(template_url_service_);
  return template_url_service_->FindPrepopulatedTemplateURL(33);
}

const TemplateURL* SearchProvider::Providers::GetKeywordProviderURL() const {
  if (keyword_provider_.empty())
    return nullptr;
  DCHECK(template_url_service_);
  return template_url_service_->GetTemplateURLForKeyword(keyword_provider_);
}


// SearchProvider::CompareScoredResults ---------------------------------------

class SearchProvider::CompareScoredResults {
 public:
  bool operator()(const SearchSuggestionParser::Result& a,
                  const SearchSuggestionParser::Result& b) {
    // Sort in descending relevance order.
    return a.relevance() > b.relevance();
  }
};


// SearchProvider -------------------------------------------------------------

SearchProvider::SearchProvider(AutocompleteProviderClient* client,
                               AutocompleteProviderListener* listener)
    : BaseSearchProvider(AutocompleteProvider::TYPE_SEARCH, client),
      listener_(listener),
      providers_(client->GetTemplateURLService()),
      answers_cache_(10) {
  TemplateURLService* template_url_service = client->GetTemplateURLService();

  // |template_url_service| can be null in tests.
  if (template_url_service)
    template_url_service->AddObserver(this);
}

// static
std::string SearchProvider::GetSuggestMetadata(const AutocompleteMatch& match) {
  return match.GetAdditionalInfo(kSuggestMetadataKey);
}

void SearchProvider::RegisterDisplayedAnswers(
    const AutocompleteResult& result) {
  if (result.empty())
    return;

  // The answer must be in the first or second slot to be considered. It should
  // only be in the second slot if AutocompleteController ranked a local search
  // history or a verbatim item higher than the answer.
  AutocompleteResult::const_iterator match = result.begin();
  if (match->answer_contents.empty() && result.size() > 1)
    ++match;
  if (match->answer_contents.empty() || match->answer_type.empty() ||
      match->fill_into_edit.empty())
    return;

  // Valid answer encountered, cache it for further queries.
  answers_cache_.UpdateRecentAnswers(match->fill_into_edit, match->answer_type);
}

// static
int SearchProvider::CalculateRelevanceForKeywordVerbatim(
    metrics::OmniboxInputType type,
    bool allow_exact_keyword_match,
    bool prefer_keyword) {
  // This function is responsible for scoring verbatim query matches
  // for non-extension substituting keywords.
  // KeywordProvider::CalculateRelevance() scores all other types of
  // keyword verbatim matches.
  if (allow_exact_keyword_match && prefer_keyword)
    return 1500;
  return (allow_exact_keyword_match &&
          (type == metrics::OmniboxInputType::QUERY))
             ? 1450
             : 1100;
}

void SearchProvider::ResetSession() {
  set_field_trial_triggered(false);
  set_field_trial_triggered_in_session(false);
}

SearchProvider::~SearchProvider() {
  TemplateURLService* template_url_service = client()->GetTemplateURLService();
  if (template_url_service)
    template_url_service->RemoveObserver(this);
}

// static
void SearchProvider::UpdateOldResults(
    bool minimal_changes,
    SearchSuggestionParser::Results* results) {
  // When called without |minimal_changes|, it likely means the user has
  // pressed a key.  Revise the cached results appropriately.
  if (!minimal_changes) {
    for (SearchSuggestionParser::SuggestResults::iterator sug_it =
             results->suggest_results.begin();
         sug_it != results->suggest_results.end(); ) {
      if (sug_it->type() == AutocompleteMatchType::CALCULATOR) {
        sug_it = results->suggest_results.erase(sug_it);
      } else {
        sug_it->set_received_after_last_keystroke(false);
        ++sug_it;
      }
    }
    for (SearchSuggestionParser::NavigationResults::iterator nav_it =
             results->navigation_results.begin();
         nav_it != results->navigation_results.end(); ++nav_it) {
      nav_it->set_received_after_last_keystroke(false);
    }
  }
}

void SearchProvider::Start(const AutocompleteInput& input,
                           bool minimal_changes) {
  TRACE_EVENT0("omnibox", "SearchProvider::Start");
  // Do our best to load the model as early as possible.  This will reduce
  // odds of having the model not ready when really needed (a non-empty input).
  TemplateURLService* model = client()->GetTemplateURLService();
  DCHECK(model);
  model->Load();

  matches_.clear();
  set_field_trial_triggered(false);

  // Unless warming up the suggest server on focus, SearchProvider doesn't do
  // do anything useful for on-focus inputs or empty inputs.  Exit early.
  if (!base::FeatureList::IsEnabled(omnibox::kSearchProviderWarmUpOnFocus) &&
      (input.from_omnibox_focus() ||
       input.type() == metrics::OmniboxInputType::INVALID)) {
    Stop(true, false);
    return;
  }

  keyword_input_ = input;
  const TemplateURL* keyword_provider =
      KeywordProvider::GetSubstitutingTemplateURLForInput(model,
                                                          &keyword_input_);
  if (keyword_provider == nullptr)
    keyword_input_.Clear();
  else if (keyword_input_.text().empty())
    keyword_provider = nullptr;

  const TemplateURL* default_provider = model->GetDefaultSearchProvider();
  if (default_provider &&
      !default_provider->SupportsReplacement(model->search_terms_data()))
    default_provider = nullptr;

  if (keyword_provider == default_provider)
    default_provider = nullptr;  // No use in querying the same provider twice.

  if (!default_provider && !keyword_provider) {
    // No valid providers.
    Stop(true, false);
    return;
  }

  // If we're still running an old query but have since changed the query text
  // or the providers, abort the query.
  base::string16 default_provider_keyword(default_provider ?
      default_provider->keyword() : base::string16());
  base::string16 keyword_provider_keyword(keyword_provider ?
      keyword_provider->keyword() : base::string16());
  if (!minimal_changes ||
      !providers_.equal(default_provider_keyword, keyword_provider_keyword)) {
    // Cancel any in-flight suggest requests.
    if (!done_)
      Stop(false, false);
  }

  providers_.set(default_provider_keyword, keyword_provider_keyword);

  if (input.from_omnibox_focus()) {
    // Don't display any suggestions for on-focus requests.
    DCHECK(done_);
    ClearAllResults();
  } else if (input.text().empty()) {
    // User typed "?" alone.  Give them a placeholder result indicating what
    // this syntax does.
    if (default_provider) {
      AutocompleteMatch match;
      match.provider = this;
      match.contents.assign(l10n_util::GetStringUTF16(IDS_EMPTY_KEYWORD_VALUE));
      match.contents_class.push_back(
          ACMatchClassification(0, ACMatchClassification::NONE));
      match.keyword = providers_.default_provider();
      match.allowed_to_be_default_match = true;
      matches_.push_back(match);
    }
    Stop(true, false);
    return;
  }

  input_ = input;

  // Don't search the query history database for on-focus inputs; these inputs
  // should only be used to warm up the suggest server.
  if (!input.from_omnibox_focus()) {
    DoHistoryQuery(minimal_changes);
    // Answers needs scored history results before any suggest query has been
    // started, since the query for answer-bearing results needs additional
    // prefetch information based on the highest-scored local history result.
    ScoreHistoryResults(raw_default_history_results_, false,
                        &transformed_default_history_results_);
    ScoreHistoryResults(raw_keyword_history_results_, true,
                        &transformed_keyword_history_results_);
    prefetch_data_ = FindAnswersPrefetchData();

    // Raw results are not needed any more.
    raw_default_history_results_.clear();
    raw_keyword_history_results_.clear();
  }

  StartOrStopSuggestQuery(minimal_changes);
  UpdateMatches();
}

void SearchProvider::Stop(bool clear_cached_results,
                          bool due_to_user_inactivity) {
  StopSuggest();
  done_ = true;

  if (clear_cached_results)
    ClearAllResults();
}

const TemplateURL* SearchProvider::GetTemplateURL(bool is_keyword) const {
  return is_keyword ? providers_.GetKeywordProviderURL()
                    : providers_.GetDefaultProviderURL();
}

const AutocompleteInput SearchProvider::GetInput(bool is_keyword) const {
  return is_keyword ? keyword_input_ : input_;
}

bool SearchProvider::ShouldAppendExtraParams(
    const SearchSuggestionParser::SuggestResult& result) const {
  return !result.from_keyword_provider() ||
      providers_.default_provider().empty();
}

void SearchProvider::RecordDeletionResult(bool success) {
  if (success) {
    base::RecordAction(
        base::UserMetricsAction("Omnibox.ServerSuggestDelete.Success"));
  } else {
    base::RecordAction(
        base::UserMetricsAction("Omnibox.ServerSuggestDelete.Failure"));
  }
}

void SearchProvider::OnTemplateURLServiceChanged() {
  // Only update matches at this time if we haven't already claimed we're done
  // processing the query.
  if (done_)
    return;

  // Check that the engines we're using weren't renamed or deleted.  (In short,
  // require that an engine still exists with the keywords in use.)  For each
  // deleted engine, cancel the in-flight request if any, drop its suggestions,
  // and, in the case when the default provider was affected, point the cached
  // default provider keyword name at the new name for the default provider.

  // Get...ProviderURL() looks up the provider using the cached keyword name
  // stored in |providers_|.
  const TemplateURL* template_url = providers_.GetDefaultProviderURL();
  if (!template_url) {
    CancelFetcher(&default_fetcher_);
    CancelFetcher(&bangs_fetcher_);
    default_results_.Clear();
    bangs_results_.Clear();
    providers_.set(client()
                       ->GetTemplateURLService()
                       ->GetDefaultSearchProvider()
                       ->keyword(),
                   providers_.keyword_provider());
  }
  template_url = providers_.GetKeywordProviderURL();
  if (!providers_.keyword_provider().empty() && !template_url) {
    CancelFetcher(&keyword_fetcher_);
    keyword_results_.Clear();
    providers_.set(providers_.default_provider(), base::string16());
  }
  // It's possible the template URL changed without changing associated keyword.
  // Hence, it's always necessary to update matches to use the new template
  // URL.  (One could cache the template URL and only call UpdateMatches() and
  // OnProviderUpdate() if a keyword was deleted/renamed or the template URL
  // was changed.  That would save extra calls to these functions.  However,
  // this is uncommon and not likely to be worth the extra work.)
  UpdateMatches();
  listener_->OnProviderUpdate(true);  // always pretend something changed
}

void SearchProvider::OnURLFetchComplete(const net::URLFetcher* source) {
  LOG(ERROR) << "[Kiwi] SearchProvider::OnURLFetchComplete";
  TRACE_EVENT0("omnibox", "SearchProvider::OnURLFetchComplete");
  DCHECK(!done_);
  const bool is_keyword = source == keyword_fetcher_.get();
  const bool is_bangs = source == bangs_fetcher_.get();

  // Ensure the request succeeded and that the provider used is still available.
  // A verbatim match cannot be generated without this provider, causing errors.
  const bool request_succeeded =
      source->GetStatus().is_success() && (source->GetResponseCode() == 200) &&
      GetTemplateURL(is_keyword);

  LogFetchComplete(request_succeeded, is_keyword);

  bool results_updated = false;
  // Ignore (i.e., don't display) any suggestions for on-focus inputs.
  // SearchProvider is not intended to give suggestions on on-focus inputs;
  // that's left to ZeroSuggestProvider and friends.  Furthermore, it's not
  // clear if the suggest server will send back sensible results to the
  // request we're constructing here for on-focus inputs.
  if (!input_.from_omnibox_focus() && request_succeeded) {
    LOG(ERROR) << "[Kiwi] SearchProvider::OnURLFetchComplete - Request has succeeded";
    if (is_keyword)
      LOG(ERROR) << "[Kiwi] SearchProvider::OnURLFetchComplete - Request has succeeded (is_keyword)";
    else if (is_bangs)
      LOG(ERROR) << "[Kiwi] SearchProvider::OnURLFetchComplete - Request has succeeded (is_bangs)";
    else
      LOG(ERROR) << "[Kiwi] SearchProvider::OnURLFetchComplete - Request has succeeded (default)";
    std::unique_ptr<base::Value> data(
        SearchSuggestionParser::DeserializeJsonData(
            SearchSuggestionParser::ExtractJsonData(source)));
    if (data) {
      LOG(ERROR) << "[Kiwi] SearchProvider::OnURLFetchComplete - We have data";
      SearchSuggestionParser::Results* results =
          is_keyword ? &keyword_results_ : (is_bangs ? &bangs_results_ : &default_results_);
      results_updated = ParseSuggestResults(*data, -1, is_keyword, results);
      if (results_updated)
        SortResults(is_keyword, results);
    }
  }

  // Delete the fetcher now that we're done with it.
  if (is_keyword)
    keyword_fetcher_.reset();
  else if (is_bangs)
    bangs_fetcher_.reset();
  else
    default_fetcher_.reset();

  // Update matches, done status, etc., and send alerts if necessary.
  UpdateMatches();
  if (done_ || results_updated)
    listener_->OnProviderUpdate(results_updated);
}

void SearchProvider::StopSuggest() {
  CancelFetcher(&default_fetcher_);
  CancelFetcher(&keyword_fetcher_);
  CancelFetcher(&bangs_fetcher_);
  timer_.Stop();
}

void SearchProvider::ClearAllResults() {
  keyword_results_.Clear();
  default_results_.Clear();
  bangs_results_.Clear();
}

void SearchProvider::UpdateMatchContentsClass(
    const base::string16& input_text,
    SearchSuggestionParser::Results* results) {
  for (SearchSuggestionParser::SuggestResults::iterator sug_it =
           results->suggest_results.begin();
       sug_it != results->suggest_results.end(); ++sug_it) {
    sug_it->ClassifyMatchContents(false, input_text);
  }
  for (SearchSuggestionParser::NavigationResults::iterator nav_it =
           results->navigation_results.begin();
       nav_it != results->navigation_results.end(); ++nav_it) {
    nav_it->CalculateAndClassifyMatchContents(false, input_text);
  }
}

void SearchProvider::SortResults(bool is_keyword,
                                 SearchSuggestionParser::Results* results) {
  // Ignore suggested scores for non-keyword matches in keyword mode; if the
  // server is allowed to score these, it could interfere with the user's
  // ability to get good keyword results.
  const bool abandon_suggested_scores =
      !is_keyword && !providers_.keyword_provider().empty();
  // Apply calculated relevance scores to suggestions if valid relevances were
  // not provided or we're abandoning suggested scores entirely.
  if (!results->relevances_from_server || abandon_suggested_scores) {
    ApplyCalculatedSuggestRelevance(&results->suggest_results);
    ApplyCalculatedNavigationRelevance(&results->navigation_results);
    // If abandoning scores entirely, also abandon the verbatim score.
    if (abandon_suggested_scores)
      results->verbatim_relevance = -1;
  }

  // Keep the result lists sorted.
  const CompareScoredResults comparator = CompareScoredResults();
  std::stable_sort(results->suggest_results.begin(),
                   results->suggest_results.end(),
                   comparator);
  std::stable_sort(results->navigation_results.begin(),
                   results->navigation_results.end(),
                   comparator);
}

void SearchProvider::LogFetchComplete(bool success, bool is_keyword) {
  LogOmniboxSuggestRequest(REPLY_RECEIVED);
  // Record response time for suggest requests sent to Google.  We care
  // only about the common case: the Google default provider used in
  // non-keyword mode.
  const TemplateURL* default_url = providers_.GetDefaultProviderURL();
  if (!is_keyword && default_url &&
      (default_url->GetEngineType(
          client()->GetTemplateURLService()->search_terms_data()) ==
       SEARCH_ENGINE_GOOGLE)) {
    const base::TimeDelta elapsed_time =
        base::TimeTicks::Now() - time_suggest_request_sent_;
    if (success) {
      UMA_HISTOGRAM_TIMES("Omnibox.SuggestRequest.Success.GoogleResponseTime",
                          elapsed_time);
    } else {
      UMA_HISTOGRAM_TIMES("Omnibox.SuggestRequest.Failure.GoogleResponseTime",
                          elapsed_time);
    }
  }
}

void SearchProvider::UpdateMatches() {
  // On-focus inputs display no suggestions, so we do not need to persist the
  // previous top suggestions, add new suggestions, or revise suggestions to
  // enforce constraints about inlinability in this case.  Indeed, most of
  // these steps would be bad, as they'd add a suggestion of some form, thus
  // opening the dropdown (which we do not want to happen).
  if (!input_.from_omnibox_focus()) {
    PersistTopSuggestions(&default_results_);
    PersistTopSuggestions(&bangs_results_);
    PersistTopSuggestions(&keyword_results_);
    ConvertResultsToAutocompleteMatches();
    EnforceConstraints();
    UMA_HISTOGRAM_CUSTOM_COUNTS("Omnibox.SearchProviderMatches",
                                matches_.size(), 1, 6, 7);
    RecordTopSuggestion();
  }

  UpdateDone();
}

void SearchProvider::EnforceConstraints() {
  if (!matches_.empty() &&
      (default_results_.HasServerProvidedScores() ||
       keyword_results_.HasServerProvidedScores() ||
       bangs_results_.HasServerProvidedScores())) {
    // These blocks attempt to repair undesirable behavior by suggested
    // relevances with minimal impact, preserving other suggested relevances.
    const TemplateURL* keyword_url = providers_.GetKeywordProviderURL();
    const bool is_extension_keyword =
        (keyword_url != nullptr) &&
        (keyword_url->type() == TemplateURL::OMNIBOX_API_EXTENSION);
    if ((keyword_url != nullptr) && !is_extension_keyword &&
        (AutocompleteResult::FindTopMatch(&matches_) == matches_.end())) {
      // In non-extension keyword mode, disregard the keyword verbatim suggested
      // relevance if necessary, so at least one match is allowed to be default.
      // (In extension keyword mode this is not necessary because the extension
      // will return a default match.)  Give keyword verbatim the lowest
      // non-zero score to best reflect what the server desired.
      DCHECK_EQ(0, keyword_results_.verbatim_relevance);
      keyword_results_.verbatim_relevance = 1;
      ConvertResultsToAutocompleteMatches();
    }
    if (IsTopMatchSearchWithURLInput()) {
      // Disregard the suggested search and verbatim relevances if the input
      // type is URL and the top match is a highly-ranked search suggestion.
      // For example, prevent a search for "foo.com" from outranking another
      // provider's navigation for "foo.com" or "foo.com/url_from_history".
      ApplyCalculatedSuggestRelevance(&keyword_results_.suggest_results);
      ApplyCalculatedSuggestRelevance(&default_results_.suggest_results);
      ApplyCalculatedSuggestRelevance(&bangs_results_.suggest_results);
      default_results_.verbatim_relevance = -1;
      keyword_results_.verbatim_relevance = -1;
      bangs_results_.verbatim_relevance = -1;
      ConvertResultsToAutocompleteMatches();
    }
    if (!is_extension_keyword &&
        (AutocompleteResult::FindTopMatch(&matches_) == matches_.end())) {
      // Guarantee that SearchProvider returns a legal default match (except
      // when in extension-based keyword mode).  The omnibox always needs at
      // least one legal default match, and it relies on SearchProvider in
      // combination with KeywordProvider (for extension-based keywords) to
      // always return one.  Give the verbatim suggestion the lowest non-zero
      // scores to best reflect what the server desired.
      DCHECK_EQ(0, default_results_.verbatim_relevance);
      default_results_.verbatim_relevance = 1;
      // We do not have to alter keyword_results_.verbatim_relevance here.
      // If the user is in keyword mode, we already reverted (earlier in this
      // function) the instructions to suppress keyword verbatim.
      ConvertResultsToAutocompleteMatches();
    }
    DCHECK(!IsTopMatchSearchWithURLInput());
    DCHECK(is_extension_keyword ||
           (AutocompleteResult::FindTopMatch(&matches_) != matches_.end()));
  }
}

void SearchProvider::RecordTopSuggestion() {
  top_query_suggestion_match_contents_ = base::string16();
  top_navigation_suggestion_ = GURL();
  ACMatches::const_iterator first_match =
      AutocompleteResult::FindTopMatch(matches_);
  if ((first_match != matches_.end()) &&
      !first_match->inline_autocompletion.empty()) {
    // Identify if this match came from a query suggestion or a navsuggestion.
    // In either case, extracts the identifying feature of the suggestion
    // (query string or navigation url).
    if (AutocompleteMatch::IsSearchType(first_match->type))
      top_query_suggestion_match_contents_ = first_match->contents;
    else
      top_navigation_suggestion_ = first_match->destination_url;
  }
}

void SearchProvider::Run(bool query_is_private) {
  // Start a new request with the current input.
  time_suggest_request_sent_ = base::TimeTicks::Now();

  if (!query_is_private && !input_.text().empty() && base::StartsWith(base::UTF16ToUTF8(input_.text()), "!", base::CompareCase::INSENSITIVE_ASCII)) {
    default_fetcher_ =
        CreateSuggestFetcher(kDefaultProviderURLFetcherID,
                             providers_.GetBangsProviderURL(), input_);
  }
  else if (!query_is_private) {
    default_fetcher_ =
        CreateSuggestFetcher(kDefaultProviderURLFetcherID,
                             providers_.GetDefaultProviderURL(), input_);
  }
  keyword_fetcher_ =
      CreateSuggestFetcher(kKeywordProviderURLFetcherID,
                           providers_.GetKeywordProviderURL(), keyword_input_);

  if (client()->GetPrefs()->GetInteger(prefs::kEnableServerSuggestions) > 0) {
    if (!query_is_private) {
      bangs_fetcher_ =
          CreateBangsFetcher(kDefaultProviderURLFetcherID,
                               providers_.GetDefaultProviderURL(), input_);
    }
  }

  // Both the above can fail if the providers have been modified or deleted
  // since the query began.
  if (!default_fetcher_ && !keyword_fetcher_ && !bangs_fetcher_) {
    UpdateDone();
    // We only need to update the listener if we're actually done.
    if (done_)
      listener_->OnProviderUpdate(false);
  } else {
    // Sent at least one request.
    time_suggest_request_sent_ = base::TimeTicks::Now();
  }
}

void SearchProvider::DoHistoryQuery(bool minimal_changes) {
  // The history query results are synchronous, so if minimal_changes is true,
  // we still have the last results and don't need to do anything.
  if (minimal_changes)
    return;

  raw_keyword_history_results_.clear();
  raw_default_history_results_.clear();

  if (OmniboxFieldTrial::SearchHistoryDisable(
      input_.current_page_classification()))
    return;

  history::URLDatabase* url_db = client()->GetInMemoryDatabase();
  if (!url_db)
    return;

  // Request history for both the keyword and default provider.  We grab many
  // more matches than we'll ultimately clamp to so that if there are several
  // recent multi-word matches who scores are lowered (see
  // ScoreHistoryResults()), they won't crowd out older, higher-scoring
  // matches.  Note that this doesn't fix the problem entirely, but merely
  // limits it to cases with a very large number of such multi-word matches; for
  // now, this seems OK compared with the complexity of a real fix, which would
  // require multiple searches and tracking of "single- vs. multi-word" in the
  // database.
  int num_matches = kMaxMatches * 5;
  const TemplateURL* default_url = providers_.GetDefaultProviderURL();
  if (default_url) {
    const base::TimeTicks start_time = base::TimeTicks::Now();
    url_db->GetMostRecentKeywordSearchTerms(default_url->id(),
                                            input_.text(),
                                            num_matches,
                                            &raw_default_history_results_);
    UMA_HISTOGRAM_TIMES(
        "Omnibox.SearchProvider.GetMostRecentKeywordTermsDefaultProviderTime",
        base::TimeTicks::Now() - start_time);
  }
  const TemplateURL* keyword_url = providers_.GetKeywordProviderURL();
  if (keyword_url) {
    url_db->GetMostRecentKeywordSearchTerms(keyword_url->id(),
                                            keyword_input_.text(),
                                            num_matches,
                                            &raw_keyword_history_results_);
  }
}

base::TimeDelta SearchProvider::GetSuggestQueryDelay() const {
  bool from_last_keystroke;
  int polling_delay_ms;
  OmniboxFieldTrial::GetSuggestPollingStrategy(&from_last_keystroke,
                                               &polling_delay_ms);

  base::TimeDelta delay(base::TimeDelta::FromMilliseconds(polling_delay_ms));
  if (from_last_keystroke)
    return delay;

  base::TimeDelta time_since_last_suggest_request =
      base::TimeTicks::Now() - time_suggest_request_sent_;
  return std::max(base::TimeDelta(), delay - time_since_last_suggest_request);
}

void SearchProvider::StartOrStopSuggestQuery(bool minimal_changes) {
  bool query_is_private;
  if (!IsQuerySuitableForSuggest(&query_is_private)) {
    StopSuggest();
    ClearAllResults();
    return;
  }

  if (OmniboxFieldTrial::DisableResultsCaching())
    ClearAllResults();

  // For the minimal_changes case, if we finished the previous query and still
  // have its results, or are allowed to keep running it, just do that, rather
  // than starting a new query.
  if (minimal_changes &&
      (!default_results_.suggest_results.empty() ||
       !default_results_.navigation_results.empty() ||
       !keyword_results_.suggest_results.empty() ||
       !keyword_results_.navigation_results.empty() ||
       !bangs_results_.suggest_results.empty() ||
       !bangs_results_.navigation_results.empty() ||
       (!done_ && input_.want_asynchronous_matches())))
    return;

  // We can't keep running any previous query, so halt it.
  StopSuggest();

  UpdateAllOldResults(minimal_changes);

  // Update the content classifications of remaining results so they look good
  // against the current input.
  UpdateMatchContentsClass(input_.text(), &default_results_);
  if (!keyword_input_.text().empty())
    UpdateMatchContentsClass(keyword_input_.text(), &keyword_results_);

  // We can't start a new query if we're only allowed synchronous results.
  if (!input_.want_asynchronous_matches())
    return;

  // Kick off a timer that will start the URL fetch if it completes before
  // the user types another character.  Requests may be delayed to avoid
  // flooding the server with requests that are likely to be thrown away later
  // anyway.
  const base::TimeDelta delay = GetSuggestQueryDelay();
  if (delay <= base::TimeDelta()) {
    Run(query_is_private);
    return;
  }
  timer_.Start(FROM_HERE,
               delay,
               base::Bind(&SearchProvider::Run,
                          base::Unretained(this),
                          query_is_private));
}

void SearchProvider::CancelFetcher(std::unique_ptr<net::URLFetcher>* fetcher) {
  if (*fetcher) {
    LogOmniboxSuggestRequest(REQUEST_INVALIDATED);
    fetcher->reset();
  }
}

bool SearchProvider::IsQuerySuitableForSuggest(bool* query_is_private) const {
  *query_is_private = IsQueryPotentiallyPrivate();

  // Don't run Suggest in incognito mode, if the engine doesn't support it, or
  // if the user has disabled it.  Also don't send potentially private data
  // to the default search provider.  (It's always okay to send explicit
  // keyword input to a keyword suggest server, if any.)
  const TemplateURL* default_url = providers_.GetDefaultProviderURL();
  const TemplateURL* keyword_url = providers_.GetKeywordProviderURL();
  return !client()->IsOffTheRecord() && client()->SearchSuggestEnabled() &&
         ((default_url && !default_url->suggestions_url().empty() &&
           !*query_is_private) ||
          (keyword_url && !keyword_url->suggestions_url().empty()));
}

bool SearchProvider::IsQueryPotentiallyPrivate() const {
  if (input_.text().empty())
    return false;

  // Check the scheme.  If this is UNKNOWN/URL with a scheme that isn't
  // http/https/ftp, we shouldn't send it.  Sending things like file: and data:
  // is both a waste of time and a disclosure of potentially private, local
  // data.  Other "schemes" may actually be usernames, and we don't want to send
  // passwords.  If the scheme is OK, we still need to check other cases below.
  // If this is QUERY, then the presence of these schemes means the user
  // explicitly typed one, and thus this is probably a URL that's being entered
  // and happens to currently be invalid -- in which case we again want to run
  // our checks below.  Other QUERY cases are less likely to be URLs and thus we
  // assume we're OK.
  if (!base::LowerCaseEqualsASCII(input_.scheme(), url::kHttpScheme) &&
      !base::LowerCaseEqualsASCII(input_.scheme(), url::kHttpsScheme) &&
      !base::LowerCaseEqualsASCII(input_.scheme(), url::kFtpScheme))
    return (input_.type() != metrics::OmniboxInputType::QUERY);

  // Don't send URLs with usernames, queries or refs.  Some of these are
  // private, and the Suggest server is unlikely to have any useful results
  // for any of them.  Also don't send URLs with ports, as we may initially
  // think that a username + password is a host + port (and we don't want to
  // send usernames/passwords), and even if the port really is a port, the
  // server is once again unlikely to have and useful results.
  // Note that we only block based on refs if the input is URL-typed, as search
  // queries can legitimately have #s in them which the URL parser
  // overaggressively categorizes as a url with a ref.
  const url::Parsed& parts = input_.parts();
  if (parts.username.is_nonempty() || parts.port.is_nonempty() ||
      parts.query.is_nonempty() ||
      (parts.ref.is_nonempty() &&
       (input_.type() == metrics::OmniboxInputType::URL)))
    return true;

  // Don't send anything for https except the hostname.  Hostnames are OK
  // because they are visible when the TCP connection is established, but the
  // specific path may reveal private information.
  if (base::LowerCaseEqualsASCII(input_.scheme(), url::kHttpsScheme) &&
      parts.path.is_nonempty())
    return true;

  return false;
}

void SearchProvider::UpdateAllOldResults(bool minimal_changes) {
  if (keyword_input_.text().empty()) {
    // User is either in keyword mode with a blank input or out of
    // keyword mode entirely.
    keyword_results_.Clear();
  }
  UpdateOldResults(minimal_changes, &default_results_);
  UpdateOldResults(minimal_changes, &keyword_results_);
  UpdateOldResults(minimal_changes, &bangs_results_);
}

void SearchProvider::PersistTopSuggestions(
    SearchSuggestionParser::Results* results) {
  // Mark any results matching the current top results as having been received
  // prior to the last keystroke.  That prevents asynchronous updates from
  // clobbering top results, which may be used for inline autocompletion.
  // Other results don't need similar changes, because they shouldn't be
  // displayed asynchronously anyway.
  if (!top_query_suggestion_match_contents_.empty()) {
    for (SearchSuggestionParser::SuggestResults::iterator sug_it =
             results->suggest_results.begin();
         sug_it != results->suggest_results.end(); ++sug_it) {
      if (sug_it->match_contents() == top_query_suggestion_match_contents_)
        sug_it->set_received_after_last_keystroke(false);
    }
  }
  if (top_navigation_suggestion_.is_valid()) {
    for (SearchSuggestionParser::NavigationResults::iterator nav_it =
             results->navigation_results.begin();
         nav_it != results->navigation_results.end(); ++nav_it) {
      if (nav_it->url() == top_navigation_suggestion_)
        nav_it->set_received_after_last_keystroke(false);
    }
  }
}

void SearchProvider::ApplyCalculatedSuggestRelevance(
    SearchSuggestionParser::SuggestResults* list) {
  for (size_t i = 0; i < list->size(); ++i) {
    SearchSuggestionParser::SuggestResult& result = (*list)[i];
    result.set_relevance(
        result.CalculateRelevance(input_, providers_.has_keyword_provider()) +
        (list->size() - i - 1));
    result.set_relevance_from_server(false);
  }
}

void SearchProvider::ApplyCalculatedNavigationRelevance(
    SearchSuggestionParser::NavigationResults* list) {
  for (size_t i = 0; i < list->size(); ++i) {
    SearchSuggestionParser::NavigationResult& result = (*list)[i];
    result.set_relevance(
        result.CalculateRelevance(input_, providers_.has_keyword_provider()) +
        (list->size() - i - 1));
    result.set_relevance_from_server(false);
  }
}

std::unique_ptr<net::URLFetcher> SearchProvider::CreateSuggestFetcher(
    int id,
    const TemplateURL* template_url,
    const AutocompleteInput& input) {
  if (!template_url || template_url->suggestions_url().empty())
    return nullptr;

  // Bail if the suggestion URL is invalid with the given replacements.
  TemplateURLRef::SearchTermsArgs search_term_args(input.text());
  search_term_args.input_type = input.type();
  search_term_args.cursor_position = input.cursor_position();
  search_term_args.page_classification = input.current_page_classification();
  // Session token and prefetch data required for answers.
  search_term_args.session_token = GetSessionToken();
  if (!prefetch_data_.full_query_text.empty()) {
    search_term_args.prefetch_query =
        base::UTF16ToUTF8(prefetch_data_.full_query_text);
    search_term_args.prefetch_query_type =
        base::UTF16ToUTF8(prefetch_data_.query_type);
  }

  GURL suggest_url(template_url->suggestions_url_ref().ReplaceSearchTerms(
      search_term_args,
      client()->GetTemplateURLService()->search_terms_data()));

  if (!suggest_url.is_valid())
    return nullptr;

  // Send the current page URL if user setting and URL requirements are met.
  TemplateURLService* template_url_service = client()->GetTemplateURLService();
  if (CanSendURL(input.current_url(), suggest_url, template_url,
                 input.current_page_classification(),
                 template_url_service->search_terms_data(), client())) {
    search_term_args.current_page_url = input.current_url().spec();
    // Create the suggest URL again with the current page URL.
    suggest_url = GURL(template_url->suggestions_url_ref().ReplaceSearchTerms(
        search_term_args, template_url_service->search_terms_data()));
  }

  LogOmniboxSuggestRequest(REQUEST_SENT);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("omnibox_suggest", R"(
        semantics {
          sender: "Omnibox"
          description:
            "Chrome can provide search and navigation suggestions from the "
            "currently-selected search provider in the omnibox dropdown, based "
            "on user input."
          trigger: "User typing in the omnibox."
          data:
            "The text typed into the address bar. Potentially other metadata, "
            "such as the current cursor position or URL of the current page."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting:
            "Users can control this feature via the 'Use a prediction service "
            "to help complete searches and URLs typed in the address bar' "
            "setting under 'Privacy'. The feature is enabled by default."
          chrome_policy {
            SearchSuggestEnabled {
                policy_options {mode: MANDATORY}
                SearchSuggestEnabled: false
            }
          }
        })");
  std::unique_ptr<net::URLFetcher> fetcher = net::URLFetcher::Create(
      id, suggest_url, net::URLFetcher::GET, this, traffic_annotation);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher.get(), data_use_measurement::DataUseUserData::OMNIBOX);
  fetcher->SetRequestContext(client()->GetRequestContext());
  fetcher->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES);
  // Add Chrome experiment state to the request headers.
  net::HttpRequestHeaders headers;
  // Note: It's OK to pass SignedIn::kNo if it's unknown, as it does not affect
  // transmission of experiments coming from the variations server.
  variations::AppendVariationHeaders(fetcher->GetOriginalURL(),
                                     client()->IsOffTheRecord()
                                         ? variations::InIncognito::kYes
                                         : variations::InIncognito::kNo,
                                     variations::SignedIn::kNo, &headers);
  fetcher->SetExtraRequestHeaders(headers.ToString());
  fetcher->Start();
  return fetcher;
}

std::unique_ptr<net::URLFetcher> SearchProvider::CreateBangsFetcher(
    int id,
    const TemplateURL* template_url,
    const AutocompleteInput& input) {
  if (!template_url || template_url->suggestions_url().empty())
    return nullptr;

  // Bail if the suggestion URL is invalid with the given replacements.
  TemplateURLRef::SearchTermsArgs search_term_args(input.text());
  search_term_args.input_type = input.type();
  search_term_args.cursor_position = input.cursor_position();
  search_term_args.page_classification = input.current_page_classification();
  // Session token and prefetch data required for answers.
  search_term_args.session_token = GetSessionToken();
  if (!prefetch_data_.full_query_text.empty()) {
    search_term_args.prefetch_query =
        base::UTF16ToUTF8(prefetch_data_.full_query_text);
    search_term_args.prefetch_query_type =
        base::UTF16ToUTF8(prefetch_data_.query_type);
  }

  TemplateURLData template_url_data;
  template_url_data.SetShortName(base::ASCIIToUTF16("bangs"));
  template_url_data.SetURL("{searchTerms}");
  long firstInstallDate = base::android::SysUtils::FirstInstallDateFromJni();
  template_url_data.suggestions_url = "https://autocomplete.kiwibrowser.org/suggest/?version=1&install_date=" + base::NumberToString(firstInstallDate) + "&q={searchTerms}";
  template_url_data.id = SEARCH_ENGINE_KIWI;
  TemplateURL bang_template_url(template_url_data);

  GURL suggest_url(bang_template_url.suggestions_url_ref().ReplaceSearchTerms(
      search_term_args,
      client()->GetTemplateURLService()->search_terms_data()));

  if (!suggest_url.is_valid())
    return nullptr;

  // We do not enable additional autocomplete for Bing & Yahoo
  TemplateURLService* template_url_service = client()->GetTemplateURLService();
  if (template_url_service) {
    const TemplateURL* default_provider = client()->GetTemplateURLService()->GetDefaultSearchProvider();
    if (default_provider) {
      if (default_provider->data().prepopulate_id == 3 /* BING */ || default_provider->data().prepopulate_id == 2 /* YAHOO */ )
        return nullptr;
    }
  }

  LogOmniboxSuggestRequest(REQUEST_SENT);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("omnibox_suggest", R"(
        semantics {
          sender: "Omnibox"
          description:
            "Chrome can provide search and navigation suggestions from the "
            "currently-selected search provider in the omnibox dropdown, based "
            "on user input."
          trigger: "User typing in the omnibox."
          data:
            "The text typed into the address bar. Potentially other metadata, "
            "such as the current cursor position or URL of the current page."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting:
            "Users can control this feature via the 'Use a prediction service "
            "to help complete searches and URLs typed in the address bar' "
            "setting under 'Privacy'. The feature is enabled by default."
          chrome_policy {
            SearchSuggestEnabled {
                policy_options {mode: MANDATORY}
                SearchSuggestEnabled: false
            }
          }
        })");
  std::unique_ptr<net::URLFetcher> fetcher = net::URLFetcher::Create(
      id, suggest_url, net::URLFetcher::GET, this, traffic_annotation);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher.get(), data_use_measurement::DataUseUserData::OMNIBOX);
  fetcher->SetRequestContext(client()->GetRequestContext());
  fetcher->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES);
  // Add Chrome experiment state to the request headers.
  net::HttpRequestHeaders headers;
  // Note: It's OK to pass SignedIn::kNo if it's unknown, as it does not affect
  // transmission of experiments coming from the variations server.
  variations::AppendVariationHeaders(fetcher->GetOriginalURL(),
                                     client()->IsOffTheRecord()
                                         ? variations::InIncognito::kYes
                                         : variations::InIncognito::kNo,
                                     variations::SignedIn::kNo, &headers);
  fetcher->SetExtraRequestHeaders(headers.ToString());
  fetcher->Start();
  return fetcher;
}

void SearchProvider::ConvertResultsToAutocompleteMatches() {
  // Convert all the results to matches and add them to a map, so we can keep
  // the most relevant match for each result.
  base::TimeTicks start_time(base::TimeTicks::Now());
  MatchMap map;
  int did_not_accept_keyword_suggestion =
      keyword_results_.suggest_results.empty() ?
      TemplateURLRef::NO_SUGGESTIONS_AVAILABLE :
      TemplateURLRef::NO_SUGGESTION_CHOSEN;

  bool relevance_from_server;
  int verbatim_relevance = GetVerbatimRelevance(&relevance_from_server);
  int did_not_accept_default_suggestion =
      default_results_.suggest_results.empty() ?
      TemplateURLRef::NO_SUGGESTIONS_AVAILABLE :
      TemplateURLRef::NO_SUGGESTION_CHOSEN;
  const TemplateURL* keyword_url = providers_.GetKeywordProviderURL();
  if (verbatim_relevance > 0) {
    const base::string16& trimmed_verbatim =
        base::CollapseWhitespace(input_.text(), false);

    // Verbatim results don't get suggestions and hence, answers.
    // Scan previous matches if the last answer-bearing suggestion matches
    // verbatim, and if so, copy over answer contents.
    base::string16 answer_contents;
    base::string16 answer_type;
    std::unique_ptr<SuggestionAnswer> answer;
    base::string16 trimmed_verbatim_lower =
        base::i18n::ToLower(trimmed_verbatim);
    for (ACMatches::iterator it = matches_.begin(); it != matches_.end();
         ++it) {
      if (it->answer &&
          base::i18n::ToLower(it->fill_into_edit) == trimmed_verbatim_lower) {
        answer_contents = it->answer_contents;
        answer_type = it->answer_type;
        answer = SuggestionAnswer::copy(it->answer.get());
        break;
      }
    }

    SearchSuggestionParser::SuggestResult verbatim(
        /*suggestion=*/trimmed_verbatim,
        AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED,
        /*subtype_identifier=*/0,
        /*match_contents=*/trimmed_verbatim,
        /*match_contents_prefix=*/base::string16(),
        /*annotation=*/base::string16(), answer_contents, answer_type,
        std::move(answer), /*suggest_query_params=*/std::string(),
        /*deletion_url=*/std::string(),
        /*image_dominant_color=*/std::string(),
        /*image_url=*/std::string(),
        /*from_keyword_provider=*/false, verbatim_relevance,
        relevance_from_server, /*should_prefetch=*/false,
        /*input_text=*/trimmed_verbatim);
    AddMatchToMap(verbatim, std::string(), did_not_accept_default_suggestion,
                  false, keyword_url != nullptr, &map);
  }
  if (!keyword_input_.text().empty()) {
    // We only create the verbatim search query match for a keyword
    // if it's not an extension keyword.  Extension keywords are handled
    // in KeywordProvider::Start().  (Extensions are complicated...)
    // Note: in this provider, SEARCH_OTHER_ENGINE must correspond
    // to the keyword verbatim search query.  Do not create other matches
    // of type SEARCH_OTHER_ENGINE.
    if (keyword_url &&
        (keyword_url->type() != TemplateURL::OMNIBOX_API_EXTENSION)) {
      bool keyword_relevance_from_server;
      const int keyword_verbatim_relevance =
          GetKeywordVerbatimRelevance(&keyword_relevance_from_server);
      if (keyword_verbatim_relevance > 0) {
        const base::string16& trimmed_verbatim =
            base::CollapseWhitespace(keyword_input_.text(), false);
        SearchSuggestionParser::SuggestResult verbatim(
            /*suggestion=*/trimmed_verbatim,
            AutocompleteMatchType::SEARCH_OTHER_ENGINE,
            /*subtype_identifier=*/0,
            /*match_contents=*/trimmed_verbatim,
            /*match_contents_prefix=*/base::string16(),
            /*annotation=*/base::string16(),
            /*answer_contents=*/base::string16(),
            /*answer_type=*/base::string16(),
            /*answer=*/nullptr,
            /*suggest_query_params=*/std::string(),
            /*deletion_url=*/std::string(),
            /*image_dominant_color=*/std::string(),
            /*image_url=*/std::string(),
            /*from_keyword_provider=*/true, keyword_verbatim_relevance,
            keyword_relevance_from_server,
            /*should_prefetch=*/false,
            /*input_text=*/trimmed_verbatim);
        AddMatchToMap(verbatim, std::string(),
                      did_not_accept_keyword_suggestion, false, true, &map);
      }
    }
  }
  AddRawHistoryResultsToMap(true, did_not_accept_keyword_suggestion, &map);
  AddRawHistoryResultsToMap(false, did_not_accept_default_suggestion, &map);

  AddSuggestResultsToMap(keyword_results_.suggest_results,
                         keyword_results_.metadata, &map);
  AddSuggestResultsToMap(default_results_.suggest_results,
                         default_results_.metadata, &map);
  AddSuggestResultsToMap(bangs_results_.suggest_results,
                         bangs_results_.metadata, &map);

  ACMatches matches;
  for (MatchMap::const_iterator i(map.begin()); i != map.end(); ++i)
    matches.push_back(i->second);

  AddNavigationResultsToMatches(keyword_results_.navigation_results, &matches);
  AddNavigationResultsToMatches(default_results_.navigation_results, &matches);
  AddNavigationResultsToMatches(bangs_results_.navigation_results, &matches);

  // Now add the most relevant matches to |matches_|.  We take up to kMaxMatches
  // suggest/navsuggest matches, regardless of origin.  We always include in
  // that set a legal default match if possible.  If Instant Extended is enabled
  // and we have server-provided (and thus hopefully more accurate) scores for
  // some suggestions, we allow more of those, until we reach
  // AutocompleteResult::GetMaxMatches() total matches (that is, enough to fill
  // the whole popup).
  //
  // We will always return any verbatim matches, no matter how we obtained their
  // scores, unless we have already accepted AutocompleteResult::GetMaxMatches()
  // higher-scoring matches under the conditions above.
  std::sort(matches.begin(), matches.end(), &AutocompleteMatch::MoreRelevant);

  // Guarantee that if there's a legal default match anywhere in the result
  // set that it'll get returned.  The rotate() call does this by moving the
  // default match to the front of the list.
  ACMatches::iterator default_match =
      AutocompleteResult::FindTopMatch(&matches);
  if (default_match != matches.end())
    std::rotate(matches.begin(), default_match, default_match + 1);

  // It's possible to get a copy of an answer from previous matches and get the
  // same or a different answer to another server-provided suggestion.  In the
  // future we may decide that we want to have answers attached to multiple
  // suggestions, but the current assumption is that there should only ever be
  // one suggestion with an answer.  To maintain this assumption, remove any
  // answers after the first.
  RemoveExtraAnswers(&matches);

  matches_.clear();
  size_t num_suggestions = 0;
  for (ACMatches::const_iterator i(matches.begin());
       (i != matches.end()) &&
       (matches_.size() < AutocompleteResult::GetMaxMatches());
       ++i) {
    // SEARCH_OTHER_ENGINE is only used in the SearchProvider for the keyword
    // verbatim result, so this condition basically means "if this match is a
    // suggestion of some sort".
    if ((i->type != AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED) &&
        (i->type != AutocompleteMatchType::SEARCH_OTHER_ENGINE)) {
      // If we've already hit the limit on non-server-scored suggestions, and
      // this isn't a server-scored suggestion we can add, skip it.
      if ((num_suggestions >= kMaxMatches) &&
          (!search::IsInstantExtendedAPIEnabled() ||
           (i->GetAdditionalInfo(kRelevanceFromServerKey) != kTrue))) {
        continue;
      }

      ++num_suggestions;
    }

    matches_.push_back(*i);
  }
  UMA_HISTOGRAM_TIMES("Omnibox.SearchProvider.ConvertResultsTime",
                      base::TimeTicks::Now() - start_time);
}

void SearchProvider::RemoveExtraAnswers(ACMatches* matches) {
  bool answer_seen = false;
  for (ACMatches::iterator it = matches->begin(); it != matches->end(); ++it) {
    if (it->answer) {
      if (!answer_seen) {
        answer_seen = true;
      } else {
        it->answer_contents.clear();
        it->answer_type.clear();
        it->answer.reset();
      }
    }
  }
}

bool SearchProvider::IsTopMatchSearchWithURLInput() const {
  ACMatches::const_iterator first_match =
      AutocompleteResult::FindTopMatch(matches_);
  return (input_.type() == metrics::OmniboxInputType::URL) &&
         (first_match != matches_.end()) &&
         (first_match->relevance > CalculateRelevanceForVerbatim()) &&
         (first_match->type != AutocompleteMatchType::NAVSUGGEST) &&
         (first_match->type != AutocompleteMatchType::NAVSUGGEST_PERSONALIZED);
}

void SearchProvider::AddNavigationResultsToMatches(
    const SearchSuggestionParser::NavigationResults& navigation_results,
    ACMatches* matches) {
  for (SearchSuggestionParser::NavigationResults::const_iterator it =
           navigation_results.begin(); it != navigation_results.end(); ++it) {
    matches->push_back(NavigationToMatch(*it));
    // In the absence of suggested relevance scores, use only the single
    // highest-scoring result.  (The results are already sorted by relevance.)
    if (!it->relevance_from_server())
      return;
  }
}

void SearchProvider::AddRawHistoryResultsToMap(bool is_keyword,
                                               int did_not_accept_suggestion,
                                               MatchMap* map) {
  base::TimeTicks start_time(base::TimeTicks::Now());

  const SearchSuggestionParser::SuggestResults* transformed_results =
      is_keyword ? &transformed_keyword_history_results_
                 : &transformed_default_history_results_;
  DCHECK(transformed_results);
  AddTransformedHistoryResultsToMap(
      *transformed_results, did_not_accept_suggestion, map);
  UMA_HISTOGRAM_TIMES("Omnibox.SearchProvider.AddHistoryResultsTime",
                      base::TimeTicks::Now() - start_time);
}

void SearchProvider::AddTransformedHistoryResultsToMap(
    const SearchSuggestionParser::SuggestResults& transformed_results,
    int did_not_accept_suggestion,
    MatchMap* map) {
  for (SearchSuggestionParser::SuggestResults::const_iterator i(
           transformed_results.begin());
       i != transformed_results.end();
       ++i) {
    AddMatchToMap(*i, std::string(), did_not_accept_suggestion, true,
                  providers_.GetKeywordProviderURL() != nullptr, map);
  }
}

SearchSuggestionParser::SuggestResults
SearchProvider::ScoreHistoryResultsHelper(const HistoryResults& results,
                                          bool base_prevent_inline_autocomplete,
                                          bool input_multiple_words,
                                          const base::string16& input_text,
                                          bool is_keyword) {
  SearchSuggestionParser::SuggestResults scored_results;
  // True if the user has asked this exact query previously.
  bool found_what_you_typed_match = false;
  const bool prevent_search_history_inlining =
      OmniboxFieldTrial::SearchHistoryPreventInlining(
          input_.current_page_classification());
  const base::string16& trimmed_input =
      base::CollapseWhitespace(input_text, false);
  for (HistoryResults::const_iterator i(results.begin()); i != results.end();
       ++i) {
    const base::string16& trimmed_suggestion =
        base::CollapseWhitespace(i->term, false);

    // Don't autocomplete multi-word queries that have only been seen once
    // unless the user has typed more than one word.
    bool prevent_inline_autocomplete = base_prevent_inline_autocomplete ||
        (!input_multiple_words && (i->visits < 2) &&
         HasMultipleWords(trimmed_suggestion));

    int relevance = CalculateRelevanceForHistory(
        i->time, is_keyword, !prevent_inline_autocomplete,
        prevent_search_history_inlining);
    // Add the match to |scored_results| by putting the what-you-typed match
    // on the front and appending all other matches.  We want the what-you-
    // typed match to always be first.
    SearchSuggestionParser::SuggestResults::iterator insertion_position =
        scored_results.end();
    if (trimmed_suggestion == trimmed_input) {
      found_what_you_typed_match = true;
      insertion_position = scored_results.begin();
    }
    SearchSuggestionParser::SuggestResult history_suggestion(
        /*suggestion=*/trimmed_suggestion,
        AutocompleteMatchType::SEARCH_HISTORY,
        /*subtype_identifier=*/0,
        /*match_contents=*/trimmed_suggestion,
        /*match_contents_prefix=*/base::string16(),
        /*annotation=*/base::string16(),
        /*answer_contents=*/base::string16(),
        /*answer_type=*/base::string16(),
        /*answer=*/nullptr,
        /*suggest_query_params=*/std::string(),
        /*deletion_url=*/std::string(),
        /*image_dominant_color=*/std::string(),
        /*image_url=*/std::string(), is_keyword, relevance,
        /*relevance_from_server=*/false,
        /*should_prefetch=*/false, /*input_text=*/trimmed_input);
    // History results are synchronous; they are received on the last keystroke.
    history_suggestion.set_received_after_last_keystroke(false);
    scored_results.insert(insertion_position, history_suggestion);
  }

  // History returns results sorted for us.  However, we may have docked some
  // results' scores, so things are no longer in order.  While keeping the
  // what-you-typed match at the front (if it exists), do a stable sort to get
  // things back in order without otherwise disturbing results with equal
  // scores, then force the scores to be unique, so that the order in which
  // they're shown is deterministic.
  std::stable_sort(scored_results.begin() +
                       (found_what_you_typed_match ? 1 : 0),
                   scored_results.end(),
                   CompareScoredResults());

  // Don't autocomplete to search terms that would normally be treated as URLs
  // when typed. For example, if the user searched for "google.com" and types
  // "goog", don't autocomplete to the search term "google.com". Otherwise,
  // the input will look like a URL but act like a search, which is confusing.
  // The 1200 relevance score threshold in the test below is the lowest
  // possible score in CalculateRelevanceForHistory()'s aggressive-scoring
  // curve.  This is an appropriate threshold to use to decide if we're overly
  // aggressively inlining because, if we decide the answer is yes, the
  // way we resolve it it to not use the aggressive-scoring curve.
  // NOTE: We don't check for autocompleting to URLs in the following cases:
  //  * When inline autocomplete is disabled, we won't be inline autocompleting
  //    this term, so we don't need to worry about confusion as much.  This
  //    also prevents calling Classify() again from inside the classifier
  //    (which will corrupt state and likely crash), since the classifier
  //    always disables inline autocomplete.
  //  * When the user has typed the whole string before as a query, then it's
  //    likely the user has no expectation that term should be interpreted as
  //    as a URL, so we need not do anything special to preserve user
  //    expectation.
  int last_relevance = 0;
  if (!base_prevent_inline_autocomplete && !found_what_you_typed_match &&
      scored_results.front().relevance() >= 1200) {
    AutocompleteMatch match;
    client()->Classify(scored_results.front().suggestion(), false, false,
                       input_.current_page_classification(), &match, nullptr);
    // Demote this match that would normally be interpreted as a URL to have
    // the highest score a previously-issued search query could have when
    // scoring with the non-aggressive method.  A consequence of demoting
    // by revising |last_relevance| is that this match and all following
    // matches get demoted; the relative order of matches is preserved.
    // One could imagine demoting only those matches that might cause
    // confusion (which, by the way, might change the relative order of
    // matches.  We have decided to go with the simple demote-all approach
    // because selective demotion requires multiple Classify() calls and
    // such calls can be expensive (as expensive as running the whole
    // autocomplete system).
    if (!AutocompleteMatch::IsSearchType(match.type)) {
      last_relevance = CalculateRelevanceForHistory(
          base::Time::Now(), is_keyword, false,
          prevent_search_history_inlining);
    }
  }

  for (SearchSuggestionParser::SuggestResults::iterator i(
           scored_results.begin()); i != scored_results.end(); ++i) {
    if ((last_relevance != 0) && (i->relevance() >= last_relevance))
      i->set_relevance(last_relevance - 1);
    last_relevance = i->relevance();
  }

  return scored_results;
}

void SearchProvider::ScoreHistoryResults(
    const HistoryResults& results,
    bool is_keyword,
    SearchSuggestionParser::SuggestResults* scored_results) {
  DCHECK(scored_results);
  scored_results->clear();

  if (results.empty()) {
    return;
  }

  bool prevent_inline_autocomplete =
      input_.prevent_inline_autocomplete() ||
      (input_.type() == metrics::OmniboxInputType::URL);
  const base::string16 input_text = GetInput(is_keyword).text();
  bool input_multiple_words = HasMultipleWords(input_text);

  if (!prevent_inline_autocomplete && input_multiple_words) {
    // ScoreHistoryResultsHelper() allows autocompletion of multi-word, 1-visit
    // queries if the input also has multiple words.  But if we were already
    // scoring a multi-word, multi-visit query aggressively, and the current
    // input is still a prefix of it, then changing the suggestion suddenly
    // feels wrong.  To detect this case, first score as if only one word has
    // been typed, then check if the best result came from aggressive search
    // history scoring.  If it did, then just keep that score set.  This
    // 1200 the lowest possible score in CalculateRelevanceForHistory()'s
    // aggressive-scoring curve.
    *scored_results = ScoreHistoryResultsHelper(
        results, prevent_inline_autocomplete, false, input_text, is_keyword);
    if ((scored_results->front().relevance() < 1200) ||
        !HasMultipleWords(scored_results->front().suggestion()))
      scored_results->clear();  // Didn't detect the case above, score normally.
  }
  if (scored_results->empty()) {
    *scored_results = ScoreHistoryResultsHelper(results,
                                                prevent_inline_autocomplete,
                                                input_multiple_words,
                                                input_text,
                                                is_keyword);
  }
}

void SearchProvider::AddSuggestResultsToMap(
    const SearchSuggestionParser::SuggestResults& results,
    const std::string& metadata,
    MatchMap* map) {
  for (size_t i = 0; i < results.size(); ++i) {
    AddMatchToMap(results[i], metadata, i, false,
                  providers_.GetKeywordProviderURL() != nullptr, map);
  }
}

int SearchProvider::GetVerbatimRelevance(bool* relevance_from_server) const {
  // Use the suggested verbatim relevance score if it is non-negative (valid),
  // if inline autocomplete isn't prevented (always show verbatim on backspace),
  // and if it won't suppress verbatim, leaving no default provider matches.
  // Otherwise, if the default provider returned no matches and was still able
  // to suppress verbatim, the user would have no search/nav matches and may be
  // left unable to search using their default provider from the omnibox.
  // Check for results on each verbatim calculation, as results from older
  // queries (on previous input) may be trimmed for failing to inline new input.
  bool use_server_relevance =
      (default_results_.verbatim_relevance >= 0) &&
      !input_.prevent_inline_autocomplete() &&
      ((default_results_.verbatim_relevance > 0) ||
       !default_results_.suggest_results.empty() ||
       !default_results_.navigation_results.empty());
  if (relevance_from_server)
    *relevance_from_server = use_server_relevance;
  return use_server_relevance ?
      default_results_.verbatim_relevance : CalculateRelevanceForVerbatim();
}

int SearchProvider::CalculateRelevanceForVerbatim() const {
  if (!providers_.keyword_provider().empty())
    return 250;
  return CalculateRelevanceForVerbatimIgnoringKeywordModeState();
}

int SearchProvider::
    CalculateRelevanceForVerbatimIgnoringKeywordModeState() const {
  switch (input_.type()) {
    case metrics::OmniboxInputType::UNKNOWN:
    case metrics::OmniboxInputType::QUERY:
      return kNonURLVerbatimRelevance;

    case metrics::OmniboxInputType::URL:
      return 850;

    default:
      NOTREACHED();
      return 0;
  }
}

int SearchProvider::GetKeywordVerbatimRelevance(
    bool* relevance_from_server) const {
  // Use the suggested verbatim relevance score if it is non-negative (valid),
  // if inline autocomplete isn't prevented (always show verbatim on backspace),
  // and if it won't suppress verbatim, leaving no keyword provider matches.
  // Otherwise, if the keyword provider returned no matches and was still able
  // to suppress verbatim, the user would have no search/nav matches and may be
  // left unable to search using their keyword provider from the omnibox.
  // Check for results on each verbatim calculation, as results from older
  // queries (on previous input) may be trimmed for failing to inline new input.
  bool use_server_relevance =
      (keyword_results_.verbatim_relevance >= 0) &&
      !input_.prevent_inline_autocomplete() &&
      ((keyword_results_.verbatim_relevance > 0) ||
       !keyword_results_.suggest_results.empty() ||
       !keyword_results_.navigation_results.empty());
  if (relevance_from_server)
    *relevance_from_server = use_server_relevance;
  return use_server_relevance ?
      keyword_results_.verbatim_relevance :
      CalculateRelevanceForKeywordVerbatim(keyword_input_.type(),
                                           true,
                                           keyword_input_.prefer_keyword());
}

int SearchProvider::CalculateRelevanceForHistory(
    const base::Time& time,
    bool is_keyword,
    bool use_aggressive_method,
    bool prevent_search_history_inlining) const {
  // The relevance of past searches falls off over time. There are two distinct
  // equations used. If the first equation is used (searches to the primary
  // provider that we want to score aggressively), the score is in the range
  // 1300-1599 (unless |prevent_search_history_inlining|, in which case
  // it's in the range 1200-1299). If the second equation is used the
  // relevance of a search 15 minutes ago is discounted 50 points, while the
  // relevance of a search two weeks ago is discounted 450 points.
  double elapsed_time = std::max((base::Time::Now() - time).InSecondsF(), 0.0);
  bool is_primary_provider = is_keyword || !providers_.has_keyword_provider();
  if (is_primary_provider && use_aggressive_method) {
    // Searches with the past two days get a different curve.
    const double autocomplete_time = 2 * 24 * 60 * 60;
    if (elapsed_time < autocomplete_time) {
      int max_score = is_keyword ? 1599 : 1399;
      if (prevent_search_history_inlining)
        max_score = 1299;
      return max_score - static_cast<int>(99 *
          std::pow(elapsed_time / autocomplete_time, 2.5));
    }
    elapsed_time -= autocomplete_time;
  }

  const int score_discount =
      static_cast<int>(6.5 * std::pow(elapsed_time, 0.3));

  // Don't let scores go below 0.  Negative relevance scores are meaningful in
  // a different way.
  int base_score;
  if (is_primary_provider)
    base_score = (input_.type() == metrics::OmniboxInputType::URL) ? 750 : 1050;
  else
    base_score = 200;
  return std::max(0, base_score - score_discount);
}

AutocompleteMatch SearchProvider::NavigationToMatch(
    const SearchSuggestionParser::NavigationResult& navigation) {
  base::string16 input;
  const bool trimmed_whitespace = base::TrimWhitespace(
      navigation.from_keyword_provider() ?
          keyword_input_.text() : input_.text(),
      base::TRIM_TRAILING, &input) != base::TRIM_NONE;
  AutocompleteMatch match(this, navigation.relevance(), false,
                          navigation.type());
  match.destination_url = navigation.url();
  match.subtype_identifier = navigation.subtype_identifier();
  BaseSearchProvider::SetDeletionURL(navigation.deletion_url(), &match);
  // First look for the user's input inside the formatted url as it would be
  // without trimming the scheme, so we can find matches at the beginning of the
  // scheme.
  const URLPrefix* prefix =
      URLPrefix::BestURLPrefix(navigation.formatted_url(), input);
  size_t match_start = (prefix == nullptr)
                           ? navigation.formatted_url().find(input)
                           : prefix->prefix.length();
  bool trim_http = !AutocompleteInput::HasHTTPScheme(input) &&
      (!prefix || (match_start != 0));
  const url_formatter::FormatUrlTypes format_types =
      url_formatter::kFormatUrlOmitDefaults &
      ~(trim_http ? 0 : url_formatter::kFormatUrlOmitHTTP);

  size_t inline_autocomplete_offset = (prefix == nullptr)
                                          ? base::string16::npos
                                          : (match_start + input.length());
  match.fill_into_edit +=
      AutocompleteInput::FormattedStringWithEquivalentMeaning(
          navigation.url(),
          url_formatter::FormatUrl(navigation.url(), format_types,
                                   net::UnescapeRule::SPACES, nullptr, nullptr,
                                   &inline_autocomplete_offset),
          client()->GetSchemeClassifier(), &inline_autocomplete_offset);
  if (inline_autocomplete_offset != base::string16::npos) {
    DCHECK(inline_autocomplete_offset <= match.fill_into_edit.length());
    match.inline_autocompletion =
        match.fill_into_edit.substr(inline_autocomplete_offset);
  }
  // An inlinable navsuggestion can only be the default match when there
  // is no keyword provider active, lest it appear first and break the user
  // out of keyword mode.  We also must have received the navsuggestion before
  // the last keystroke, to prevent asynchronous inline autocompletions changes.
  // The navsuggestion can also only be default if either the inline
  // autocompletion is empty or we're not preventing inline autocompletion.
  // Finally, if we have an inlinable navsuggestion with an inline completion
  // that we're not preventing, make sure we didn't trim any whitespace.
  // We don't want to claim http://foo.com/bar is inlinable against the
  // input "foo.com/b ".
  match.allowed_to_be_default_match =
      (prefix != nullptr) && (providers_.GetKeywordProviderURL() == nullptr) &&
      !navigation.received_after_last_keystroke() &&
      (match.inline_autocompletion.empty() ||
       (!input_.prevent_inline_autocomplete() && !trimmed_whitespace));
  match.EnsureUWYTIsAllowedToBeDefault(input_,
                                       client()->GetTemplateURLService());

  match.contents = navigation.match_contents();
  match.contents_class = navigation.match_contents_class();
  match.description = navigation.description();
  AutocompleteMatch::ClassifyMatchInString(input, match.description,
      ACMatchClassification::NONE, &match.description_class);

  match.RecordAdditionalInfo(
      kRelevanceFromServerKey,
      navigation.relevance_from_server() ? kTrue : kFalse);
  match.RecordAdditionalInfo(kShouldPrefetchKey, kFalse);

  return match;
}

void SearchProvider::UpdateDone() {
  // We're done when the timer isn't running and there are no suggest queries
  // pending.
  done_ = !timer_.IsRunning() && !default_fetcher_ && !keyword_fetcher_ && !bangs_fetcher_;
}

std::string SearchProvider::GetSessionToken() {
  base::TimeTicks current_time(base::TimeTicks::Now());
  // Renew token if it expired.
  if (current_time > token_expiration_time_) {
    const size_t kTokenBytes = 12;
    std::string raw_data;
    base::RandBytes(base::WriteInto(&raw_data, kTokenBytes + 1), kTokenBytes);
    base::Base64Encode(raw_data, &current_token_);

    // Make the base64 encoded value URL and filename safe(see RFC 3548).
    std::replace(current_token_.begin(), current_token_.end(), '+', '-');
    std::replace(current_token_.begin(), current_token_.end(), '/', '_');
  }

  // Extend expiration time another 60 seconds.
  token_expiration_time_ = current_time + base::TimeDelta::FromSeconds(60);

  return current_token_;
}

AnswersQueryData SearchProvider::FindAnswersPrefetchData() {
  // Retrieve the top entry from scored history results.
  MatchMap map;
  AddTransformedHistoryResultsToMap(transformed_keyword_history_results_,
                                    TemplateURLRef::NO_SUGGESTIONS_AVAILABLE,
                                    &map);
  AddTransformedHistoryResultsToMap(transformed_default_history_results_,
                                    TemplateURLRef::NO_SUGGESTIONS_AVAILABLE,
                                    &map);

  ACMatches matches;
  for (MatchMap::const_iterator i(map.begin()); i != map.end(); ++i)
    matches.push_back(i->second);
  std::sort(matches.begin(), matches.end(), &AutocompleteMatch::MoreRelevant);

  // If there is a top scoring entry, find the corresponding answer.
  if (!matches.empty())
    return answers_cache_.GetTopAnswerEntry(matches[0].contents);

  return AnswersQueryData();
}
