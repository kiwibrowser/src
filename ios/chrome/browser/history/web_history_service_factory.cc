// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/history/web_history_service_factory.h"

#include "base/memory/singleton.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/history/core/browser/web_history_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/sync/driver/sync_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/identity_manager_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#include "net/url_request/url_request_context_getter.h"

namespace ios {
namespace {

// Returns true if the user is signed-in and full history sync is enabled,
// false otherwise.
bool IsHistorySyncEnabled(ios::ChromeBrowserState* browser_state) {
  syncer::SyncService* sync_service =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(browser_state);
  return sync_service && sync_service->IsSyncActive() &&
         sync_service->GetActiveDataTypes().Has(
             syncer::HISTORY_DELETE_DIRECTIVES);
}

}  // namespace

// static
history::WebHistoryService* WebHistoryServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  // Ensure that the service is not instantiated or used if the user is not
  // signed into sync, or if web history is not enabled.
  if (!IsHistorySyncEnabled(browser_state))
    return nullptr;

  return static_cast<history::WebHistoryService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
WebHistoryServiceFactory* WebHistoryServiceFactory::GetInstance() {
  return base::Singleton<WebHistoryServiceFactory>::get();
}

WebHistoryServiceFactory::WebHistoryServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "WebHistoryService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(IOSChromeProfileSyncServiceFactory::GetInstance());
  DependsOn(IdentityManagerFactory::GetInstance());
}

WebHistoryServiceFactory::~WebHistoryServiceFactory() {
}

std::unique_ptr<KeyedService> WebHistoryServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<history::WebHistoryService>(
      IdentityManagerFactory::GetForBrowserState(browser_state),
      browser_state->GetRequestContext());
}

}  // namespace ios
