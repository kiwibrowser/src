// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_SESSIONS_FOREIGN_SESSIONS_SUGGESTIONS_PROVIDER_H_
#define COMPONENTS_NTP_SNIPPETS_SESSIONS_FOREIGN_SESSIONS_SUGGESTIONS_PROVIDER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "components/ntp_snippets/category.h"
#include "components/ntp_snippets/category_status.h"
#include "components/ntp_snippets/content_suggestions_provider.h"
#include "components/sessions/core/session_types.h"
#include "components/sync_sessions/synced_session.h"

class PrefRegistrySimple;
class PrefService;

namespace ntp_snippets {

// Simple interface to get foreign tab data on demand and on change.
class ForeignSessionsProvider {
 public:
  virtual ~ForeignSessionsProvider() = default;
  virtual bool HasSessionsData() = 0;
  virtual std::vector<const sync_sessions::SyncedSession*>
  GetAllForeignSessions() = 0;
  // Should only be called at most once.
  virtual void SubscribeForForeignTabChange(
      const base::Closure& change_callback) = 0;
};

// Provides content suggestions from foreign sessions.
class ForeignSessionsSuggestionsProvider : public ContentSuggestionsProvider {
 public:
  ForeignSessionsSuggestionsProvider(
      ContentSuggestionsProvider::Observer* observer,
      std::unique_ptr<ForeignSessionsProvider> foreign_sessions_provider,
      PrefService* pref_service);
  ~ForeignSessionsSuggestionsProvider() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class ForeignSessionsSuggestionsProviderTest;
  struct SessionData;

  // ContentSuggestionsProvider implementation.
  CategoryStatus GetCategoryStatus(Category category) override;
  CategoryInfo GetCategoryInfo(Category category) override;
  void DismissSuggestion(const ContentSuggestion::ID& suggestion_id) override;
  void FetchSuggestionImage(const ContentSuggestion::ID& suggestion_id,
                            ImageFetchedCallback callback) override;
  void FetchSuggestionImageData(const ContentSuggestion::ID& suggestion_id,
                                ImageDataFetchedCallback callback) override;
  void Fetch(const Category& category,
             const std::set<std::string>& known_suggestion_ids,
             FetchDoneCallback callback) override;
  void ClearHistory(
      base::Time begin,
      base::Time end,
      const base::Callback<bool(const GURL& url)>& filter) override;
  void ClearCachedSuggestions() override;
  void GetDismissedSuggestionsForDebugging(
      Category category,
      DismissedSuggestionsCallback callback) override;
  void ClearDismissedSuggestionsForDebugging(Category category) override;

  void OnForeignTabChange();
  std::vector<ContentSuggestion> BuildSuggestions();
  std::vector<SessionData> GetSuggestionCandidates(
      const base::Callback<bool(const std::string& id)>& suggestions_filter);
  ContentSuggestion BuildSuggestion(const SessionData& data);

  CategoryStatus category_status_;
  const Category provided_category_;
  std::unique_ptr<ForeignSessionsProvider> foreign_sessions_provider_;
  PrefService* pref_service_;

  DISALLOW_COPY_AND_ASSIGN(ForeignSessionsSuggestionsProvider);
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_SESSIONS_FOREIGN_SESSIONS_SUGGESTIONS_PROVIDER_H_
