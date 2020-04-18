// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/profile_oauth2_token_service_ios_provider_impl.h"

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/browser/signin/constants.h"
#include "ios/chrome/browser/signin/signin_util.h"
#import "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#import "ios/public/provider/chrome/browser/signin/chrome_identity.h"
#include "ios/public/provider/chrome/browser/signin/chrome_identity_service.h"
#include "ios/public/provider/chrome/browser/signin/signin_error_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Returns the account info for |identity|.
// Returns an empty account info if |identity| is nil.
ProfileOAuth2TokenServiceIOSProvider::AccountInfo GetAccountInfo(
    ChromeIdentity* identity) {
  ProfileOAuth2TokenServiceIOSProvider::AccountInfo account_info;
  if (identity) {
    account_info.gaia = base::SysNSStringToUTF8([identity gaiaID]);
    account_info.email = base::SysNSStringToUTF8([identity userEmail]);
  }
  return account_info;
}
}

ProfileOAuth2TokenServiceIOSProviderImpl::
    ProfileOAuth2TokenServiceIOSProviderImpl() {}

ProfileOAuth2TokenServiceIOSProviderImpl::
    ~ProfileOAuth2TokenServiceIOSProviderImpl() {}

void ProfileOAuth2TokenServiceIOSProviderImpl::GetAccessToken(
    const std::string& gaia_id,
    const std::string& client_id,
    const std::set<std::string>& scopes,
    const AccessTokenCallback& callback) {
  AccessTokenCallback scoped_callback = callback;
  ios::ChromeIdentityService* identity_service =
      ios::GetChromeBrowserProvider()->GetChromeIdentityService();
  identity_service->GetAccessToken(
      identity_service->GetIdentityWithGaiaID(gaia_id), client_id, scopes,
      ^(NSString* token, NSDate* expiration, NSError* error) {
        if (!scoped_callback.is_null())
          scoped_callback.Run(token, expiration, error);
      });
}

std::vector<ProfileOAuth2TokenServiceIOSProvider::AccountInfo>
ProfileOAuth2TokenServiceIOSProviderImpl::GetAllAccounts() const {
  std::vector<AccountInfo> accounts;
  NSArray* identities = ios::GetChromeBrowserProvider()
                            ->GetChromeIdentityService()
                            ->GetAllIdentities();
  for (ChromeIdentity* identity in identities) {
    accounts.push_back(GetAccountInfo(identity));
  }
  return accounts;
}

AuthenticationErrorCategory
ProfileOAuth2TokenServiceIOSProviderImpl::GetAuthenticationErrorCategory(
    const std::string& gaia_id,
    NSError* error) const {
  DCHECK(error);
  if ([error.domain isEqualToString:kAuthenticationErrorDomain] &&
      error.code == NO_AUTHENTICATED_USER) {
    return kAuthenticationErrorCategoryUnknownIdentityErrors;
  }

  ios::ChromeIdentityService* identity_service =
      ios::GetChromeBrowserProvider()->GetChromeIdentityService();
  if (identity_service->IsMDMError(
          identity_service->GetIdentityWithGaiaID(gaia_id), error)) {
    return kAuthenticationErrorCategoryAuthorizationErrors;
  }

  ios::SigninErrorProvider* provider =
      ios::GetChromeBrowserProvider()->GetSigninErrorProvider();
  switch (provider->GetErrorCategory(error)) {
    case ios::SigninErrorCategory::UNKNOWN_ERROR: {
      // Google's OAuth 2 implementation returns a 400 with JSON body
      // containing error key "invalid_grant" to indicate the refresh token
      // is invalid or has been revoked by the user.
      // Check that the underlying library does not categorize these errors as
      // unknown.
      NSString* json_error_key = provider->GetInvalidGrantJsonErrorKey();
      DCHECK(!provider->IsBadRequest(error) ||
             ![[[error userInfo] valueForKeyPath:@"json.error"]
                 isEqual:json_error_key]);
      return kAuthenticationErrorCategoryUnknownErrors;
    }
    case ios::SigninErrorCategory::AUTHORIZATION_ERROR:
      if (provider->IsForbidden(error)) {
        return kAuthenticationErrorCategoryAuthorizationForbiddenErrors;
      }
      return kAuthenticationErrorCategoryAuthorizationErrors;
    case ios::SigninErrorCategory::NETWORK_ERROR:
      return kAuthenticationErrorCategoryNetworkServerErrors;
    case ios::SigninErrorCategory::USER_CANCELLATION_ERROR:
      return kAuthenticationErrorCategoryUserCancellationErrors;
  }
}
