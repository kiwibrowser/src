// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/data_usage/data_use_tab_model.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "chrome/browser/android/data_usage/data_use_matcher.h"
#include "chrome/browser/android/data_usage/external_data_use_observer.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/navigation_entry.h"
#include "url/gurl.h"

namespace {

// Default maximum number of tabs to maintain session information about. May be
// overridden by the field trial.
const size_t kDefaultMaxTabEntries = 200;

// Default maximum number of tracking session history to maintain per tab. May
// be overridden by the field trial.
const size_t kDefaultMaxSessionsPerTab = 5;

// Default expiration duration in seconds, after which a closed and an open tab
// entry can be removed from the list of tab entries, respectively. May be
// overridden by the field trial.
const uint32_t kDefaultClosedTabExpirationDurationSeconds = 30;  // 30 seconds.
const uint32_t kDefaultOpenTabExpirationDurationSeconds =
    60 * 60 * 24 * 5;  // 5 days.

// Default expiration duration in seconds of a matching rule, used when
// expiration time is not specified.
const uint32_t kDefaultMatchingRuleExpirationDurationSeconds =
    60 * 60 * 24;  // 24 hours.

const char kUMAExpiredInactiveTabEntryRemovalDurationHistogram[] =
    "DataUsage.TabModel.ExpiredInactiveTabEntryRemovalDuration";
const char kUMAExpiredActiveTabEntryRemovalDurationHistogram[] =
    "DataUsage.TabModel.ExpiredActiveTabEntryRemovalDuration";
const char kUMAUnexpiredTabEntryRemovalDurationHistogram[] =
    "DataUsage.TabModel.UnexpiredTabEntryRemovalDuration";

// Key used to save the data use tracking label in navigation entry extra data.
const char kDataUseTabModelLabel[] = "data_use_tab_model_label";

// Reason for starting or ending the tracking session. These enums must remain
// synchronized with the enum of the same name in
// metrics/histograms/histograms.xml. These values are written to logs.  New
// enum values can be added, but existing enums must never be renumbered or
// deleted and reused.
enum DataUsageTrackingSessionStartReason {
  START_REASON_CUSTOM_TAB_PACKAGE_MATCH = 0,
  START_REASON_OMNIBOX_SEARCH = 1,
  START_REASON_OMNIBOX_NAVIGATION = 2,
  START_REASON_BOOKMARK = 3,
  START_REASON_LINK = 4,
  START_REASON_RELOAD = 5,
  START_REASON_MAX = 6
};

enum DataUsageTrackingSessionEndReason {
  END_REASON_OMNIBOX_SEARCH = 0,
  END_REASON_OMNIBOX_NAVIGATION = 1,
  END_REASON_BOOKMARK = 2,
  END_REASON_HISTORY = 3,
  END_REASON_MAX = 4
};

// Returns various parameters from the values specified in the field trial.
size_t GetMaxTabEntries() {
  size_t max_tab_entries = kDefaultMaxTabEntries;
  std::string variation_value = variations::GetVariationParamValue(
      android::ExternalDataUseObserver::kExternalDataUseObserverFieldTrial,
      "max_tab_entries");
  if (!variation_value.empty() &&
      base::StringToSizeT(variation_value, &max_tab_entries)) {
    return max_tab_entries;
  }
  return kDefaultMaxTabEntries;
}

// Returns various parameters from the values specified in the field trial.
size_t GetMaxSessionsPerTab() {
  size_t max_sessions_per_tab = kDefaultMaxSessionsPerTab;
  std::string variation_value = variations::GetVariationParamValue(
      android::ExternalDataUseObserver::kExternalDataUseObserverFieldTrial,
      "max_sessions_per_tab");
  if (!variation_value.empty() &&
      base::StringToSizeT(variation_value, &max_sessions_per_tab)) {
    return max_sessions_per_tab;
  }
  return kDefaultMaxSessionsPerTab;
}

base::TimeDelta GetClosedTabExpirationDuration() {
  uint32_t duration_seconds = kDefaultClosedTabExpirationDurationSeconds;
  std::string variation_value = variations::GetVariationParamValue(
      android::ExternalDataUseObserver::kExternalDataUseObserverFieldTrial,
      "closed_tab_expiration_duration_seconds");
  if (!variation_value.empty() &&
      base::StringToUint(variation_value, &duration_seconds)) {
    return base::TimeDelta::FromSeconds(duration_seconds);
  }
  return base::TimeDelta::FromSeconds(
      kDefaultClosedTabExpirationDurationSeconds);
}

base::TimeDelta GetOpenTabExpirationDuration() {
  uint32_t duration_seconds = kDefaultOpenTabExpirationDurationSeconds;
  std::string variation_value = variations::GetVariationParamValue(
      android::ExternalDataUseObserver::kExternalDataUseObserverFieldTrial,
      "open_tab_expiration_duration_seconds");
  if (!variation_value.empty() &&
      base::StringToUint(variation_value, &duration_seconds)) {
    return base::TimeDelta::FromSeconds(duration_seconds);
  }
  return base::TimeDelta::FromSeconds(kDefaultOpenTabExpirationDurationSeconds);
}

base::TimeDelta GetDefaultMatchingRuleExpirationDuration() {
  uint32_t duration_seconds = kDefaultMatchingRuleExpirationDurationSeconds;
  std::string variation_value = variations::GetVariationParamValue(
      android::ExternalDataUseObserver::kExternalDataUseObserverFieldTrial,
      "default_matching_rule_expiration_duration_seconds");
  if (!variation_value.empty() &&
      base::StringToUint(variation_value, &duration_seconds)) {
    return base::TimeDelta::FromSeconds(duration_seconds);
  }
  return base::TimeDelta::FromSeconds(
      kDefaultMatchingRuleExpirationDurationSeconds);
}

void RecordStartTrackingMetrics(
    android::DataUseTabModel::TransitionType transition,
    bool is_custom_tab_package_match) {
  DataUsageTrackingSessionStartReason start_reason;
  switch (transition) {
    case android::DataUseTabModel::TRANSITION_OMNIBOX_SEARCH:
      start_reason = START_REASON_OMNIBOX_SEARCH;
      break;
    case android::DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION:
      start_reason = START_REASON_OMNIBOX_NAVIGATION;
      break;
    case android::DataUseTabModel::TRANSITION_BOOKMARK:
      start_reason = START_REASON_BOOKMARK;
      break;
    case android::DataUseTabModel::TRANSITION_LINK:
      start_reason = START_REASON_LINK;
      break;
    case android::DataUseTabModel::TRANSITION_RELOAD:
      start_reason = START_REASON_RELOAD;
      break;
    case android::DataUseTabModel::TRANSITION_CUSTOM_TAB:
      if (!is_custom_tab_package_match)
        return;
      start_reason = START_REASON_CUSTOM_TAB_PACKAGE_MATCH;
      break;
    case android::DataUseTabModel::TRANSITION_FORWARD_BACK:
      return;
    case android::DataUseTabModel::TRANSITION_HISTORY_ITEM:
    case android::DataUseTabModel::TRANSITION_FORM_SUBMIT:
      NOTREACHED();
      return;
  }
  UMA_HISTOGRAM_ENUMERATION("DataUsage.TrackingSessionStartReason",
                            start_reason, START_REASON_MAX);
}

void RecordEndTrackingMetrics(
    android::DataUseTabModel::TransitionType transition) {
  DataUsageTrackingSessionEndReason end_reason;
  switch (transition) {
    case android::DataUseTabModel::TRANSITION_OMNIBOX_SEARCH:
      end_reason = END_REASON_OMNIBOX_SEARCH;
      break;
    case android::DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION:
      end_reason = END_REASON_OMNIBOX_NAVIGATION;
      break;
    case android::DataUseTabModel::TRANSITION_BOOKMARK:
      end_reason = END_REASON_BOOKMARK;
      break;
    case android::DataUseTabModel::TRANSITION_HISTORY_ITEM:
      end_reason = END_REASON_HISTORY;
      break;
    case android::DataUseTabModel::TRANSITION_FORWARD_BACK:
      return;
    case android::DataUseTabModel::TRANSITION_CUSTOM_TAB:
    case android::DataUseTabModel::TRANSITION_FORM_SUBMIT:
    case android::DataUseTabModel::TRANSITION_LINK:
    case android::DataUseTabModel::TRANSITION_RELOAD:
      NOTREACHED();
      return;
  }
  UMA_HISTOGRAM_ENUMERATION("DataUsage.TrackingSessionEndReason", end_reason,
                            END_REASON_MAX);
}

}  // namespace

