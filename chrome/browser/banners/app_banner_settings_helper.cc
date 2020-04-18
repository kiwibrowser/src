// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/banners/app_banner_settings_helper.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/banners/app_banner_manager.h"
#include "chrome/browser/banners/app_banner_metrics.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/installable/installable_logging.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/rappor/public/rappor_utils.h"
#include "components/rappor/rappor_service_impl.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace {

// Max number of apps (including ServiceWorker based web apps) that a particular
// site may show a banner for.
const size_t kMaxAppsPerSite = 3;

// Default number of days that dismissing or ignoring the banner will prevent it
// being seen again for.
const unsigned int kMinimumBannerBlockedToBannerShown = 90;
const unsigned int kMinimumDaysBetweenBannerShows = 14;

// Default site engagement required to trigger the banner.
const unsigned int kDefaultTotalEngagementToTrigger = 2;

// The number of days in the past that a site should be launched from homescreen
// to be considered recent.
// TODO(dominickn): work out how to unify this with
// WebappDataStorage.wasLaunchedRecently.
const unsigned int kRecentLastLaunchInDays = 10;

// Dictionary keys to use for the events. Must be kept in sync with
// AppBannerEvent.
constexpr const char* kBannerEventKeys[] = {
    "couldShowBannerEvents",
    "didShowBannerEvent",
    "didBlockBannerEvent",
    "didAddToHomescreenEvent",
};

// Keys to use when querying the variations params.
const char kBannerParamsKey[] = "AppBannerTriggering";
const char kBannerParamsEngagementTotalKey[] = "site_engagement_total";
const char kBannerParamsDaysAfterBannerDismissedKey[] = "days_after_dismiss";
const char kBannerParamsDaysAfterBannerIgnoredKey[] = "days_after_ignore";
const char kBannerParamsLanguageKey[] = "language_option";

// Total engagement score required before a banner will actually be triggered.
double gTotalEngagementToTrigger = kDefaultTotalEngagementToTrigger;

unsigned int gDaysAfterDismissedToShow = kMinimumBannerBlockedToBannerShown;
unsigned int gDaysAfterIgnoredToShow = kMinimumDaysBetweenBannerShows;

std::unique_ptr<base::DictionaryValue> GetOriginAppBannerData(
    HostContentSettingsMap* settings,
    const GURL& origin_url) {
  if (!settings)
    return std::make_unique<base::DictionaryValue>();

  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(settings->GetWebsiteSetting(
          origin_url, origin_url, CONTENT_SETTINGS_TYPE_APP_BANNER,
          std::string(), NULL));
  if (!dict)
    return std::make_unique<base::DictionaryValue>();

  return dict;
}

base::Value* GetAppDict(base::DictionaryValue* origin_dict,
                        const std::string& key_name) {
  base::Value* app_dict =
      origin_dict->FindKeyOfType(key_name, base::Value::Type::DICTIONARY);
  if (!app_dict) {
    // Don't allow more than kMaxAppsPerSite dictionaries.
    if (origin_dict->size() < kMaxAppsPerSite) {
      app_dict = origin_dict->SetKey(
          key_name, base::Value(base::Value::Type::DICTIONARY));
    }
  }

  return app_dict;
}

// Queries variations for the number of days which dismissing and ignoring the
// banner should prevent a banner from showing.
void UpdateDaysBetweenShowing() {
  std::string dismiss_param = variations::GetVariationParamValue(
      kBannerParamsKey, kBannerParamsDaysAfterBannerDismissedKey);
  std::string ignore_param = variations::GetVariationParamValue(
      kBannerParamsKey, kBannerParamsDaysAfterBannerIgnoredKey);

  if (!dismiss_param.empty() && !ignore_param.empty()) {
    unsigned int dismiss_days = 0;
    unsigned int ignore_days = 0;

    if (base::StringToUint(dismiss_param, &dismiss_days) &&
        base::StringToUint(ignore_param, &ignore_days)) {
      AppBannerSettingsHelper::SetDaysAfterDismissAndIgnoreToTrigger(
          dismiss_days, ignore_days);
    }
  }
}

// Queries variations for the maximum site engagement score required to trigger
// the banner showing.
void UpdateSiteEngagementToTrigger() {
  std::string total_param = variations::GetVariationParamValue(
      kBannerParamsKey, kBannerParamsEngagementTotalKey);

  if (!total_param.empty()) {
    double total_engagement = -1;

    if (base::StringToDouble(total_param, &total_engagement) &&
        total_engagement >= 0) {
      AppBannerSettingsHelper::SetTotalEngagementToTrigger(total_engagement);
    }
  }
}

// Reports whether |event| was recorded within the |period| up until |now|.
bool WasEventWithinPeriod(AppBannerSettingsHelper::AppBannerEvent event,
                          base::TimeDelta period,
                          content::WebContents* web_contents,
                          const GURL& origin_url,
                          const std::string& package_name_or_start_url,
                          base::Time now) {
  base::Time event_time = AppBannerSettingsHelper::GetSingleBannerEvent(
      web_contents, origin_url, package_name_or_start_url, event);

  // Null times are in the distant past, so the delta between real times and
  // null events will always be greater than the limits.
  return (now - event_time < period);
}

}  // namespace

