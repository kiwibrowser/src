// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/consent_auditor/consent_auditor_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/consent_auditor/consent_auditor.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/version_info/version_info.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/sync/ios_user_event_service_factory.h"
#include "ios/web/public/browser_state.h"

// static
consent_auditor::ConsentAuditor* ConsentAuditorFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<consent_auditor::ConsentAuditor*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
consent_auditor::ConsentAuditor*
ConsentAuditorFactory::GetForBrowserStateIfExists(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<consent_auditor::ConsentAuditor*>(
      GetInstance()->GetServiceForBrowserState(browser_state, false));
}

// static
ConsentAuditorFactory* ConsentAuditorFactory::GetInstance() {
  return base::Singleton<ConsentAuditorFactory>::get();
}

ConsentAuditorFactory::ConsentAuditorFactory()
    : BrowserStateKeyedServiceFactory(
          "ConsentAuditor",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(IOSUserEventServiceFactory::GetInstance());
}

ConsentAuditorFactory::~ConsentAuditorFactory() {}

std::unique_ptr<KeyedService> ConsentAuditorFactory::BuildServiceInstanceFor(
    web::BrowserState* browser_state) const {
  ios::ChromeBrowserState* ios_browser_state =
      ios::ChromeBrowserState::FromBrowserState(browser_state);
  syncer::UserEventService* const user_event_service =
      IOSUserEventServiceFactory::GetForBrowserState(ios_browser_state);
  return std::make_unique<consent_auditor::ConsentAuditor>(
      ios_browser_state->GetPrefs(), user_event_service,
      // The browser version and locale do not change runtime, so we can pass
      // them directly.
      version_info::GetVersionNumber(),
      GetApplicationContext()->GetApplicationLocale());
}

bool ConsentAuditorFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

void ConsentAuditorFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  consent_auditor::ConsentAuditor::RegisterProfilePrefs(registry);
}
