// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/desktop_promotion/desktop_promotion_sync_service_factory.h"

#include <memory>

#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/desktop_promotion/desktop_promotion_sync_service.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"

// static
DesktopPromotionSyncService*
DesktopPromotionSyncServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<DesktopPromotionSyncService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
DesktopPromotionSyncServiceFactory*
DesktopPromotionSyncServiceFactory::GetInstance() {
  return base::Singleton<DesktopPromotionSyncServiceFactory>::get();
}

DesktopPromotionSyncServiceFactory::DesktopPromotionSyncServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "DesktopPromotionSyncService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(IOSChromeProfileSyncServiceFactory::GetInstance());
}

DesktopPromotionSyncServiceFactory::~DesktopPromotionSyncServiceFactory() =
    default;

std::unique_ptr<KeyedService>
DesktopPromotionSyncServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<DesktopPromotionSyncService>(
      browser_state->GetPrefs(),
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(browser_state));
}

bool DesktopPromotionSyncServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
