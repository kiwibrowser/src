// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/signin/web_view_identity_manager_factory.h"

#include <memory>

#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/core/browser/signin_manager.h"
#include "ios/web_view/internal/signin/web_view_oauth2_token_service_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_manager_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "services/identity/public/cpp/identity_manager.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

// Class that wraps IdentityManager in a KeyedService (as IdentityManager is a
// client-side library intended for use by any process, it would be a layering
// violation for IdentityManager itself to have direct knowledge of
// KeyedService).
class IdentityManagerHolder : public KeyedService {
 public:
  explicit IdentityManagerHolder(WebViewBrowserState* browser_state)
      : identity_manager_(
            WebViewSigninManagerFactory::GetForBrowserState(browser_state),
            WebViewOAuth2TokenServiceFactory::GetForBrowserState(
                browser_state)) {}

  identity::IdentityManager* identity_manager() { return &identity_manager_; }

 private:
  identity::IdentityManager identity_manager_;
};

WebViewIdentityManagerFactory::WebViewIdentityManagerFactory()
    : BrowserStateKeyedServiceFactory(
          "IdentityManager",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(WebViewOAuth2TokenServiceFactory::GetInstance());
  DependsOn(WebViewSigninManagerFactory::GetInstance());
}

WebViewIdentityManagerFactory::~WebViewIdentityManagerFactory() {}

// static
identity::IdentityManager* WebViewIdentityManagerFactory::GetForBrowserState(
    WebViewBrowserState* browser_state) {
  IdentityManagerHolder* holder = static_cast<IdentityManagerHolder*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));

  return holder->identity_manager();
}

// static
WebViewIdentityManagerFactory* WebViewIdentityManagerFactory::GetInstance() {
  return base::Singleton<WebViewIdentityManagerFactory>::get();
}

std::unique_ptr<KeyedService>
WebViewIdentityManagerFactory::BuildServiceInstanceFor(
    web::BrowserState* browser_state) const {
  return std::make_unique<IdentityManagerHolder>(
      WebViewBrowserState::FromBrowserState(browser_state));
}

}  // namespace ios_web_view
