// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/identity_manager_factory.h"

#include <memory>

#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/core/browser/signin_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/oauth2_token_service_factory.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#include "services/identity/public/cpp/identity_manager.h"

// Class that wraps IdentityManager in a KeyedService (as IdentityManager is a
// client-side library intended for use by any process, it would be a layering
// violation for IdentityManager itself to have direct knowledge of
// KeyedService).
class IdentityManagerHolder : public KeyedService {
 public:
  explicit IdentityManagerHolder(ios::ChromeBrowserState* browser_state)
      : identity_manager_(
            ios::SigninManagerFactory::GetForBrowserState(browser_state),
            OAuth2TokenServiceFactory::GetForBrowserState(browser_state)) {}

  identity::IdentityManager* identity_manager() { return &identity_manager_; }

 private:
  identity::IdentityManager identity_manager_;
};

IdentityManagerFactory::IdentityManagerFactory()
    : BrowserStateKeyedServiceFactory(
          "IdentityManager",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(OAuth2TokenServiceFactory::GetInstance());
  DependsOn(ios::SigninManagerFactory::GetInstance());
}

IdentityManagerFactory::~IdentityManagerFactory() {}

// static
identity::IdentityManager* IdentityManagerFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  IdentityManagerHolder* holder = static_cast<IdentityManagerHolder*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));

  return holder->identity_manager();
}

// static
IdentityManagerFactory* IdentityManagerFactory::GetInstance() {
  return base::Singleton<IdentityManagerFactory>::get();
}

std::unique_ptr<KeyedService> IdentityManagerFactory::BuildServiceInstanceFor(
    web::BrowserState* browser_state) const {
  return std::make_unique<IdentityManagerHolder>(
      ios::ChromeBrowserState::FromBrowserState(browser_state));
}
