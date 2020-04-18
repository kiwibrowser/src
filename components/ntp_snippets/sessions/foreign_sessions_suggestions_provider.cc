// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/sessions/foreign_sessions_suggestions_provider.h"

#include <algorithm>
#include <map>
#include <tuple>
#include <utility>

#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/ntp_snippets/category.h"
#include "components/ntp_snippets/category_info.h"
#include "components/ntp_snippets/content_suggestion.h"
#include "components/ntp_snippets/features.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/ntp_snippets/pref_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/session_types.h"
#include "components/strings/grit/components_strings.h"
#include "components/sync_sessions/synced_session.h"
#include "components/variations/variations_associated_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

using base::Time;
using base::TimeDelta;
using sessions::SerializedNavigationEntry;
using sessions::SessionTab;
using sessions::SessionWindow;
using sync_sessions::SyncedSessionWindow;
using sync_sessions::SyncedSession;

using DismissedFilter = base::Callback<bool(const std::string& id)>;

namespace ntp_snippets {
namespace {

const int kMaxForeignTabsTotal = 10;
const int kMaxForeignTabsPerDevice = 3;
const int kMaxForeignTabAgeInMinutes = 180;

const char* kMaxForeignTabsTotalParamName = "max_foreign_tabs_total";
const char* kMaxForeignTabsPerDeviceParamName = "max_foreign_tabs_per_device";
const char* kMaxForeignTabAgeInMinutesParamName =
    "max_foreign_tabs_age_in_minutes";

int GetMaxForeignTabsTotal() {
  return variations::GetVariationParamByFeatureAsInt(
      ntp_snippets::kForeignSessionsSuggestionsFeature,
      kMaxForeignTabsTotalParamName, kMaxForeignTabsTotal);
}

int GetMaxForeignTabsPerDevice() {
  return variations::GetVariationParamByFeatureAsInt(
      ntp_snippets::kForeignSessionsSuggestionsFeature,
      kMaxForeignTabsPerDeviceParamName, kMaxForeignTabsPerDevice);
}

TimeDelta GetMaxForeignTabAge() {
  return TimeDelta::FromMinutes(variations::GetVariationParamByFeatureAsInt(
      ntp_snippets::kForeignSessionsSuggestionsFeature,
      kMaxForeignTabAgeInMinutesParamName, kMaxForeignTabAgeInMinutes));
}

// This filter does two things. Most importantly it lets through only ids that
// have not been dismissed. The other responsibility this class has is it tracks
// all of the ids that fly past it, and it will save the intersection of
// initially dismissed ids, and seen ids. This will aggressively prune any
// dismissal that is not currently blocking a recent tab.
class PrefsPruningDismissedItemFilter {
 public:
  explicit PrefsPruningDismissedItemFilter(PrefService* pref_service)
      : pref_service_(pref_service),
        initial_dismissed_ids_(prefs::ReadDismissedIDsFromPrefs(
            *pref_service_,
            prefs::kDismissedForeignSessionsSuggestions)) {}

  ~PrefsPruningDismissedItemFilter() {
    prefs::StoreDismissedIDsToPrefs(pref_service_,
                                    prefs::kDismissedForeignSessionsSuggestions,
                                    active_dismissed_ids_);
  }

  // Returns a Callback that can be easily used to filter out ids. Should not be
  // stored anywhere, the filter should always outlive the returned callback.
  DismissedFilter ToCallback() {
    return base::Bind(&PrefsPruningDismissedItemFilter::ShouldInclude,
                      base::Unretained(this));
  }

 private:
  bool ShouldInclude(const std::string& id) {
    if (initial_dismissed_ids_.find(id) == initial_dismissed_ids_.end()) {
      return true;
    }
    active_dismissed_ids_.insert(id);
    return false;
  }

  PrefService* pref_service_;

  // Ids that we know should be filterd out.
  std::set<std::string> initial_dismissed_ids_;

  // Ids that we have seen and were filtered out. This will be what is saved to
  // preferences upon our destructor.
  std::set<std::string> active_dismissed_ids_;