namespace android {

// static
const char DataUseTabModel::kDefaultTag[] = "ChromeTab";
const char DataUseTabModel::kCustomTabTag[] = "ChromeCustomTab";

DataUseTabModel::DataUseTabModel(
    const base::Closure& force_fetch_matching_rules_callback,
    const base::Callback<void(bool)>& on_matching_rules_fetched_callback)
    : max_tab_entries_(GetMaxTabEntries()),
      max_sessions_per_tab_(GetMaxSessionsPerTab()),
      closed_tab_expiration_duration_(GetClosedTabExpirationDuration()),
      open_tab_expiration_duration_(GetOpenTabExpirationDuration()),
      tick_clock_(base::DefaultTickClock::GetInstance()),
      force_fetch_matching_rules_callback_(force_fetch_matching_rules_callback),
      is_ready_for_navigation_event_(false),
      is_control_app_installed_(false),
      weak_factory_(this) {
  DCHECK(force_fetch_matching_rules_callback_);
  DCHECK(on_matching_rules_fetched_callback);
  data_use_matcher_.reset(new DataUseMatcher(
      base::Bind(&DataUseTabModel::OnTrackingLabelRemoved, GetWeakPtr()),
      on_matching_rules_fetched_callback,
      GetDefaultMatchingRuleExpirationDuration()));
  // Detach from current thread since rest of DataUseTabModel lives on the UI
  // thread and the current thread may not be UI thread..
  thread_checker_.DetachFromThread();
}

DataUseTabModel::~DataUseTabModel() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

base::WeakPtr<DataUseTabModel> DataUseTabModel::GetWeakPtr() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return weak_factory_.GetWeakPtr();
}