// Key to store instant apps events.
const char AppBannerSettingsHelper::kInstantAppsKey[] = "instantapps";

void AppBannerSettingsHelper::ClearHistoryForURLs(
    Profile* profile,
    const std::set<GURL>& origin_urls) {
  HostContentSettingsMap* settings =
      HostContentSettingsMapFactory::GetForProfile(profile);
  for (const GURL& origin_url : origin_urls) {
    settings->SetWebsiteSettingDefaultScope(origin_url, GURL(),
                                            CONTENT_SETTINGS_TYPE_APP_BANNER,
                                            std::string(), nullptr);
    settings->FlushLossyWebsiteSettings();
  }
}

void AppBannerSettingsHelper::RecordBannerInstallEvent(
    content::WebContents* web_contents,
    const std::string& package_name_or_start_url,
    AppBannerRapporMetric rappor_metric) {
  banners::TrackInstallEvent(banners::INSTALL_EVENT_WEB_APP_INSTALLED);

  AppBannerSettingsHelper::RecordBannerEvent(
      web_contents, web_contents->GetLastCommittedURL(),
      package_name_or_start_url,
      AppBannerSettingsHelper::APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN,
      banners::AppBannerManager::GetCurrentTime());

  rappor::SampleDomainAndRegistryFromGURL(
      g_browser_process->rappor_service(),
      (rappor_metric == WEB ? "AppBanner.WebApp.Installed"
                            : "AppBanner.NativeApp.Installed"),
      web_contents->GetLastCommittedURL());
}

void AppBannerSettingsHelper::RecordBannerDismissEvent(
    content::WebContents* web_contents,
    const std::string& package_name_or_start_url,
    AppBannerRapporMetric rappor_metric) {
  banners::TrackDismissEvent(banners::DISMISS_EVENT_CLOSE_BUTTON);

  AppBannerSettingsHelper::RecordBannerEvent(
      web_contents, web_contents->GetLastCommittedURL(),
      package_name_or_start_url,
      AppBannerSettingsHelper::APP_BANNER_EVENT_DID_BLOCK,
      banners::AppBannerManager::GetCurrentTime());

  rappor::SampleDomainAndRegistryFromGURL(
      g_browser_process->rappor_service(),
      (rappor_metric == WEB ? "AppBanner.WebApp.Dismissed"
                            : "AppBanner.NativeApp.Dismissed"),
      web_contents->GetLastCommittedURL());
}

void AppBannerSettingsHelper::RecordBannerEvent(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url,
    AppBannerEvent event,
    base::Time time) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (profile->IsOffTheRecord() || package_name_or_start_url.empty())
    return;

  HostContentSettingsMap* settings =
      HostContentSettingsMapFactory::GetForProfile(profile);
  std::unique_ptr<base::DictionaryValue> origin_dict =
      GetOriginAppBannerData(settings, origin_url);
  if (!origin_dict)
    return;

  base::Value* app_dict =
      GetAppDict(origin_dict.get(), package_name_or_start_url);
  if (!app_dict)
    return;

  // Dates are stored in their raw form (i.e. not local dates) to be resilient
  // to time zone changes.
  const char* event_key = kBannerEventKeys[event];

  if (event == APP_BANNER_EVENT_COULD_SHOW) {
    // Do not overwrite a could show event, as this is used for metrics.
    if (app_dict->FindKeyOfType(event_key, base::Value::Type::DOUBLE))
      return;
  }
  app_dict->SetKey(event_key,
                   base::Value(static_cast<double>(time.ToInternalValue())));

  settings->SetWebsiteSettingDefaultScope(
      origin_url, GURL(), CONTENT_SETTINGS_TYPE_APP_BANNER, std::string(),
      std::move(origin_dict));

  // App banner content settings are lossy, meaning they will not cause the
  // prefs to become dirty. This is fine for most events, as if they are lost it
  // just means the user will have to engage a little bit more. However the
  // DID_ADD_TO_HOMESCREEN event should always be recorded to prevent
  // spamminess.
  if (event == APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN)
    settings->FlushLossyWebsiteSettings();
}

bool AppBannerSettingsHelper::HasBeenInstalled(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url) {
  base::Time added_time =
      GetSingleBannerEvent(web_contents, origin_url, package_name_or_start_url,
                           APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN);

  return !added_time.is_null();
}

bool AppBannerSettingsHelper::WasBannerRecentlyBlocked(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url,
    base::Time now) {
  DCHECK(!package_name_or_start_url.empty());

  return WasEventWithinPeriod(
      APP_BANNER_EVENT_DID_BLOCK,
      base::TimeDelta::FromDays(gDaysAfterDismissedToShow), web_contents,
      origin_url, package_name_or_start_url, now);
}

bool AppBannerSettingsHelper::WasBannerRecentlyIgnored(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url,
    base::Time now) {
  DCHECK(!package_name_or_start_url.empty());

  return WasEventWithinPeriod(
      APP_BANNER_EVENT_DID_SHOW,
      base::TimeDelta::FromDays(gDaysAfterIgnoredToShow), web_contents,
      origin_url, package_name_or_start_url, now);
}