  DISALLOW_COPY_AND_ASSIGN(PrefsPruningDismissedItemFilter);
};

// This filter only lets through ids that should normally be filtered out. As
// such, this filter should only be used when purposely trying to view dismissed
// content.
class InverseDismissedItemFilter {
 public:
  explicit InverseDismissedItemFilter(PrefService* pref_service)
      : dismissed_ids_(prefs::ReadDismissedIDsFromPrefs(
            *pref_service,
            prefs::kDismissedForeignSessionsSuggestions)) {}

  // Returns a Callback that can be easily used to filter out ids. Should not be
  // stored anywhere, the filter should always outlive the returned callback.
  DismissedFilter ToCallback() {
    return base::Bind(&InverseDismissedItemFilter::ShouldInclude,
                      base::Unretained(this));
  }

 private:
  bool ShouldInclude(const std::string& id) {
    return dismissed_ids_.find(id) != dismissed_ids_.end();
  }

  std::set<std::string> dismissed_ids_;

  DISALLOW_COPY_AND_ASSIGN(InverseDismissedItemFilter);
};

}  // namespace

// Collection of pointers to various sessions objects that contain a superset of
// the information needed to create a single suggestion.
struct ForeignSessionsSuggestionsProvider::SessionData {
  const sync_sessions::SyncedSession* session;
  const sessions::SessionTab* tab;
  const sessions::SerializedNavigationEntry* navigation;
  bool operator<(const SessionData& other) const {
    // Note that SerializedNavigationEntry::timestamp() is never set to a
    // value, so always use SessionTab::timestamp() instead.
    // TODO(skym): It might be better if we sorted by recency of session, and
    // only then by recency of the tab. Right now this causes a single
    // device's tabs to be interleaved with another devices' tabs.
    return tab->timestamp > other.tab->timestamp;
  }
};

ForeignSessionsSuggestionsProvider::ForeignSessionsSuggestionsProvider(
    ContentSuggestionsProvider::Observer* observer,
    std::unique_ptr<ForeignSessionsProvider> foreign_sessions_provider,
    PrefService* pref_service)
    : ContentSuggestionsProvider(observer),
      category_status_(CategoryStatus::INITIALIZING),
      provided_category_(
          Category::FromKnownCategory(KnownCategories::FOREIGN_TABS)),
      foreign_sessions_provider_(std::move(foreign_sessions_provider)),
      pref_service_(pref_service) {
  foreign_sessions_provider_->SubscribeForForeignTabChange(
      base::Bind(&ForeignSessionsSuggestionsProvider::OnForeignTabChange,
                 base::Unretained(this)));

  // If sync is already initialzed, try suggesting now, though this is unlikely.
  OnForeignTabChange();
}

ForeignSessionsSuggestionsProvider::~ForeignSessionsSuggestionsProvider() =
    default;

// static
void ForeignSessionsSuggestionsProvider::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kDismissedForeignSessionsSuggestions);
}

CategoryStatus ForeignSessionsSuggestionsProvider::GetCategoryStatus(
    Category category) {
  DCHECK_EQ(category, provided_category_);
  return category_status_;
}

CategoryInfo ForeignSessionsSuggestionsProvider::GetCategoryInfo(
    Category category) {
  DCHECK_EQ(category, provided_category_);
  return CategoryInfo(l10n_util::GetStringUTF16(
                          IDS_NTP_FOREIGN_SESSIONS_SUGGESTIONS_SECTION_HEADER),
                      ContentSuggestionsCardLayout::MINIMAL_CARD,
                      ContentSuggestionsAdditionalAction::VIEW_ALL,
                      /*show_if_empty=*/false,
                      l10n_util::GetStringUTF16(
                          IDS_NTP_FOREIGN_SESSIONS_SUGGESTIONS_SECTION_EMPTY));
}

void ForeignSessionsSuggestionsProvider::DismissSuggestion(
    const ContentSuggestion::ID& suggestion_id) {
  // Assume this suggestion is still valid, and blindly add it to dismissals.
  // Pruning will happen the next time we are asked to suggest.
  std::set<std::string> dismissed_ids = prefs::ReadDismissedIDsFromPrefs(
      *pref_service_, prefs::kDismissedForeignSessionsSuggestions);
  dismissed_ids.insert(suggestion_id.id_within_category());
  prefs::StoreDismissedIDsToPrefs(pref_service_,
                                  prefs::kDismissedForeignSessionsSuggestions,
                                  dismissed_ids);
}

