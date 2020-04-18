// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/google/google_url_tracker_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/google/core/browser/google_pref_names.h"
#include "components/google/core/browser/google_url_tracker.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/google/google_url_tracker_client_impl.h"

namespace ios {

// static
GoogleURLTracker* GoogleURLTrackerFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<GoogleURLTracker*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
GoogleURLTrackerFactory* GoogleURLTrackerFactory::GetInstance() {
  return base::Singleton<GoogleURLTrackerFactory>::get();
}

GoogleURLTrackerFactory::GoogleURLTrackerFactory()
    : BrowserStateKeyedServiceFactory(
          "GoogleURLTracker",
          BrowserStateDependencyManager::GetInstance()) {
}

GoogleURLTrackerFactory::~GoogleURLTrackerFactory() {
}

std::unique_ptr<KeyedService> GoogleURLTrackerFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);

  // Delete this now-unused pref.
  // At some point in the future, this code can be removed entirely.
  browser_state->GetOriginalChromeBrowserState()->GetPrefs()->ClearPref(
      prefs::kLastPromptedGoogleURL);

  return std::make_unique<GoogleURLTracker>(
      base::WrapUnique(new GoogleURLTrackerClientImpl(browser_state)),
      GoogleURLTracker::NORMAL_MODE);
}

web::BrowserState* GoogleURLTrackerFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

bool GoogleURLTrackerFactory::ServiceIsCreatedWithBrowserState() const {
  return true;
}

bool GoogleURLTrackerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

void GoogleURLTrackerFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  GoogleURLTracker::RegisterProfilePrefs(registry);
}

}  // namespace ios