base::Time AppBannerSettingsHelper::GetSingleBannerEvent(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url,
    AppBannerEvent event) {
  DCHECK(event < APP_BANNER_EVENT_NUM_EVENTS);

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  HostContentSettingsMap* settings =
      HostContentSettingsMapFactory::GetForProfile(profile);
  std::unique_ptr<base::DictionaryValue> origin_dict =
      GetOriginAppBannerData(settings, origin_url);

  if (!origin_dict)
    return base::Time();

  base::Value* app_dict =
      GetAppDict(origin_dict.get(), package_name_or_start_url);
  if (!app_dict)
    return base::Time();

  base::Value* internal_time = app_dict->FindKeyOfType(
      kBannerEventKeys[event], base::Value::Type::DOUBLE);
  if (!internal_time)
    return base::Time();

  return base::Time::FromInternalValue(internal_time->GetDouble());
}

bool AppBannerSettingsHelper::HasSufficientEngagement(double total_engagement) {
  return (base::CommandLine::ForCurrentProcess()->HasSwitch(
             switches::kBypassAppBannerEngagementChecks)) ||
         (total_engagement >= gTotalEngagementToTrigger);
}

void AppBannerSettingsHelper::RecordMinutesFromFirstVisitToShow(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& package_name_or_start_url,
    base::Time time) {
  base::Time could_show_time =
      GetSingleBannerEvent(web_contents, origin_url, package_name_or_start_url,
                           APP_BANNER_EVENT_COULD_SHOW);

  int minutes = 0;
  if (!could_show_time.is_null())
    minutes = (time - could_show_time).InMinutes();

  banners::TrackMinutesFromFirstVisitToBannerShown(minutes);
}

bool AppBannerSettingsHelper::WasLaunchedRecently(Profile* profile,
                                                  const GURL& origin_url,
                                                  base::Time now) {
  HostContentSettingsMap* settings =
      HostContentSettingsMapFactory::GetForProfile(profile);
  std::unique_ptr<base::DictionaryValue> origin_dict =
      GetOriginAppBannerData(settings, origin_url);

  if (!origin_dict)
    return false;

  // Iterate over everything in the content setting, which should be a set of
  // dictionaries per app path. If we find one that has been added to
  // homescreen recently, return true.
  base::TimeDelta recent_last_launch_in_days =
      base::TimeDelta::FromDays(kRecentLastLaunchInDays);
  for (base::DictionaryValue::Iterator it(*origin_dict); !it.IsAtEnd();
       it.Advance()) {
    if (it.value().is_dict()) {
      const base::DictionaryValue* value;
      it.value().GetAsDictionary(&value);

      double internal_time;
      if (it.key() == kInstantAppsKey ||
          !value->GetDouble(
              kBannerEventKeys[APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN],
              &internal_time)) {
        continue;
      }

      if ((now - base::Time::FromInternalValue(internal_time)) <=
          recent_last_launch_in_days) {
        return true;
      }
    }
  }

  return false;
}

void AppBannerSettingsHelper::SetDaysAfterDismissAndIgnoreToTrigger(
    unsigned int dismiss_days,
    unsigned int ignore_days) {
  gDaysAfterDismissedToShow = dismiss_days;
  gDaysAfterIgnoredToShow = ignore_days;
}

void AppBannerSettingsHelper::SetTotalEngagementToTrigger(
    double total_engagement) {
  gTotalEngagementToTrigger = total_engagement;
}

void AppBannerSettingsHelper::SetDefaultParameters() {
  SetTotalEngagementToTrigger(kDefaultTotalEngagementToTrigger);
}

void AppBannerSettingsHelper::UpdateFromFieldTrial() {
  // If we are using the site engagement score, only extract the total
  // engagement to trigger from the params variations.
  UpdateDaysBetweenShowing();
  UpdateSiteEngagementToTrigger();
}

AppBannerSettingsHelper::LanguageOption
AppBannerSettingsHelper::GetHomescreenLanguageOption() {
  std::string param = variations::GetVariationParamValue(
      kBannerParamsKey, kBannerParamsLanguageKey);
  unsigned int language_option = 0;

  if (param.empty() || !base::StringToUint(param, &language_option) ||
      language_option < LANGUAGE_OPTION_MIN ||
      language_option > LANGUAGE_OPTION_MAX) {
    return LANGUAGE_OPTION_DEFAULT;
  }

  return static_cast<LanguageOption>(language_option);
}

AppBannerSettingsHelper::ScopedTriggerSettings::ScopedTriggerSettings(
    unsigned int dismiss_days,
    unsigned int ignore_days) {
  old_dismiss_ = gDaysAfterDismissedToShow;
  old_ignore_ = gDaysAfterIgnoredToShow;
  gDaysAfterDismissedToShow = dismiss_days;
  gDaysAfterIgnoredToShow = ignore_days;
}

AppBannerSettingsHelper::ScopedTriggerSettings::~ScopedTriggerSettings() {
  gDaysAfterDismissedToShow = old_dismiss_;
  gDaysAfterIgnoredToShow = old_ignore_;
}
