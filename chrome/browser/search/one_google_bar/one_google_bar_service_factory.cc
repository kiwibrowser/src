// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/one_google_bar/one_google_bar_service_factory.h"

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/optional.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/google/google_url_tracker_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_loader_impl.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_service.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "chrome/common/chrome_features.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/signin/core/browser/cookie_settings_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

// static
OneGoogleBarService* OneGoogleBarServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<OneGoogleBarService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
OneGoogleBarServiceFactory* OneGoogleBarServiceFactory::GetInstance() {
  return base::Singleton<OneGoogleBarServiceFactory>::get();
}

OneGoogleBarServiceFactory::OneGoogleBarServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "OneGoogleBarService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(CookieSettingsFactory::GetInstance());
  DependsOn(GaiaCookieManagerServiceFactory::GetInstance());
  DependsOn(GoogleURLTrackerFactory::GetInstance());
}

OneGoogleBarServiceFactory::~OneGoogleBarServiceFactory() = default;

KeyedService* OneGoogleBarServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(features::kOneGoogleBarOnLocalNtp)) {
    return nullptr;
  }

  Profile* profile = Profile::FromBrowserContext(context);
  GaiaCookieManagerService* cookie_service =
      GaiaCookieManagerServiceFactory::GetForProfile(profile);
  GoogleURLTracker* google_url_tracker =
      GoogleURLTrackerFactory::GetForProfile(profile);
  std::string override_api_url_str = base::GetFieldTrialParamValueByFeature(
      features::kOneGoogleBarOnLocalNtp, "one-google-api-url");
  base::Optional<std::string> override_api_url;
  if (!override_api_url_str.empty()) {
    override_api_url = override_api_url_str;
  }
  content_settings::CookieSettings* cookie_settings =
      CookieSettingsFactory::GetForProfile(profile).get();
  auto url_loader_factory =
      content::BrowserContext::GetDefaultStoragePartition(context)
          ->GetURLLoaderFactoryForBrowserProcess();
  return new OneGoogleBarService(
      cookie_service,
      std::make_unique<OneGoogleBarLoaderImpl>(
          url_loader_factory, google_url_tracker,
          g_browser_process->GetApplicationLocale(), override_api_url,
          AccountConsistencyModeManager::IsMirrorEnabledForProfile(profile) &&
              signin::SettingsAllowSigninCookies(cookie_settings)));
}
