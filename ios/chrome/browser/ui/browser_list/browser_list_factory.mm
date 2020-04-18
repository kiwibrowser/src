// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/browser_list/browser_list_factory.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/browser_list/browser_list_impl.h"
#import "ios/chrome/browser/ui/browser_list/browser_web_state_list_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// static
BrowserList* BrowserListFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<BrowserList*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
BrowserListFactory* BrowserListFactory::GetInstance() {
  return base::Singleton<BrowserListFactory>::get();
}

BrowserListFactory::BrowserListFactory()
    : BrowserStateKeyedServiceFactory(
          "BrowserList",
          BrowserStateDependencyManager::GetInstance()) {}

BrowserListFactory::~BrowserListFactory() = default;

std::unique_ptr<KeyedService> BrowserListFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return std::make_unique<BrowserListImpl>(
      ios::ChromeBrowserState::FromBrowserState(context),
      std::make_unique<BrowserWebStateListDelegate>());
}

web::BrowserState* BrowserListFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateOwnInstanceInIncognito(context);
}
