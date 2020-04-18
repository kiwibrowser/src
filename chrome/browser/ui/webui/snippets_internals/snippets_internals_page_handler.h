// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SNIPPETS_INTERNALS_SNIPPETS_INTERNALS_PAGE_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SNIPPETS_INTERNALS_SNIPPETS_INTERNALS_PAGE_HANDLER_H_

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/ui/webui/snippets_internals/snippets_internals.mojom.h"
#include "components/ntp_snippets/content_suggestions_service.h"
#include "components/ntp_snippets/remote/remote_suggestions_provider.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/binding.h"

// TODO: Write tests for this.
class SnippetsInternalsPageHandler
    : public snippets_internals::mojom::PageHandler,
      public ntp_snippets::ContentSuggestionsService::Observer {
 public:
  explicit SnippetsInternalsPageHandler(
      snippets_internals::mojom::PageHandlerRequest request,
      snippets_internals::mojom::PagePtr,
      ntp_snippets::ContentSuggestionsService* content_suggestions_service,
      PrefService* pref_service);
  ~SnippetsInternalsPageHandler() override;

  // snippets_internals::mojom::PageHandler
  void GetGeneralProperties(GetGeneralPropertiesCallback) override;
  void GetUserClassifierProperties(
      GetUserClassifierPropertiesCallback) override;
  void ClearUserClassifierProperties() override;
  void GetCategoryRankerProperties(
      GetCategoryRankerPropertiesCallback) override;
  void ReloadSuggestions() override;
  void GetDebugLog(GetDebugLogCallback) override;
  void ClearCachedSuggestions() override;
  void GetRemoteContentSuggestionsProperties(
      GetRemoteContentSuggestionsPropertiesCallback) override;
  void FetchSuggestionsInBackground(
      int64_t,
      FetchSuggestionsInBackgroundCallback) override;
  void IsPushingDummySuggestionPossible(
      IsPushingDummySuggestionPossibleCallback) override;
  void PushDummySuggestionInBackground(
      int64_t,
      PushDummySuggestionInBackgroundCallback) override;
  void GetLastJson(GetLastJsonCallback) override;
  void ResetNotificationState() override;
  void GetSuggestionsByCategory(GetSuggestionsByCategoryCallback) override;
  void ClearDismissedSuggestions(int64_t) override;

 private:
  // ntp_snippets::ContentSuggestionsService::Observer:
  void OnNewSuggestions(ntp_snippets::Category category) override;
  void OnCategoryStatusChanged(
      ntp_snippets::Category category,
      ntp_snippets::CategoryStatus new_status) override;
  void OnSuggestionInvalidated(
      const ntp_snippets::ContentSuggestion::ID& suggestion_id) override;
  void OnFullRefreshRequired() override;
  void ContentSuggestionsServiceShutdown() override;

  void FetchSuggestionsInBackgroundImpl(FetchSuggestionsInBackgroundCallback);
  void GetSuggestionsByCategoryImpl(GetSuggestionsByCategoryCallback);
  void PushDummySuggestionInBackgroundImpl(
      PushDummySuggestionInBackgroundCallback);

  // Misc. methods.
  void CollectDismissedSuggestions(
      int last_index,
      GetSuggestionsByCategoryCallback callback,
      std::vector<ntp_snippets::ContentSuggestion> suggestions);

  // Binding from the mojo interface to concrete impl.
  mojo::Binding<snippets_internals::mojom::PageHandler> binding_;

  // Observer to notify frontend of dirty data.
  ScopedObserver<ntp_snippets::ContentSuggestionsService,
                 ntp_snippets::ContentSuggestionsService::Observer>
      content_suggestions_service_observer_;

  // Services that provide the data & functionality.
  ntp_snippets::ContentSuggestionsService* content_suggestions_service_;
  ntp_snippets::RemoteSuggestionsProvider* remote_suggestions_provider_;
  PrefService* pref_service_;

  // Store dismissed suggestions in an instance variable during aggregation
  std::map<ntp_snippets::Category,
           std::vector<ntp_snippets::ContentSuggestion>,
           ntp_snippets::Category::CompareByID>
      dismissed_suggestions_;

  // Timers to delay actions.
  base::OneShotTimer suggestion_fetch_timer_;
  base::OneShotTimer suggestion_push_timer_;

  // Handle back to the page by which we can update.
  snippets_internals::mojom::PagePtr page_;

  base::WeakPtrFactory<SnippetsInternalsPageHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SnippetsInternalsPageHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SNIPPETS_INTERNALS_SNIPPETS_INTERNALS_PAGE_HANDLER_H_
