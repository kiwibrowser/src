// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/ios_chrome_signin_client.h"

#include "base/strings/utf_string_conversions.h"
#include "components/metrics/metrics_service.h"
#include "components/signin/core/browser/cookie_settings_util.h"
#include "components/signin/core/browser/signin_cookie_change_subscription.h"
#include "components/signin/ios/browser/account_consistency_service.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/browser_state_info_cache.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/signin/account_consistency_service_factory.h"
#include "ios/chrome/browser/signin/gaia_auth_fetcher_ios.h"
#include "ios/chrome/browser/web_data_service_factory.h"
#include "ios/chrome/common/channel_info.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

IOSChromeSigninClient::IOSChromeSigninClient(
    ios::ChromeBrowserState* browser_state,
    SigninErrorController* signin_error_controller,
    scoped_refptr<content_settings::CookieSettings> cookie_settings,
    scoped_refptr<HostContentSettingsMap> host_content_settings_map,
    scoped_refptr<TokenWebData> token_web_data)
    : network_callback_helper_(
          std::make_unique<WaitForNetworkCallbackHelper>()),
      browser_state_(browser_state),
      signin_error_controller_(signin_error_controller),
      cookie_settings_(cookie_settings),
      host_content_settings_map_(host_content_settings_map),
      token_web_data_(token_web_data) {
  signin_error_controller_->AddObserver(this);
}

IOSChromeSigninClient::~IOSChromeSigninClient() {
  signin_error_controller_->RemoveObserver(this);
}

void IOSChromeSigninClient::Shutdown() {
  network_callback_helper_.reset();
}

base::Time IOSChromeSigninClient::GetInstallDate() {
  return base::Time::FromTimeT(
      GetApplicationContext()->GetMetricsService()->GetInstallDate());
}

// Returns a string describing the chrome version environment. Version format:
// <Build Info> <OS> <Version number> (<Last change>)<channel or "-devel">
// If version information is unavailable, returns "invalid."
std::string IOSChromeSigninClient::GetProductVersion() {
  return GetVersionString();
}

void IOSChromeSigninClient::OnSignedIn(const std::string& account_id,
                                       const std::string& gaia_id,
                                       const std::string& username,
                                       const std::string& password) {
  ios::ChromeBrowserStateManager* browser_state_manager =
      GetApplicationContext()->GetChromeBrowserStateManager();
  BrowserStateInfoCache* cache =
      browser_state_manager->GetBrowserStateInfoCache();
  size_t index = cache->GetIndexOfBrowserStateWithPath(
      browser_state_->GetOriginalChromeBrowserState()->GetStatePath());
  if (index != std::string::npos) {
    cache->SetAuthInfoOfBrowserStateAtIndex(index, gaia_id,
                                            base::UTF8ToUTF16(username));
  }
}

void IOSChromeSigninClient::OnSignedOut() {
  BrowserStateInfoCache* cache = GetApplicationContext()
                                     ->GetChromeBrowserStateManager()
                                     ->GetBrowserStateInfoCache();
  size_t index = cache->GetIndexOfBrowserStateWithPath(
      browser_state_->GetOriginalChromeBrowserState()->GetStatePath());

  // If sign out occurs because Sync setup was in progress and the browser state
  // got deleted, then it is no longer in the cache.
  if (index == std::string::npos)
    return;

  cache->SetAuthInfoOfBrowserStateAtIndex(index, std::string(),
                                          base::string16());
}

scoped_refptr<TokenWebData> IOSChromeSigninClient::GetDatabase() {
  return token_web_data_;
}

PrefService* IOSChromeSigninClient::GetPrefs() {
  return browser_state_->GetPrefs();
}

net::URLRequestContextGetter* IOSChromeSigninClient::GetURLRequestContext() {
  return browser_state_->GetRequestContext();
}

void IOSChromeSigninClient::DoFinalInit() {}

bool IOSChromeSigninClient::CanRevokeCredentials() {
  return true;
}

std::string IOSChromeSigninClient::GetSigninScopedDeviceId() {
  return GetOrCreateScopedDeviceIdPref(GetPrefs());
}

bool IOSChromeSigninClient::ShouldMergeSigninCredentialsIntoCookieJar() {
  return false;
}

bool IOSChromeSigninClient::IsFirstRun() const {
  return false;
}

bool IOSChromeSigninClient::AreSigninCookiesAllowed() {
  return signin::SettingsAllowSigninCookies(cookie_settings_.get());
}

void IOSChromeSigninClient::AddContentSettingsObserver(
    content_settings::Observer* observer) {
  host_content_settings_map_->AddObserver(observer);
}

void IOSChromeSigninClient::RemoveContentSettingsObserver(
    content_settings::Observer* observer) {
  host_content_settings_map_->RemoveObserver(observer);
}

std::unique_ptr<SigninClient::CookieChangeSubscription>
IOSChromeSigninClient::AddCookieChangeCallback(
    const GURL& url,
    const std::string& name,
    net::CookieChangeCallback callback) {
  scoped_refptr<net::URLRequestContextGetter> context_getter =
      GetURLRequestContext();
  DCHECK(context_getter.get());
  return std::make_unique<SigninCookieChangeSubscription>(
      context_getter, url, name, std::move(callback));
}

void IOSChromeSigninClient::DelayNetworkCall(const base::Closure& callback) {
  network_callback_helper_->HandleCallback(callback);
}

std::unique_ptr<GaiaAuthFetcher> IOSChromeSigninClient::CreateGaiaAuthFetcher(
    GaiaAuthConsumer* consumer,
    const std::string& source,
    net::URLRequestContextGetter* getter) {
  return std::make_unique<GaiaAuthFetcherIOS>(consumer, source, getter,
                                              browser_state_);
}

void IOSChromeSigninClient::PreGaiaLogout(base::OnceClosure callback) {
  AccountConsistencyService* accountConsistencyService =
      ios::AccountConsistencyServiceFactory::GetForBrowserState(browser_state_);
  accountConsistencyService->RemoveChromeConnectedCookies(std::move(callback));
}

void IOSChromeSigninClient::OnErrorChanged() {
  BrowserStateInfoCache* cache = GetApplicationContext()
                                     ->GetChromeBrowserStateManager()
                                     ->GetBrowserStateInfoCache();
  size_t index = cache->GetIndexOfBrowserStateWithPath(
      browser_state_->GetOriginalChromeBrowserState()->GetStatePath());
  if (index == std::string::npos)
    return;

  cache->SetBrowserStateIsAuthErrorAtIndex(
      index, signin_error_controller_->HasError());
}