void DataUseTabModel::OnNavigationEvent(
    SessionID tab_id,
    TransitionType transition,
    const GURL& url,
    const std::string& package,
    content::NavigationEntry* navigation_entry) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(tab_id.is_valid());
  DCHECK(!navigation_entry || (navigation_entry->GetURL() == url));

  std::string current_label, new_label;
  bool is_package_match;

  if (is_control_app_installed_ && !data_use_matcher_->HasRules())
    force_fetch_matching_rules_callback_.Run();

  GetCurrentAndNewLabelForNavigationEvent(tab_id, transition, url, package,
                                          navigation_entry, &current_label,
                                          &new_label, &is_package_match);
  if (!current_label.empty() && new_label.empty()) {
    EndTrackingDataUse(transition, tab_id);
  } else if (current_label.empty() && !new_label.empty()) {
    StartTrackingDataUse(
        transition, tab_id, new_label,
        ((transition == TRANSITION_CUSTOM_TAB) && is_package_match));
  } else if (!current_label.empty() && current_label == new_label) {
    TabEntryMap::iterator tab_entry_iterator = active_tabs_.find(tab_id);
    if (tab_entry_iterator != active_tabs_.end())
      tab_entry_iterator->second.NotifyPageLoad();
  }
  if (navigation_entry && !new_label.empty()) {
    // Save the label to be used for back-forward navigations.
    navigation_entry->SetExtraData(kDataUseTabModelLabel,
                                   base::ASCIIToUTF16(new_label));
  }
}

