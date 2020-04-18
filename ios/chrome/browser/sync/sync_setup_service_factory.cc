// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/sync/sync_setup_service_factory.h"

#include "base/memory/singleton.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service.h"

// static
SyncSetupService* SyncSetupServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<SyncSetupService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
SyncSetupService* SyncSetupServiceFactory::GetForBrowserStateIfExists(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<SyncSetupService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, false));
}

// static
SyncSetupServiceFactory* SyncSetupServiceFactory::GetInstance() {
  return base::Singleton<SyncSetupServiceFactory>::get();
}

SyncSetupServiceFactory::SyncSetupServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "SyncSetupService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(IOSChromeProfileSyncServiceFactory::GetInstance());
}

SyncSetupServiceFactory::~SyncSetupServiceFactory() {
}

std::unique_ptr<KeyedService> SyncSetupServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<SyncSetupService>(
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(browser_state),
      browser_state->GetPrefs());
}
