// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/signin/authentication_service_fake.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/account_tracker_service_factory.h"
#import "ios/chrome/browser/signin/authentication_service_delegate_fake.h"
#import "ios/chrome/browser/signin/authentication_service_factory.h"
#include "ios/chrome/browser/signin/oauth2_token_service_factory.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#import "ios/public/provider/chrome/browser/signin/chrome_identity.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

AuthenticationServiceFake::AuthenticationServiceFake(
    PrefService* pref_service,
    ProfileOAuth2TokenService* token_service,
    SyncSetupService* sync_setup_service,
    AccountTrackerService* account_tracker,
    SigninManager* signin_manager,
    browser_sync::ProfileSyncService* sync_service)
    : AuthenticationService(pref_service,
                            token_service,
                            sync_setup_service,
                            account_tracker,
                            signin_manager,
                            sync_service),
      have_accounts_changed_(false) {}

AuthenticationServiceFake::~AuthenticationServiceFake() {}

void AuthenticationServiceFake::SignIn(ChromeIdentity* identity,
                                       const std::string& hosted_domain) {
  authenticated_identity_ = identity;
}

void AuthenticationServiceFake::SignOut(
    signin_metrics::ProfileSignout signout_source,
    ProceduralBlock completion) {
  authenticated_identity_ = nil;
  if (completion)
    completion();
}

void AuthenticationServiceFake::SetHaveAccountsChanged(bool changed) {
  have_accounts_changed_ = changed;
}

bool AuthenticationServiceFake::HaveAccountsChanged() {
  return have_accounts_changed_;
}

bool AuthenticationServiceFake::IsAuthenticated() {
  return authenticated_identity_ != nil;
}

ChromeIdentity* AuthenticationServiceFake::GetAuthenticatedIdentity() {
  return authenticated_identity_;
}

NSString* AuthenticationServiceFake::GetAuthenticatedUserEmail() {
  return [authenticated_identity_ userEmail];
}

std::unique_ptr<KeyedService>
AuthenticationServiceFake::CreateAuthenticationService(
    web::BrowserState* context) {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  auto service = base::WrapUnique(new AuthenticationServiceFake(
      browser_state->GetPrefs(),
      OAuth2TokenServiceFactory::GetForBrowserState(browser_state),
      SyncSetupServiceFactory::GetForBrowserState(browser_state),
      ios::AccountTrackerServiceFactory::GetForBrowserState(browser_state),
      ios::SigninManagerFactory::GetForBrowserState(browser_state),
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(browser_state)));
  service->Initialize(std::make_unique<AuthenticationServiceDelegateFake>());
  return service;
}
