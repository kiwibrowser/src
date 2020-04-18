// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/pref_metrics_service.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/prefs/pref_service_syncable_util.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/tabs/pinned_tab_codec.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/rappor/public/rappor_utils.h"
#include "components/rappor/rappor_service_impl.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/sync_preferences/synced_pref_change_registrar.h"
#include "content/public/browser/browser_url_handler.h"
#include "crypto/hmac.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace {

const int kSessionStartupPrefValueMax = SessionStartupPref::kPrefValueMax;

#if !defined(OS_ANDROID)
// Record a sample for the Settings.NewTabPage rappor metric.
void SampleNewTabPageURL(Profile* profile) {
  GURL ntp_url(chrome::kChromeUINewTabURL);
  bool reverse_on_redirect = false;
  content::BrowserURLHandler::GetInstance()->RewriteURLIfNecessary(
      &ntp_url,
      profile,
      &reverse_on_redirect);
  if (ntp_url.is_valid()) {
    rappor::SampleDomainAndRegistryFromGURL(g_browser_process->rappor_service(),
                                            "Settings.NewTabPage", ntp_url);
  }
}
#endif

}  // namespace

PrefMetricsService::PrefMetricsService(Profile* profile)
    : profile_(profile),
      prefs_(profile_->GetPrefs()),
      local_state_(g_browser_process->local_state()),
      weak_factory_(this) {
  RecordLaunchPrefs();

  sync_preferences::PrefServiceSyncable* prefs =
      PrefServiceSyncableFromProfile(profile_);
  synced_pref_change_registrar_.reset(
      new sync_preferences::SyncedPrefChangeRegistrar(prefs));

  RegisterSyncedPrefObservers();
}

// For unit testing only.
PrefMetricsService::PrefMetricsService(Profile* profile,
                                       PrefService* local_state)
    : profile_(profile),
      prefs_(profile->GetPrefs()),
      local_state_(local_state),
      weak_factory_(this) {
}

PrefMetricsService::~PrefMetricsService() {
}

// static
void PrefMetricsService::RecordHomePageLaunchMetrics(bool show_home_button,
                                                     bool homepage_is_ntp,
                                                     const GURL& homepage_url) {
  UMA_HISTOGRAM_BOOLEAN("Settings.ShowHomeButton", show_home_button);
  if (show_home_button) {
    UMA_HISTOGRAM_BOOLEAN("Settings.GivenShowHomeButton_HomePageIsNewTabPage",
                          homepage_is_ntp);
  }

  // For non-NTP homepages, see if the URL comes from the same TLD+1 as a known
  // search engine.  Note that this is only an approximation of search engine
  // use, due to both false negatives (pages that come from unknown TLD+1 X but
  // consist of a search box that sends to known TLD+1 Y) and false positives
  // (pages that share a TLD+1 with a known engine but aren't actually search
  // pages, e.g. plus.google.com).  Additionally, record the TLD+1 of non-NTP
  // homepages through the privacy-preserving Rappor service.
  if (!homepage_is_ntp) {
    if (homepage_url.is_valid()) {
      UMA_HISTOGRAM_ENUMERATION(
          "Settings.HomePageEngineType",
          TemplateURLPrepopulateData::GetEngineType(homepage_url),
          SEARCH_ENGINE_MAX);
      rappor::SampleDomainAndRegistryFromGURL(
          g_browser_process->rappor_service(), "Settings.HomePage2",
          homepage_url);
    }
  }
}

