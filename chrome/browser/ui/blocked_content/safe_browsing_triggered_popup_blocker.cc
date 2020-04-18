// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/safe_browsing_triggered_popup_blocker.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/db/util.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/console_message_level.h"
#include "third_party/blink/public/web/web_triggering_event_info.h"

namespace {

const char kIgnoreSublistsParam[] = "ignore_sublists";

void LogAction(SafeBrowsingTriggeredPopupBlocker::Action action) {
  UMA_HISTOGRAM_ENUMERATION("ContentSettings.Popups.StrongBlockerActions",
                            action,
                            SafeBrowsingTriggeredPopupBlocker::Action::kCount);
}

}  // namespace

using safe_browsing::SubresourceFilterLevel;

const base::Feature kAbusiveExperienceEnforce{"AbusiveExperienceEnforce",
                                              base::FEATURE_ENABLED_BY_DEFAULT};

SafeBrowsingTriggeredPopupBlocker::PageData::PageData() = default;

SafeBrowsingTriggeredPopupBlocker::PageData::~PageData() {
  if (is_triggered_) {
    UMA_HISTOGRAM_COUNTS_100("ContentSettings.Popups.StrongBlocker.NumBlocked",
                             num_popups_blocked_);
  }
}

// static
void SafeBrowsingTriggeredPopupBlocker::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kAbusiveExperienceInterventionEnforce,
                                true /* default_value */);
}

// static
std::unique_ptr<SafeBrowsingTriggeredPopupBlocker>
SafeBrowsingTriggeredPopupBlocker::MaybeCreate(
    content::WebContents* web_contents) {
  if (!IsEnabled(web_contents))
    return nullptr;

  auto* observer_manager =
      subresource_filter::SubresourceFilterObserverManager::FromWebContents(
          web_contents);
  if (!observer_manager)
    return nullptr;
  return base::WrapUnique(
      new SafeBrowsingTriggeredPopupBlocker(web_contents, observer_manager));
}

SafeBrowsingTriggeredPopupBlocker::~SafeBrowsingTriggeredPopupBlocker() =
    default;

bool SafeBrowsingTriggeredPopupBlocker::ShouldApplyStrongPopupBlocker(
    const content::OpenURLParams* open_url_params) {
  LogAction(Action::kConsidered);
  if (!current_page_data_->is_triggered())
    return false;

  bool should_block = true;
  if (open_url_params) {
    should_block = open_url_params->triggering_event_info ==
                   blink::WebTriggeringEventInfo::kFromUntrustedEvent;
  }
  if (!IsEnabled(web_contents()))
    return false;

  if (should_block) {
    LogAction(Action::kBlocked);
    current_page_data_->inc_num_popups_blocked();
    web_contents()->GetMainFrame()->AddMessageToConsole(
        content::CONSOLE_MESSAGE_LEVEL_ERROR, kAbusiveEnforceMessage);
  }
  return should_block;
}

SafeBrowsingTriggeredPopupBlocker::SafeBrowsingTriggeredPopupBlocker(
    content::WebContents* web_contents,
    subresource_filter::SubresourceFilterObserverManager* observer_manager)
    : content::WebContentsObserver(web_contents),
      scoped_observer_(this),
      current_page_data_(std::make_unique<PageData>()),
      ignore_sublists_(
          base::GetFieldTrialParamByFeatureAsBool(kAbusiveExperienceEnforce,
                                                  kIgnoreSublistsParam,
                                                  false /* default_value */)) {
  DCHECK(observer_manager);
  scoped_observer_.Add(observer_manager);
}

void SafeBrowsingTriggeredPopupBlocker::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame())
    return;

  base::Optional<SubresourceFilterLevel> level;
  level_for_next_committed_navigation_.swap(level);

  // Only care about main frame navigations that commit.
  if (!navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  DCHECK(current_page_data_);
  current_page_data_ = std::make_unique<PageData>();
  if (navigation_handle->IsErrorPage())
    return;

  // Log a warning only if we've matched a warn-only safe browsing list.
  if (level == SubresourceFilterLevel::ENFORCE) {
    current_page_data_->set_is_triggered(true);
    LogAction(Action::kEnforcedSite);
  } else if (level == SubresourceFilterLevel::WARN) {
    web_contents()->GetMainFrame()->AddMessageToConsole(
        content::CONSOLE_MESSAGE_LEVEL_WARNING, kAbusiveWarnMessage);
    LogAction(Action::kWarningSite);
  }
  LogAction(Action::kNavigation);
}

// This method will always be called before the DidFinishNavigation associated
// with this handle.
void SafeBrowsingTriggeredPopupBlocker::OnSafeBrowsingCheckComplete(
    content::NavigationHandle* navigation_handle,
    safe_browsing::SBThreatType threat_type,
    const safe_browsing::ThreatMetadata& threat_metadata) {
  DCHECK(navigation_handle->IsInMainFrame());
  if (threat_type !=
      safe_browsing::SBThreatType::SB_THREAT_TYPE_SUBRESOURCE_FILTER)
    return;
  if (ignore_sublists_) {
    // No warning for ignore_sublists mode.
    level_for_next_committed_navigation_ = SubresourceFilterLevel::ENFORCE;
    return;
  }

  auto abusive = threat_metadata.subresource_filter_match.find(
      safe_browsing::SubresourceFilterType::ABUSIVE);
  if (abusive == threat_metadata.subresource_filter_match.end())
    return;
  level_for_next_committed_navigation_ = abusive->second;
}

void SafeBrowsingTriggeredPopupBlocker::OnSubresourceFilterGoingAway() {
  scoped_observer_.RemoveAll();
}

bool SafeBrowsingTriggeredPopupBlocker::IsEnabled(
    const content::WebContents* web_contents) {
  // If feature is disabled, return false. This is done so that if the feature
  // is broken it can be disabled irrespective of the policy.
  if (!base::FeatureList::IsEnabled(kAbusiveExperienceEnforce))
    return false;

  // If enterprise policy is not set, this will return true which is the default
  // preference value.
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  return profile->GetPrefs()->GetBoolean(
      prefs::kAbusiveExperienceInterventionEnforce);
}