void DataUseTabModel::OnTabCloseEvent(SessionID tab_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(tab_id.is_valid());

  TabEntryMap::iterator tab_entry_iterator = active_tabs_.find(tab_id);
  if (tab_entry_iterator == active_tabs_.end())
    return;

  TabDataUseEntry& tab_entry = tab_entry_iterator->second;
  if (tab_entry.IsTrackingDataUse())
    tab_entry.EndTracking();
  tab_entry.OnTabCloseEvent();
}

void DataUseTabModel::OnTrackingLabelRemoved(const std::string& label) {
  for (auto& tab_entry : active_tabs_)
    tab_entry.second.EndTrackingWithLabel(label);
}

bool DataUseTabModel::GetTrackingInfoForTabAtTime(
    SessionID tab_id,
    const base::TimeTicks timestamp,
    TrackingInfo* output_tracking_info) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  output_tracking_info->label = "";

  // Data use that cannot be attributed to a tab will not be labeled.
  if (!tab_id.is_valid())
    return false;

  TabEntryMap::const_iterator tab_entry_iterator = active_tabs_.find(tab_id);
  if (tab_entry_iterator != active_tabs_.end()) {
    bool is_available = tab_entry_iterator->second.GetLabel(
        timestamp, &output_tracking_info->label);
    if (is_available) {
      output_tracking_info->tag =
          tab_entry_iterator->second.is_custom_tab_package_match()
              ? DataUseTabModel::kCustomTabTag
              : DataUseTabModel::kDefaultTag;
    }
    return is_available;
  }

  return false;  // Tab session not found.
}

bool DataUseTabModel::WouldNavigationEventEndTracking(
    SessionID tab_id,
    TransitionType transition,
    const GURL& url,
    const content::NavigationEntry* navigation_entry) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(tab_id.is_valid());
  std::string current_label, new_label;
  bool is_package_match;
  GetCurrentAndNewLabelForNavigationEvent(
      tab_id, transition, url, std::string(), navigation_entry, &current_label,
      &new_label, &is_package_match);
  return (!current_label.empty() && new_label.empty());
}

void DataUseTabModel::AddObserver(TabDataUseObserver* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observers_.AddObserver(observer);
}

void DataUseTabModel::RemoveObserver(TabDataUseObserver* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observers_.RemoveObserver(observer);
}

void DataUseTabModel::RegisterURLRegexes(
    const std::vector<std::string>& app_package_name,
    const std::vector<std::string>& domain_path_regex,
    const std::vector<std::string>& label) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Fetch rule requests could be started when control app was installed, and
  // the response could be received when control app was uninstalled. Ignore
  // these spurious rule fetch responses.
  if (!is_control_app_installed_)
    return;
  data_use_matcher_->RegisterURLRegexes(app_package_name, domain_path_regex,
                                        label);
  if (!is_ready_for_navigation_event_) {
    is_ready_for_navigation_event_ = true;
    NotifyObserversOfDataUseTabModelReady();
  }
}

void DataUseTabModel::OnControlAppInstallStateChange(
    bool is_control_app_installed) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(data_use_matcher_);

  is_control_app_installed_ = is_control_app_installed;
  std::vector<std::string> empty;
  if (!is_control_app_installed_) {
    // Clear rules.
    data_use_matcher_->RegisterURLRegexes(empty, empty, empty);
    if (!is_ready_for_navigation_event_) {
      is_ready_for_navigation_event_ = true;
      NotifyObserversOfDataUseTabModelReady();
    }
  } else {
    // Fetch the matching rules when the app is installed.
    force_fetch_matching_rules_callback_.Run();
  }
}

base::TimeTicks DataUseTabModel::NowTicks() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return tick_clock_->NowTicks();
}

bool DataUseTabModel::IsCustomTabPackageMatch(SessionID tab_id) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  TabEntryMap::const_iterator tab_entry_iterator = active_tabs_.find(tab_id);
  return (tab_entry_iterator != active_tabs_.end()) &&
         tab_entry_iterator->second.is_custom_tab_package_match();
}