void PrefMetricsService::RecordLaunchPrefs() {
  // On Android, determining whether the homepage is enabled requires waiting
  // for a response from a third party provider installed on the device.  So,
  // it will be logged later once all the dependent information is available.
  // See DeferredStartupHandler.java.
#if !defined(OS_ANDROID)
  GURL homepage_url(prefs_->GetString(prefs::kHomePage));
  RecordHomePageLaunchMetrics(prefs_->GetBoolean(prefs::kShowHomeButton),
                              prefs_->GetBoolean(prefs::kHomePageIsNewTabPage),
                              homepage_url);
#endif

  // Android does not support overriding the NTP URL.
#if !defined(OS_ANDROID)
  SampleNewTabPageURL(profile_);
#endif

  // Tab restoring is always done on Android, so these metrics are not
  // applicable.  Also, startup pages are not supported on Android
#if !defined(OS_ANDROID)
  int restore_on_startup = prefs_->GetInteger(prefs::kRestoreOnStartup);
  UMA_HISTOGRAM_ENUMERATION("Settings.StartupPageLoadSettings",
                            restore_on_startup, kSessionStartupPrefValueMax);
  if (restore_on_startup == SessionStartupPref::kPrefValueURLs) {
    const base::ListValue* url_list =
        prefs_->GetList(prefs::kURLsToRestoreOnStartup);
    UMA_HISTOGRAM_CUSTOM_COUNTS("Settings.StartupPageLoadURLs",
                                url_list->GetSize(), 1, 50, 20);
    // Similarly, check startup pages for known search engine TLD+1s.
    std::string url_text;
    for (size_t i = 0; i < url_list->GetSize(); ++i) {
      if (url_list->GetString(i, &url_text)) {
        GURL start_url(url_text);
        if (start_url.is_valid()) {
          UMA_HISTOGRAM_ENUMERATION(
              "Settings.StartupPageEngineTypes",
              TemplateURLPrepopulateData::GetEngineType(start_url),
              SEARCH_ENGINE_MAX);
          if (i == 0) {
            rappor::SampleDomainAndRegistryFromGURL(
                g_browser_process->rappor_service(),
                "Settings.FirstStartupPage", start_url);
          }
        }
      }
    }
  }
#endif

  // Android does not support pinned tabs.
#if !defined(OS_ANDROID)
  StartupTabs startup_tabs = PinnedTabCodec::ReadPinnedTabs(profile_);
  UMA_HISTOGRAM_CUSTOM_COUNTS("Settings.PinnedTabs",
                              startup_tabs.size(), 1, 50, 20);
  for (size_t i = 0; i < startup_tabs.size(); ++i) {
    GURL start_url(startup_tabs.at(i).url);
    if (start_url.is_valid()) {
      UMA_HISTOGRAM_ENUMERATION(
          "Settings.PinnedTabEngineTypes",
          TemplateURLPrepopulateData::GetEngineType(start_url),
          SEARCH_ENGINE_MAX);
    }
  }
#endif
}

void PrefMetricsService::RegisterSyncedPrefObservers() {
  LogHistogramValueCallback booleanHandler = base::Bind(
      &PrefMetricsService::LogBooleanPrefChange, base::Unretained(this));

  AddPrefObserver(prefs::kShowHomeButton, "ShowHomeButton", booleanHandler);
  AddPrefObserver(prefs::kHomePageIsNewTabPage, "HomePageIsNewTabPage",
                  booleanHandler);

  AddPrefObserver(prefs::kRestoreOnStartup, "StartupPageLoadSettings",
                  base::Bind(&PrefMetricsService::LogIntegerPrefChange,
                             base::Unretained(this),
                             kSessionStartupPrefValueMax));
}

void PrefMetricsService::AddPrefObserver(
    const std::string& path,
    const std::string& histogram_name_prefix,
    const LogHistogramValueCallback& callback) {
  synced_pref_change_registrar_->Add(path.c_str(),
      base::Bind(&PrefMetricsService::OnPrefChanged,
                 base::Unretained(this),
                 histogram_name_prefix, callback));
}

void PrefMetricsService::OnPrefChanged(
    const std::string& histogram_name_prefix,
    const LogHistogramValueCallback& callback,
    const std::string& path,
    bool from_sync) {
  sync_preferences::PrefServiceSyncable* prefs =
      PrefServiceSyncableFromProfile(profile_);
  const PrefService::Preference* pref = prefs->FindPreference(path);
  DCHECK(pref);
  std::string source_name(
      from_sync ? ".PulledFromSync" : ".PushedToSync");
  std::string histogram_name("Settings." + histogram_name_prefix + source_name);
  callback.Run(histogram_name, pref->GetValue());
}

void PrefMetricsService::LogBooleanPrefChange(const std::string& histogram_name,
                                              const base::Value* value) {
  bool boolean_value = false;
  if (!value->GetAsBoolean(&boolean_value))
    return;
  base::HistogramBase* histogram = base::BooleanHistogram::FactoryGet(
      histogram_name, base::HistogramBase::kUmaTargetedHistogramFlag);
  histogram->Add(boolean_value);
}

void PrefMetricsService::LogIntegerPrefChange(int boundary_value,
                                              const std::string& histogram_name,
                                              const base::Value* value) {
  int integer_value = 0;
  if (!value->GetAsInteger(&integer_value))
    return;
  base::HistogramBase* histogram = base::LinearHistogram::FactoryGet(
      histogram_name,
      1,
      boundary_value,
      boundary_value + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag);
  histogram->Add(integer_value);
}

// static
PrefMetricsService::Factory* PrefMetricsService::Factory::GetInstance() {
  return base::Singleton<PrefMetricsService::Factory>::get();
}

// static
PrefMetricsService* PrefMetricsService::Factory::GetForProfile(
    Profile* profile) {
  return static_cast<PrefMetricsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

PrefMetricsService::Factory::Factory()
    : BrowserContextKeyedServiceFactory(
        "PrefMetricsService",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(TemplateURLServiceFactory::GetInstance());
}

PrefMetricsService::Factory::~Factory() {
}

KeyedService* PrefMetricsService::Factory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new PrefMetricsService(static_cast<Profile*>(profile));
}

bool PrefMetricsService::Factory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

bool PrefMetricsService::Factory::ServiceIsNULLWhileTesting() const {
  return false;
}

content::BrowserContext* PrefMetricsService::Factory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
