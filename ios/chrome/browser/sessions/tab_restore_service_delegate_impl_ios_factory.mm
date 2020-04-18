// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/sessions/tab_restore_service_delegate_impl_ios_factory.h"

#include <memory>

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/sessions/tab_restore_service_delegate_impl_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// static
TabRestoreServiceDelegateImplIOS*
TabRestoreServiceDelegateImplIOSFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<TabRestoreServiceDelegateImplIOS*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
TabRestoreServiceDelegateImplIOSFactory*
TabRestoreServiceDelegateImplIOSFactory::GetInstance() {
  return base::Singleton<TabRestoreServiceDelegateImplIOSFactory>::get();
}

TabRestoreServiceDelegateImplIOSFactory::
    TabRestoreServiceDelegateImplIOSFactory()
    : BrowserStateKeyedServiceFactory(
          "TabRestoreServiceDelegateImplIOS",
          BrowserStateDependencyManager::GetInstance()) {}

TabRestoreServiceDelegateImplIOSFactory::
    ~TabRestoreServiceDelegateImplIOSFactory() {}

std::unique_ptr<KeyedService>
TabRestoreServiceDelegateImplIOSFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return std::make_unique<TabRestoreServiceDelegateImplIOS>(
      ios::ChromeBrowserState::FromBrowserState(context));
}
