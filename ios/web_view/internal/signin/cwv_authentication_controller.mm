// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/signin/cwv_authentication_controller_internal.h"

#include "base/strings/sys_string_conversions.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/ios/browser/profile_oauth2_token_service_ios_delegate.h"
#include "ios/web_view/internal/signin/ios_web_view_signin_client.h"
#include "ios/web_view/internal/signin/web_view_account_tracker_service_factory.h"
#include "ios/web_view/internal/signin/web_view_oauth2_token_service_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_client_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_manager_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#import "ios/web_view/public/cwv_authentication_controller_delegate.h"
#import "ios/web_view/public/cwv_identity.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation CWVAuthenticationController {
  ios_web_view::WebViewBrowserState* _browserState;
}

@synthesize delegate = _delegate;

- (instancetype)initWithBrowserState:
    (ios_web_view::WebViewBrowserState*)browserState {
  self = [super init];
  if (self) {
    _browserState = browserState;

    ios_web_view::WebViewSigninClientFactory::GetForBrowserState(_browserState)
        ->SetAuthenticationController(self);
  }
  return self;
}

- (void)dealloc {
  ios_web_view::WebViewSigninClientFactory::GetForBrowserState(_browserState)
      ->SetAuthenticationController(nil);
}

#pragma mark - Public Methods

- (void)setDelegate:(id<CWVAuthenticationControllerDelegate>)delegate {
  _delegate = delegate;

  std::string authenticatedAccountID = [self authenticatedAccountID];
  if (!authenticatedAccountID.empty()) {
    [self tokenService]->LoadCredentials(authenticatedAccountID);
  }
}

- (CWVIdentity*)currentIdentity {
  AccountInfo accountInfo =
      [self accountInfoForAccountID:[self authenticatedAccountID]];
  if (!accountInfo.IsValid()) {
    return nil;
  }
  NSString* email = base::SysUTF8ToNSString(accountInfo.email);
  NSString* fullName = base::SysUTF8ToNSString(accountInfo.full_name);
  NSString* gaiaID = base::SysUTF8ToNSString(accountInfo.gaia);
  return
      [[CWVIdentity alloc] initWithEmail:email fullName:fullName gaiaID:gaiaID];
}

- (void)signInWithIdentity:(CWVIdentity*)identity {
  AccountTrackerService* accountTracker =
      ios_web_view::WebViewAccountTrackerServiceFactory::GetForBrowserState(
          _browserState);
  AccountInfo info;
  info.gaia = base::SysNSStringToUTF8(identity.gaiaID);
  info.email = base::SysNSStringToUTF8(identity.email);
  info.full_name = base::SysNSStringToUTF8(identity.fullName);
  std::string newAuthenticatedAccountID = accountTracker->SeedAccountInfo(info);
  std::string oldAuthenticatedAccountID = [self authenticatedAccountID];
  ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(_browserState)
      ->GetAuthenticatedAccountId();

  // Assert if already signed in as a different user.
  if (!oldAuthenticatedAccountID.empty())
    CHECK_EQ(newAuthenticatedAccountID, oldAuthenticatedAccountID);

  std::string newAuthenticatedEmail =
      [self accountInfoForAccountID:newAuthenticatedAccountID].email;
  ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(_browserState)
      ->OnExternalSigninCompleted(newAuthenticatedEmail);

  [self tokenServiceDelegate]->ReloadCredentials(newAuthenticatedAccountID);
}

- (void)signOut {
  ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(_browserState)
      ->SignOut(signin_metrics::ProfileSignout::USER_CLICKED_SIGNOUT_SETTINGS,
                signin_metrics::SignoutDelete::IGNORE_METRIC);
}

#pragma mark - Private Methods

- (ProfileOAuth2TokenService*)tokenService {
  return ios_web_view::WebViewOAuth2TokenServiceFactory::GetForBrowserState(
      _browserState);
}

- (ProfileOAuth2TokenServiceIOSDelegate*)tokenServiceDelegate {
  return static_cast<ProfileOAuth2TokenServiceIOSDelegate*>(
      [self tokenService]->GetDelegate());
}

- (std::string)authenticatedAccountID {
  return ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(
             _browserState)
      ->GetAuthenticatedAccountId();
}

- (AccountInfo)accountInfoForAccountID:(const std::string&)accountID {
  AccountTrackerService* accountTracker =
      ios_web_view::WebViewAccountTrackerServiceFactory::GetForBrowserState(
          _browserState);
  return accountTracker->GetAccountInfo(accountID);
}

@end