void DataUseTabModel::NotifyObserversOfTrackingStarting(SessionID tab_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  for (TabDataUseObserver& observer : observers_)
    observer.NotifyTrackingStarting(tab_id);
}

void DataUseTabModel::NotifyObserversOfTrackingEnding(SessionID tab_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  for (TabDataUseObserver& observer : observers_)
    observer.NotifyTrackingEnding(tab_id);
}

void DataUseTabModel::NotifyObserversOfDataUseTabModelReady() {
  DCHECK(thread_checker_.CalledOnValidThread());
  for (TabDataUseObserver& observer : observers_)
    observer.OnDataUseTabModelReady();
}

void DataUseTabModel::GetCurrentAndNewLabelForNavigationEvent(
    SessionID tab_id,
    TransitionType transition,
    const GURL& url,
    const std::string& package,
    const content::NavigationEntry* navigation_entry,
    std::string* current_label,
    std::string* new_label,
    bool* is_package_match) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(tab_id.is_valid());

  TabEntryMap::const_iterator tab_entry_iterator = active_tabs_.find(tab_id);
  *current_label =
      (tab_entry_iterator != active_tabs_.end())
          ? tab_entry_iterator->second.GetActiveTrackingSessionLabel()
          : std::string();
  bool matches;
  *new_label = "";
  *is_package_match = false;

  if (!current_label->empty() &&
      tab_entry_iterator->second.is_custom_tab_package_match()) {
    DCHECK_NE(transition, TRANSITION_OMNIBOX_SEARCH);
    DCHECK_NE(transition, TRANSITION_OMNIBOX_NAVIGATION);
    DCHECK_NE(transition, TRANSITION_HISTORY_ITEM);
    *new_label = *current_label;
    return;
  }

  switch (transition) {
    case TRANSITION_OMNIBOX_SEARCH:
    case TRANSITION_OMNIBOX_NAVIGATION:
    case TRANSITION_BOOKMARK:
      // Enter or exit events.
      if (!url.is_empty()) {
        matches = data_use_matcher_->MatchesURL(url, new_label);
        DCHECK_NE(matches, new_label->empty());
      }
      break;

    case TRANSITION_CUSTOM_TAB:
    case TRANSITION_LINK:
    case TRANSITION_RELOAD:
      // Enter events.
      if (!current_label->empty()) {
        *new_label = *current_label;
        break;
      }
      // Package name should match, for transitions from external app.
      if (transition == TRANSITION_CUSTOM_TAB && !package.empty()) {
        matches = data_use_matcher_->MatchesAppPackageName(package, new_label);
        DCHECK_NE(matches, new_label->empty());
        *is_package_match = matches;
      }
      if (new_label->empty() && !url.is_empty()) {
        matches = data_use_matcher_->MatchesURL(url, new_label);
        DCHECK_NE(matches, new_label->empty());
      }
      break;

    case TRANSITION_HISTORY_ITEM:
      // Exit events.
      DCHECK(new_label->empty());
      break;

    case TRANSITION_FORWARD_BACK:
      if (navigation_entry) {
        base::string16 navigation_entry_label;
        if (navigation_entry->GetExtraData(kDataUseTabModelLabel,
                                           &navigation_entry_label)) {
          std::string label = base::UTF16ToASCII(navigation_entry_label);
          DCHECK(!label.empty());
          if (data_use_matcher_->HasValidRuleWithLabel(label))
            *new_label = label;
        }
      }
      break;

    case TRANSITION_FORM_SUBMIT:
      // No change in the tracking state.
      *new_label = *current_label;
      break;

    default:
      NOTREACHED();
      break;
  }
}

