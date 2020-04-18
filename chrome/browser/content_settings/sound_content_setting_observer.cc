// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/content_settings/sound_content_setting_observer.h"

#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/ukm/content/source_url_recorder.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/url_constants.h"
#include "services/metrics/public/cpp/ukm_builders.h"

#if !defined(OS_ANDROID)
#include "chrome/browser/ui/tabs/tab_utils.h"
#endif

DEFINE_WEB_CONTENTS_USER_DATA_KEY(SoundContentSettingObserver);

SoundContentSettingObserver::SoundContentSettingObserver(
    content::WebContents* contents)
    : content::WebContentsObserver(contents),
      logged_site_muted_ukm_(false),
      observer_(this) {
  host_content_settings_map_ = HostContentSettingsMapFactory::GetForProfile(
      Profile::FromBrowserContext(web_contents()->GetBrowserContext()));
  observer_.Add(host_content_settings_map_);
}

SoundContentSettingObserver::~SoundContentSettingObserver() = default;

void SoundContentSettingObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame() && navigation_handle->HasCommitted() &&
      !navigation_handle->IsSameDocument()) {
    MuteOrUnmuteIfNecessary();
    logged_site_muted_ukm_ = false;
  }
}

void SoundContentSettingObserver::OnAudioStateChanged(bool audible) {
  CheckSoundBlocked(audible);
}

void SoundContentSettingObserver::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    std::string resource_identifier) {
  if (content_type == CONTENT_SETTINGS_TYPE_SOUND) {
    MuteOrUnmuteIfNecessary();
    CheckSoundBlocked(web_contents()->IsCurrentlyAudible());
  }
}

void SoundContentSettingObserver::MuteOrUnmuteIfNecessary() {
  bool mute = GetCurrentContentSetting() == CONTENT_SETTING_BLOCK;

// TabMutedReason does not exist on Android.
#if defined(OS_ANDROID)
  web_contents()->SetAudioMuted(mute);
#else
  // We don't want to overwrite TabMutedReason with no change.
  if (mute == web_contents()->IsAudioMuted())
    return;

  TabMutedReason reason = chrome::GetTabAudioMutedReason(web_contents());

  // Do not unmute if we're muted due to media capture.
  if (!mute && reason == TabMutedReason::MEDIA_CAPTURE)
    return;

  // Do not unmute if we're muted due to audio indicator.
  if (!mute && reason == TabMutedReason::AUDIO_INDICATOR)
    return;

  // Do not override the decisions of an extension.
  if (reason == TabMutedReason::EXTENSION)
    return;

  // Don't unmute a chrome:// URL if the tab has been explicitly muted on a
  // chrome:// URL.
  if (reason == TabMutedReason::CONTENT_SETTING_CHROME &&
      web_contents()->GetLastCommittedURL().SchemeIs(
          content::kChromeUIScheme)) {
    return;
  }

  chrome::SetTabAudioMuted(web_contents(), mute,
                           TabMutedReason::CONTENT_SETTING, std::string());
#endif  // defined(OS_ANDROID)
}

ContentSetting SoundContentSettingObserver::GetCurrentContentSetting() {
  GURL url = web_contents()->GetLastCommittedURL();
  return host_content_settings_map_->GetContentSetting(
      url, url, CONTENT_SETTINGS_TYPE_SOUND, std::string());
}

void SoundContentSettingObserver::CheckSoundBlocked(bool is_audible) {
  if (is_audible && GetCurrentContentSetting() == CONTENT_SETTING_BLOCK) {
    // The tab has tried to play sound, but was muted.
    TabSpecificContentSettings* settings =
        TabSpecificContentSettings::FromWebContents(web_contents());
    if (settings)
      settings->OnAudioBlocked();

    RecordSiteMutedUKM();
  }
}

void SoundContentSettingObserver::RecordSiteMutedUKM() {
  // We only want to log 1 event per navigation.
  if (logged_site_muted_ukm_)
    return;
  logged_site_muted_ukm_ = true;

  ukm::builders::Media_SiteMuted(
      ukm::GetSourceIdForWebContentsDocument(web_contents()))
      .SetMuteReason(GetSiteMutedReason())
      .Record(ukm::UkmRecorder::Get());
}

SoundContentSettingObserver::MuteReason
SoundContentSettingObserver::GetSiteMutedReason() {
  const GURL url = web_contents()->GetLastCommittedURL();
  content_settings::SettingInfo info;
  host_content_settings_map_->GetWebsiteSetting(
      url, url, CONTENT_SETTINGS_TYPE_SOUND, std::string(), &info);

  DCHECK_EQ(content_settings::SETTING_SOURCE_USER, info.source);

  if (info.primary_pattern == ContentSettingsPattern::Wildcard() &&
      info.secondary_pattern == ContentSettingsPattern::Wildcard()) {
    return MuteReason::kMuteByDefault;
  }
  return MuteReason::kSiteException;
}
