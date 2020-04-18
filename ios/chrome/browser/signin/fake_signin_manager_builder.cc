// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/fake_signin_manager_builder.h"

#include <utility>

#include "components/signin/core/browser/fake_signin_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/account_tracker_service_factory.h"
#include "ios/chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "ios/chrome/browser/signin/oauth2_token_service_factory.h"
#include "ios/chrome/browser/signin/signin_client_factory.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"

namespace ios {

std::unique_ptr<KeyedService> BuildFakeSigninManager(
    web::BrowserState* browser_state) {
  ios::ChromeBrowserState* chrome_browser_state =
      ios::ChromeBrowserState::FromBrowserState(browser_state);
  std::unique_ptr<SigninManager> manager(new FakeSigninManager(
      SigninClientFactory::GetForBrowserState(chrome_browser_state),
      OAuth2TokenServiceFactory::GetForBrowserState(chrome_browser_state),
      ios::AccountTrackerServiceFactory::GetForBrowserState(
          chrome_browser_state),
      ios::GaiaCookieManagerServiceFactory::GetForBrowserState(
          chrome_browser_state)));
  manager->Initialize(nullptr);
  ios::SigninManagerFactory::GetInstance()
      ->NotifyObserversOfSigninManagerCreationForTesting(manager.get());
  return manager;
}

}  // namespace ios