void DataUseTabModel::StartTrackingDataUse(TransitionType transition,
                                           SessionID tab_id,
                                           const std::string& label,
                                           bool is_custom_tab_package_match) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // TODO(rajendrant): Explore ability to handle changes in label for current
  // session.
  bool new_tab_entry_added = false;
  TabEntryMap::iterator tab_entry_iterator = active_tabs_.find(tab_id);
  if (tab_entry_iterator == active_tabs_.end()) {
    auto new_entry = active_tabs_.insert(
        TabEntryMap::value_type(tab_id, TabDataUseEntry(this)));
    tab_entry_iterator = new_entry.first;
    DCHECK(tab_entry_iterator != active_tabs_.end());
    DCHECK(!tab_entry_iterator->second.IsTrackingDataUse());
    new_tab_entry_added = true;
  }
  if (tab_entry_iterator->second.StartTracking(label)) {
    tab_entry_iterator->second.set_custom_tab_package_match(
        is_custom_tab_package_match);
    tab_entry_iterator->second.NotifyPageLoad();
    RecordStartTrackingMetrics(transition, is_custom_tab_package_match);
    NotifyObserversOfTrackingStarting(tab_id);
  }

  if (new_tab_entry_added)
    CompactTabEntries();  // Keep total number of tab entries within limit.
}

void DataUseTabModel::EndTrackingDataUse(TransitionType transition,
                                         SessionID tab_id) {
  DCHECK(thread_checker_.CalledOnValidThread());

  TabEntryMap::iterator tab_entry_iterator = active_tabs_.find(tab_id);
  if (tab_entry_iterator != active_tabs_.end() &&
      tab_entry_iterator->second.EndTracking()) {
    NotifyObserversOfTrackingEnding(tab_id);
    RecordEndTrackingMetrics(transition);
  }
}

void DataUseTabModel::CompactTabEntries() {
  // Remove expired tab entries.
  for (TabEntryMap::iterator tab_entry_iterator = active_tabs_.begin();
       tab_entry_iterator != active_tabs_.end();) {
    const auto& tab_entry = tab_entry_iterator->second;
    if (tab_entry.IsExpired()) {
      // Track the lifetime of expired tab entry.
      const base::TimeDelta removal_time =
          NowTicks() - tab_entry.GetLatestStartOrEndTime();
      if (!tab_entry.IsTrackingDataUse()) {
        UMA_HISTOGRAM_CUSTOM_TIMES(
            kUMAExpiredInactiveTabEntryRemovalDurationHistogram, removal_time,
            base::TimeDelta::FromSeconds(1), base::TimeDelta::FromHours(1), 50);
      } else {
        UMA_HISTOGRAM_CUSTOM_TIMES(
            kUMAExpiredActiveTabEntryRemovalDurationHistogram, removal_time,
            base::TimeDelta::FromHours(1), base::TimeDelta::FromDays(5), 50);
      }
      active_tabs_.erase(tab_entry_iterator++);
    } else {
      ++tab_entry_iterator;
    }
  }

  if (active_tabs_.size() <= max_tab_entries_)
    return;

  // Remove oldest unexpired tab entries.
  while (active_tabs_.size() > max_tab_entries_) {
    // Find oldest tab entry.
    TabEntryMap::iterator oldest_tab_entry_iterator = active_tabs_.begin();
    for (TabEntryMap::iterator tab_entry_iterator = active_tabs_.begin();
         tab_entry_iterator != active_tabs_.end(); ++tab_entry_iterator) {
      if (oldest_tab_entry_iterator->second.GetLatestStartOrEndTime() >
          tab_entry_iterator->second.GetLatestStartOrEndTime()) {
        oldest_tab_entry_iterator = tab_entry_iterator;
      }
    }
    DCHECK(oldest_tab_entry_iterator != active_tabs_.end());
    UMA_HISTOGRAM_CUSTOM_TIMES(
        kUMAUnexpiredTabEntryRemovalDurationHistogram,
        NowTicks() -
            oldest_tab_entry_iterator->second.GetLatestStartOrEndTime(),
        base::TimeDelta::FromMinutes(1), base::TimeDelta::FromHours(1), 50);
    active_tabs_.erase(oldest_tab_entry_iterator);
  }
}

}  // namespace android
