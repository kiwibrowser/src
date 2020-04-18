// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/browser_list/browser_list_session_service_factory.h"

#include <memory>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/strings/sys_string_conversions.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/sessions/session_service_ios.h"
#import "ios/chrome/browser/ui/browser_list/browser_list_factory.h"
#import "ios/chrome/browser/ui/browser_list/browser_list_session_service_impl.h"
#import "ios/web/public/certificate_policy_cache.h"
#import "ios/web/public/web_state/session_certificate_policy_cache.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// WebState factory for BrowserListSessionServiceImpl. The factory is called
// to create a WebState from serialised session state, so the browser state
// certificate policy cache is updated with the information from the WebState.
std::unique_ptr<web::WebState> CreateWebState(
    ios::ChromeBrowserState* browser_state,
    CRWSessionStorage* session_storage) {
  std::unique_ptr<web::WebState> web_state =
      web::WebState::CreateWithStorageSession(
          web::WebState::CreateParams(browser_state), session_storage);
  web_state->GetSessionCertificatePolicyCache()->UpdateCertificatePolicyCache(
      web::BrowserState::GetCertificatePolicyCache(browser_state));
  return web_state;
}
}

// static
BrowserListSessionService* BrowserListSessionServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<BrowserListSessionService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
BrowserListSessionServiceFactory*
BrowserListSessionServiceFactory::GetInstance() {
  return base::Singleton<BrowserListSessionServiceFactory>::get();
}

BrowserListSessionServiceFactory::BrowserListSessionServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "BrowserListSessionService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(BrowserListFactory::GetInstance());
}

BrowserListSessionServiceFactory::~BrowserListSessionServiceFactory() {}

std::unique_ptr<KeyedService>
BrowserListSessionServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  // It is safe to use base::Unretained here as BrowserListSessionServiceImpl
  // will be destroyed before the ChromeBrowserState (as it is a KeyedService).
  return std::make_unique<BrowserListSessionServiceImpl>(
      BrowserListFactory::GetForBrowserState(browser_state),
      base::SysUTF8ToNSString(browser_state->GetStatePath().AsUTF8Unsafe()),
      [SessionServiceIOS sharedService],
      base::BindRepeating(&CreateWebState, base::Unretained(browser_state)));
}

web::BrowserState* BrowserListSessionServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  // The off-the-record ChromeBrowserState has its own BrowserListSessionService
  // to implement restoring the sessions after the application has been evicted
  // while in the background.
  return GetBrowserStateOwnInstanceInIncognito(context);
}

bool BrowserListSessionServiceFactory::ServiceIsNULLWhileTesting() const {
  // Avoid automatically saving session during unit tests.
  return true;
}