void ForeignSessionsSuggestionsProvider::FetchSuggestionImage(
    const ContentSuggestion::ID& suggestion_id,
    ImageFetchedCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), gfx::Image()));
}

void ForeignSessionsSuggestionsProvider::FetchSuggestionImageData(
    const ContentSuggestion::ID& suggestion_id,
    ImageDataFetchedCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::string()));
}

void ForeignSessionsSuggestionsProvider::Fetch(
    const Category& category,
    const std::set<std::string>& known_suggestion_ids,
    FetchDoneCallback callback) {
  LOG(DFATAL)
      << "ForeignSessionsSuggestionsProvider has no |Fetch| functionality!";
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback),
                                Status(StatusCode::PERMANENT_ERROR,
                                       "ForeignSessionsSuggestionsProvider "
                                       "has no |Fetch| functionality!"),
                                std::vector<ContentSuggestion>()));
}

void ForeignSessionsSuggestionsProvider::ClearHistory(
    Time begin,
    Time end,
    const base::Callback<bool(const GURL& url)>& filter) {
  std::set<std::string> dismissed_ids = prefs::ReadDismissedIDsFromPrefs(
      *pref_service_, prefs::kDismissedForeignSessionsSuggestions);
  for (auto iter = dismissed_ids.begin(); iter != dismissed_ids.end();) {
    if (filter.Run(GURL(*iter))) {
      iter = dismissed_ids.erase(iter);
    } else {
      ++iter;
    }
  }
  prefs::StoreDismissedIDsToPrefs(pref_service_,
                                  prefs::kDismissedForeignSessionsSuggestions,
                                  dismissed_ids);
}

void ForeignSessionsSuggestionsProvider::ClearCachedSuggestions() {
  // Ignored.
}

void ForeignSessionsSuggestionsProvider::GetDismissedSuggestionsForDebugging(
    Category category,
    DismissedSuggestionsCallback callback) {
  DCHECK_EQ(category, provided_category_);
  InverseDismissedItemFilter filter(pref_service_);
  // Use GetSuggestionCandidates instead of BuildSuggestions(), to avoid the
  // size and duplicate filtering. We want to return a complete list of
  // everything that could potentially be blocked by the not dismissed filter.
  std::vector<ContentSuggestion> suggestions;
  for (auto data : GetSuggestionCandidates(filter.ToCallback())) {
    suggestions.push_back(BuildSuggestion(data));
  }
  std::move(callback).Run(std::move(suggestions));
}

void ForeignSessionsSuggestionsProvider::ClearDismissedSuggestionsForDebugging(
    Category category) {
  DCHECK_EQ(category, provided_category_);
  pref_service_->ClearPref(prefs::kDismissedForeignSessionsSuggestions);
}

void ForeignSessionsSuggestionsProvider::OnForeignTabChange() {
  if (!foreign_sessions_provider_->HasSessionsData()) {
    if (category_status_ == CategoryStatus::AVAILABLE) {
      // This is to handle the case where the user disabled sync [sessions] or
      // logs out after we've already provided actual suggestions.
      category_status_ = CategoryStatus::NOT_PROVIDED;
      observer()->OnCategoryStatusChanged(this, provided_category_,
                                          category_status_);
    }
    return;
  }

  if (category_status_ != CategoryStatus::AVAILABLE) {
    // The further below logic will overwrite any error state. This is
    // currently okay because no where in the current implementation does the
    // status get set to an error state. Should this change, reconsider the
    // overwriting logic.
    DCHECK(category_status_ == CategoryStatus::INITIALIZING ||
           category_status_ == CategoryStatus::NOT_PROVIDED);

    // It is difficult to tell if sync simply has not initialized yet or there
    // will never be data because the user is signed out or has disabled the
    // sessions data type. Because this provider is hidden when there are no
    // results, always just update to AVAILABLE once we might have results.
    category_status_ = CategoryStatus::AVAILABLE;
    observer()->OnCategoryStatusChanged(this, provided_category_,
                                        category_status_);
  }

  // observer()->OnNewSuggestions must be called even when we have no
  // suggestions to remove previous suggestions that are now filtered out.
  observer()->OnNewSuggestions(this, provided_category_, BuildSuggestions());
}

