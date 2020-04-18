// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/fake_oauth2_token_service_builder.h"

#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/ios/browser/profile_oauth2_token_service_ios_delegate.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/account_tracker_service_factory.h"
#include "ios/chrome/browser/signin/profile_oauth2_token_service_ios_provider_impl.h"
#include "ios/chrome/browser/signin/signin_client_factory.h"
#include "ios/chrome/browser/signin/signin_error_controller_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

std::unique_ptr<KeyedService> BuildFakeOAuth2TokenService(
    web::BrowserState* context) {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<OAuth2TokenServiceDelegate> delegate =
      std::make_unique<ProfileOAuth2TokenServiceIOSDelegate>(
          SigninClientFactory::GetForBrowserState(browser_state),
          std::make_unique<ProfileOAuth2TokenServiceIOSProviderImpl>(),
          ios::AccountTrackerServiceFactory::GetForBrowserState(browser_state),
          ios::SigninErrorControllerFactory::GetForBrowserState(browser_state));
  return std::make_unique<FakeProfileOAuth2TokenService>(std::move(delegate));
}
