// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains the zero-suggest autocomplete provider. This experimental
// provider is invoked when the user focuses in the omnibox prior to editing,
// and generates search query suggestions based on the current URL.

#ifndef COMPONENTS_OMNIBOX_BROWSER_ZERO_SUGGEST_PROVIDER_H_
#define COMPONENTS_OMNIBOX_BROWSER_ZERO_SUGGEST_PROVIDER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/history/core/browser/history_types.h"
#include "components/omnibox/browser/base_search_provider.h"
#include "components/omnibox/browser/search_provider.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"

class AutocompleteProviderListener;
class HistoryURLProvider;

namespace base {
class Value;
}

namespace net {
class URLFetcher;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

// Autocomplete provider for searches based on the current URL.
//
// The controller will call Start() when the user focuses the omnibox. After
// construction, the autocomplete controller repeatedly calls Start() with some
// user input, each time expecting to receive an updated set of matches.
//
// TODO(jered): Consider deleting this class and building this functionality
// into SearchProvider after dogfood and after we break the association between
// omnibox text and suggestions.
class ZeroSuggestProvider : public BaseSearchProvider,
                            public net::URLFetcherDelegate {
 public:
  // Creates and returns an instance of this provider.
  static ZeroSuggestProvider* Create(AutocompleteProviderClient* client,
                                     HistoryURLProvider* history_url_provider,
                                     AutocompleteProviderListener* listener);

  // Registers a preference used to cache zero suggest results.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // AutocompleteProvider:
  void Start(const AutocompleteInput& input, bool minimal_changes) override;
  void Stop(bool clear_cached_results,
            bool due_to_user_inactivity) override;
  void DeleteMatch(const AutocompleteMatch& match) override;
  void AddProviderInfo(ProvidersInfo* provider_info) const override;

  // Sets |field_trial_triggered_| to false.
  void ResetSession() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(ZeroSuggestProviderTest,
                           TestStartWillStopForSomeInput);
  ZeroSuggestProvider(AutocompleteProviderClient* client,
                      HistoryURLProvider* history_url_provider,
                      AutocompleteProviderListener* listener);

  ~ZeroSuggestProvider() override;

  // ZeroSuggestProvider is processing one of the following type of results
  // at any time.
  enum ResultType {
    NONE,
    DEFAULT_SERP,          // The default search provider is queried for
                           // zero-suggest suggestions.
    DEFAULT_SERP_FOR_URL,  // The default search provider is queried for
                           // zero-suggest suggestions that are specific
                           // to the visited URL.
    MOST_VISITED
  };

  // BaseSearchProvider:
  const TemplateURL* GetTemplateURL(bool is_keyword) const override;
  const AutocompleteInput GetInput(bool is_keyword) const override;
  bool ShouldAppendExtraParams(
      const SearchSuggestionParser::SuggestResult& result) const override;
  void RecordDeletionResult(bool success) override;

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // The function updates |results_| with data parsed from |json_data|.
  //
  // * The update is not performed if |json_data| is invalid.
  // * When the provider is using cached results and |json_data| is non-empty,
  //   this function updates the cached results.
  // * When |results_| contains cached results, these are updated only if
  //   |json_cata| corresponds to an empty list. This is done to ensure that
  //   the display is cleared, as it may be showing cached results that should
  //   not be shown.
  //
  // The return value is true only when |results_| changed.
  bool UpdateResults(const std::string& json_data);

  // Adds AutocompleteMatches for each of the suggestions in |results| to
  // |map|.
  void AddSuggestResultsToMap(
      const SearchSuggestionParser::SuggestResults& results,
      MatchMap* map);

  // Returns an AutocompleteMatch for a navigational suggestion |navigation|.
  AutocompleteMatch NavigationToMatch(
      const SearchSuggestionParser::NavigationResult& navigation);

  // Converts the parsed results to a set of AutocompleteMatches and adds them
  // to |matches_|.  Also update the histograms for how many results were
  // received.
  void ConvertResultsToAutocompleteMatches();

  // Returns an AutocompleteMatch for the current URL. The match should be in
  // the top position so that pressing enter has the effect of reloading the
  // page.
  AutocompleteMatch MatchForCurrentURL();

  // When the user is in the Most Visited field trial, we ask the TopSites
  // service for the most visited URLs. It then calls back to this function to
  // return those |urls|.
  void OnMostVisitedUrlsAvailable(size_t request_num,
                                  const history::MostVisitedURLList& urls);

  // When the user is in the contextual omnibox suggestions field trial, we ask
  // the ContextualSuggestionsService for a fetcher to retrieve recommendations.
  // When the fetcher is ready, the contextual suggestion service then calls
  // back to this function with the |fetcher| to use for the request.
  void OnContextualSuggestionsFetcherAvailable(
      std::unique_ptr<net::URLFetcher> fetcher);

  // Whether zero suggest suggestions are allowed in the given context.
  bool AllowZeroSuggestSuggestions(const GURL& current_page_url) const;

  // Checks whether we have a set of zero suggest results cached, and if so
  // populates |matches_| with cached results.
  void MaybeUseCachedSuggestions();

  // Returns the type of results that should be generated for the current
  // context.
  // Logs UMA metrics. Should be called exactly once, on Start(), otherwise the
  // meaning of the data logged would change.
  ResultType TypeOfResultToRun(const GURL& current_url,
                               const GURL& suggest_url);

  // Used for efficiency when creating the verbatim match.  Can be null.
  HistoryURLProvider* history_url_provider_;

  AutocompleteProviderListener* listener_;

  // The result type that is currently being processed by provider.
  // When the provider is not running, the result type is set to NONE.
  ResultType result_type_running_;

  // For reconciling asynchronous requests for most visited URLs.
  size_t most_visited_request_num_ = 0;

  // The URL for which a suggestion fetch is pending.
  std::string current_query_;

  // The title of the page for which a suggestion fetch is pending.
  base::string16 current_title_;

  // The type of page the user is viewing (a search results page doing search
  // term replacement, an arbitrary URL, etc.).
  metrics::OmniboxEventProto::PageClassification current_page_classification_;

  // Copy of OmniboxEditModel::permanent_text_.
  base::string16 permanent_text_;

  // Fetcher used to retrieve results.
  std::unique_ptr<net::URLFetcher> fetcher_;

  // Suggestion for the current URL.
  AutocompleteMatch current_url_match_;

  // Contains suggest and navigation results as well as relevance parsed from
  // the response for the most recent zero suggest input URL.
  SearchSuggestionParser::Results results_;

  history::MostVisitedURLList most_visited_urls_;

  // For callbacks that may be run after destruction.
  base::WeakPtrFactory<ZeroSuggestProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ZeroSuggestProvider);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_ZERO_SUGGEST_PROVIDER_H_
