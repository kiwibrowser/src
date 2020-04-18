// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/account_tracker_service_factory.h"

#include <utility>

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/signin_client_factory.h"

namespace ios {

AccountTrackerServiceFactory::AccountTrackerServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "AccountTrackerService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(SigninClientFactory::GetInstance());
}

AccountTrackerServiceFactory::~AccountTrackerServiceFactory() {}

// static
AccountTrackerService* AccountTrackerServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<AccountTrackerService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
AccountTrackerServiceFactory* AccountTrackerServiceFactory::GetInstance() {
  return base::Singleton<AccountTrackerServiceFactory>::get();
}

void AccountTrackerServiceFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  AccountTrackerService::RegisterPrefs(registry);
}

std::unique_ptr<KeyedService>
AccountTrackerServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* chrome_browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<AccountTrackerService> service(new AccountTrackerService());
  service->Initialize(
      SigninClientFactory::GetForBrowserState(chrome_browser_state));
  return service;
}

}  // namespace ios
