// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/test/app/signin_test_util.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#import "base/test/ios/wait_util.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "google_apis/gaia/gaia_constants.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/signin/account_tracker_service_factory.h"
#import "ios/chrome/browser/signin/authentication_service.h"
#include "ios/chrome/browser/signin/authentication_service_factory.h"
#include "ios/chrome/browser/signin/gaia_auth_fetcher_ios.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity.h"
#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity_service.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_status.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

net::FakeURLFetcherFactory* gFakeURLFetcherFactory = nullptr;

// Specialization of the FakeURLFetcherFactory that will recognize GET requests
// for MergeSession and answer those requests correctly.
class MergeSessionFakeURLFetcherFactory : public net::FakeURLFetcherFactory {
 public:
  explicit MergeSessionFakeURLFetcherFactory(URLFetcherFactory* default_factory)
      : net::FakeURLFetcherFactory(default_factory) {}
  std::unique_ptr<net::URLFetcher> CreateURLFetcher(
      int id,
      const GURL& url,
      net::URLFetcher::RequestType request_type,
      net::URLFetcherDelegate* d,
      net::NetworkTrafficAnnotationTag traffic_annotation) override {
    const GURL kMergeSessionURL =
        GURL("https://accounts.google.com/MergeSession");
    url::Replacements<char> replacements;
    replacements.ClearRef();
    replacements.ClearQuery();
    if (url.ReplaceComponents(replacements) != kMergeSessionURL) {
      // URL is not a MergeSession GET. Use the default method.
      return net::FakeURLFetcherFactory::CreateURLFetcher(
          id, url, request_type, d, traffic_annotation);
    }
    // Actual MergeSession request. Answer is ignored by the AccountReconcilor,
    // so it can also be empty.
    return std::unique_ptr<net::FakeURLFetcher>(new net::FakeURLFetcher(
        url, d, "", net::HTTP_OK, net::URLRequestStatus::SUCCESS));
  }
};

}  // namespace

