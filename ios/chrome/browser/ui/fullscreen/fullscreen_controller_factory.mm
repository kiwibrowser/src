// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_factory.h"

#include <memory>

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// static
FullscreenController* FullscreenControllerFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<FullscreenController*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
FullscreenControllerFactory* FullscreenControllerFactory::GetInstance() {
  return base::Singleton<FullscreenControllerFactory>::get();
}

FullscreenControllerFactory::FullscreenControllerFactory()
    : BrowserStateKeyedServiceFactory(
          "FullscreenController",
          BrowserStateDependencyManager::GetInstance()) {}

std::unique_ptr<KeyedService>
FullscreenControllerFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return std::make_unique<FullscreenControllerImpl>();
}

web::BrowserState* FullscreenControllerFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateOwnInstanceInIncognito(context);
}