std::vector<ContentSuggestion>
ForeignSessionsSuggestionsProvider::BuildSuggestions() {
  const int max_foreign_tabs_total = GetMaxForeignTabsTotal();
  const int max_foreign_tabs_per_device = GetMaxForeignTabsPerDevice();

  PrefsPruningDismissedItemFilter filter(pref_service_);
  std::vector<SessionData> suggestion_candidates =
      GetSuggestionCandidates(filter.ToCallback());
  // This sorts by recency so that we keep the most recent entries and they
  // appear as suggestions in reverse chronological order.
  std::sort(suggestion_candidates.begin(), suggestion_candidates.end());

  std::vector<ContentSuggestion> suggestions;
  std::set<std::string> included_urls;
  std::map<std::string, int> suggestions_per_session;
  for (const SessionData& candidate : suggestion_candidates) {
    const std::string& session_tag = candidate.session->session_tag;
    auto duplicates_iter =
        included_urls.find(candidate.navigation->virtual_url().spec());
    auto count_iter = suggestions_per_session.find(session_tag);
    int count =
        count_iter == suggestions_per_session.end() ? 0 : count_iter->second;

    // Pick up to max (total and per device) tabs, and ensure no duplicates
    // are selected. This filtering must be done in a second pass because
    // this can cause newer tabs occluding less recent tabs, requiring more
    // than |max_foreign_tabs_per_device| to be considered per device.
    if (static_cast<int>(suggestions.size()) >= max_foreign_tabs_total ||
        duplicates_iter != included_urls.end() ||
        count >= max_foreign_tabs_per_device) {
      continue;
    }
    included_urls.insert(candidate.navigation->virtual_url().spec());
    suggestions_per_session[session_tag] = count + 1;
    suggestions.push_back(BuildSuggestion(candidate));
  }

  return suggestions;
}

std::vector<ForeignSessionsSuggestionsProvider::SessionData>
ForeignSessionsSuggestionsProvider::GetSuggestionCandidates(
    const DismissedFilter& suggestions_filter) {
  const std::vector<const SyncedSession*>& foreign_sessions =
      foreign_sessions_provider_->GetAllForeignSessions();
  const TimeDelta max_foreign_tab_age = GetMaxForeignTabAge();
  std::vector<SessionData> suggestion_candidates;
  for (const SyncedSession* session : foreign_sessions) {
    for (const auto& key_value : session->windows) {
      for (const std::unique_ptr<SessionTab>& tab :
           key_value.second->wrapped_window.tabs) {
        if (tab->navigations.empty()) {
          continue;
        }

        const SerializedNavigationEntry& navigation = tab->navigations.back();
        const std::string id = navigation.virtual_url().spec();
        // TODO(skym): Filter out internal pages. Tabs that contain only
        // non-syncable content should never reach the local client. However,
        // sync will let tabs through whose current navigation entry is
        // internal, as long as a back or forward navigation entry is valid. We
        // however, are only currently exposing the current entry, and so we
        // should ideally exclude these.
        TimeDelta tab_age = Time::Now() - tab->timestamp;
        if (tab_age < max_foreign_tab_age && suggestions_filter.Run(id)) {
          suggestion_candidates.push_back(
              SessionData{session, tab.get(), &navigation});
        }
      }
    }
  }
  return suggestion_candidates;
}

ContentSuggestion ForeignSessionsSuggestionsProvider::BuildSuggestion(
    const SessionData& data) {
  ContentSuggestion suggestion(provided_category_,
                               data.navigation->virtual_url().spec(),
                               data.navigation->virtual_url());
  suggestion.set_title(data.navigation->title());
  suggestion.set_publish_date(data.tab->timestamp);
  suggestion.set_publisher_name(
      base::UTF8ToUTF16(data.navigation->virtual_url().host()));
  return suggestion;
}

}  // namespace ntp_snippets
