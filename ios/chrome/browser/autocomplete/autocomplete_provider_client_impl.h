// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_PROVIDER_CLIENT_IMPL_H_
#define IOS_CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_PROVIDER_CLIENT_IMPL_H_

#include "base/macros.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "ios/chrome/browser/autocomplete/autocomplete_scheme_classifier_impl.h"
#include "ios/chrome/browser/search_engines/ui_thread_search_terms_data.h"

namespace ios {
class ChromeBrowserState;
}

// AutocompleteProviderClientImpl provides iOS-specific implementation of
// AutocompleteProviderClient interface.
class AutocompleteProviderClientImpl : public AutocompleteProviderClient {
 public:
  explicit AutocompleteProviderClientImpl(
      ios::ChromeBrowserState* browser_state);
  ~AutocompleteProviderClientImpl() override;

  // AutocompleteProviderClient implementation.
  net::URLRequestContextGetter* GetRequestContext() override;
  PrefService* GetPrefs() override;
  const AutocompleteSchemeClassifier& GetSchemeClassifier() const override;
  AutocompleteClassifier* GetAutocompleteClassifier() override;
  history::HistoryService* GetHistoryService() override;
  scoped_refptr<history::TopSites> GetTopSites() override;
  bookmarks::BookmarkModel* GetBookmarkModel() override;
  history::URLDatabase* GetInMemoryDatabase() override;
  InMemoryURLIndex* GetInMemoryURLIndex() override;
  TemplateURLService* GetTemplateURLService() override;
  const TemplateURLService* GetTemplateURLService() const override;
  ContextualSuggestionsService* GetContextualSuggestionsService(
      bool create_if_necessary) const override;
  const SearchTermsData& GetSearchTermsData() const override;
  scoped_refptr<ShortcutsBackend> GetShortcutsBackend() override;
  scoped_refptr<ShortcutsBackend> GetShortcutsBackendIfExists() override;
  std::unique_ptr<KeywordExtensionsDelegate> GetKeywordExtensionsDelegate(
      KeywordProvider* keyword_provider) override;
  std::string GetAcceptLanguages() const override;
  std::string GetEmbedderRepresentationOfAboutScheme() override;
  std::vector<base::string16> GetBuiltinURLs() override;
  std::vector<base::string16> GetBuiltinsToProvideAsUserTypes() override;
  // GetCurrentVisitTimestamp is only used by the contextual zero suggest
  // suggestions for desktop users. This implementation returns base::Time().
  base::Time GetCurrentVisitTimestamp() const override;
  bool IsOffTheRecord() const override;
  bool SearchSuggestEnabled() const override;
  bool IsTabUploadToGoogleActive() const override;
  bool IsAuthenticated() const override;
  void Classify(
      const base::string16& text,
      bool prefer_keyword,
      bool allow_exact_keyword_match,
      metrics::OmniboxEventProto::PageClassification page_classification,
      AutocompleteMatch* match,
      GURL* alternate_nav_url) override;
  void DeleteMatchingURLsForKeywordFromHistory(
      history::KeywordID keyword_id,
      const base::string16& term) override;
  void PrefetchImage(const GURL& url) override;
  void OnAutocompleteControllerResultReady(
      AutocompleteController* controller) override;
  bool IsTabOpenWithURL(const GURL& url,
                        const AutocompleteInput* input) override;

 private:
  ios::ChromeBrowserState* browser_state_;
  AutocompleteSchemeClassifierImpl scheme_classifier_;
  ios::UIThreadSearchTermsData search_terms_data_;

  DISALLOW_COPY_AND_ASSIGN(AutocompleteProviderClientImpl);
};

#endif  // IOS_CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_PROVIDER_CLIENT_IMPL_H_