namespace chrome_test_util {

void SetUpMockAuthentication() {
  ios::ChromeBrowserProvider* provider = ios::GetChromeBrowserProvider();
  std::unique_ptr<ios::FakeChromeIdentityService> service(
      new ios::FakeChromeIdentityService());
  service->SetUpForIntegrationTests();
  provider->SetChromeIdentityServiceForTesting(std::move(service));
  AuthenticationServiceFactory::GetForBrowserState(GetOriginalBrowserState())
      ->ResetChromeIdentityServiceObserverForTesting();
}

void TearDownMockAuthentication() {
  ios::ChromeBrowserProvider* provider = ios::GetChromeBrowserProvider();
  provider->SetChromeIdentityServiceForTesting(nullptr);
  AuthenticationServiceFactory::GetForBrowserState(GetOriginalBrowserState())
      ->ResetChromeIdentityServiceObserverForTesting();
}

void SetUpMockAccountReconcilor() {
  gFakeURLFetcherFactory =
      new MergeSessionFakeURLFetcherFactory(new net::URLFetcherImplFactory());
  GaiaAuthFetcherIOS::SetShouldUseGaiaAuthFetcherIOSForTesting(false);

  // Answer is URLs in JSON that will be fetched and used in MergeSession that
  // we also intercept, so it can be empty.
  const GURL kCheckConnectionInfoURL = GURL(base::StringPrintf(
      "https://accounts.google.com/GetCheckConnectionInfo?source=%s",
      GaiaConstants::kChromeSource));
  gFakeURLFetcherFactory->SetFakeResponse(kCheckConnectionInfoURL, "[]",
                                          net::HTTP_OK,
                                          net::URLRequestStatus::SUCCESS);

  // No accounts in the cookie jar, will trigger a MergeSession for all of
  // them.
  const GURL kListAccountsURL = GURL(base::StringPrintf(
      "https://accounts.google.com/ListAccounts?source=%s&json=standard",
      GaiaConstants::kChromeSource));
  gFakeURLFetcherFactory->SetFakeResponse(kListAccountsURL, "[\"\",[]]",
                                          net::HTTP_OK,
                                          net::URLRequestStatus::SUCCESS);

  // Ubertoken, which is sent for the MergeSession that we also intercept, so
  // it can be any bogus token.
  const GURL kUbertokenURL = GURL(base::StringPrintf(
      "https://accounts.google.com/OAuthLogin?source=%s&issueuberauth=1",
      GaiaConstants::kChromeSource));
  gFakeURLFetcherFactory->SetFakeResponse(
      kUbertokenURL, "1234-asdf-fake-ubertoken", net::HTTP_OK,
      net::URLRequestStatus::SUCCESS);

  // MergeSession request is handled by MergeSessionFakeURLFetcherFactory.

  // If a profile was previously signed in without chrome identity, a logout
  // action is done and will block all other requests until it has been dealt
  // with.
  const GURL kLogoutURL =
      GURL(base::StringPrintf("https://accounts.google.com/Logout?source=%s",
                              GaiaConstants::kChromeSource));
  gFakeURLFetcherFactory->SetFakeResponse(kLogoutURL, "", net::HTTP_OK,
                                          net::URLRequestStatus::SUCCESS);
}

void TearDownMockAccountReconcilor() {
  GaiaAuthFetcherIOS::SetShouldUseGaiaAuthFetcherIOSForTesting(true);
  delete gFakeURLFetcherFactory;
  gFakeURLFetcherFactory = nullptr;
}

bool SignOutAndClearAccounts() {
  ios::ChromeBrowserState* browser_state = GetOriginalBrowserState();
  DCHECK(browser_state);

  // Sign out current user.
  AuthenticationService* authentication_service =
      AuthenticationServiceFactory::GetForBrowserState(browser_state);
  if (authentication_service->IsAuthenticated()) {
    authentication_service->SignOut(signin_metrics::SIGNOUT_TEST, nil);
  }

  // Clear the tracked accounts.
  AccountTrackerService* account_tracker =
      ios::AccountTrackerServiceFactory::GetForBrowserState(browser_state);
  for (const AccountInfo& info : account_tracker->GetAccounts()) {
    account_tracker->RemoveAccount(info.account_id);
  }

  // Clear last signed in user preference.
  browser_state->GetPrefs()->ClearPref(prefs::kGoogleServicesLastAccountId);
  browser_state->GetPrefs()->ClearPref(prefs::kGoogleServicesLastUsername);

  // Clear known identities.
  ios::ChromeIdentityService* identity_service =
      ios::GetChromeBrowserProvider()->GetChromeIdentityService();
  NSArray* identities([identity_service->GetAllIdentities() copy]);
  for (ChromeIdentity* identity in identities) {
    identity_service->ForgetIdentity(identity, nil);
  }

  NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:10.0];
  while (identity_service->HasIdentities() &&
         [[NSDate date] compare:deadline] != NSOrderedDescending) {
    base::test::ios::SpinRunLoopWithMaxDelay(
        base::TimeDelta::FromSecondsD(0.01));
  }
  return !identity_service->HasIdentities();
}

void ResetMockAuthentication() {
  ios::FakeChromeIdentityService::GetInstanceFromChromeProvider()
      ->SetFakeMDMError(false);
}

void ResetSigninPromoPreferences() {
  ios::ChromeBrowserState* browser_state = GetOriginalBrowserState();
  PrefService* prefs = browser_state->GetPrefs();
  prefs->SetInteger(prefs::kIosBookmarkSigninPromoDisplayedCount, 0);
  prefs->SetBoolean(prefs::kIosBookmarkPromoAlreadySeen, false);
  prefs->SetInteger(prefs::kIosSettingsSigninPromoDisplayedCount, 0);
  prefs->SetBoolean(prefs::kIosSettingsPromoAlreadySeen, false);
}

}  // namespace chrome_test_util
