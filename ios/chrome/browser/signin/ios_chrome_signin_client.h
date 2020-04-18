// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_CLIENT_H_
#define IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/signin/core/browser/signin_client.h"
#include "components/signin/core/browser/signin_error_controller.h"
#include "components/signin/ios/browser/wait_for_network_callback_helper.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "net/url_request/url_request_context_getter.h"

namespace ios {
class ChromeBrowserState;
}

// Concrete implementation of SigninClient for //ios/chrome.
class IOSChromeSigninClient : public SigninClient,
                              public SigninErrorController::Observer {
 public:
  IOSChromeSigninClient(
      ios::ChromeBrowserState* browser_state,
      SigninErrorController* signin_error_controller,
      scoped_refptr<content_settings::CookieSettings> cookie_settings,
      scoped_refptr<HostContentSettingsMap> host_content_settings_map,
      scoped_refptr<TokenWebData> token_web_data);
  ~IOSChromeSigninClient() override;

  // KeyedService implementation.
  void Shutdown() override;

  // SigninClient implementation.
  base::Time GetInstallDate() override;
  std::string GetProductVersion() override;
  void OnSignedIn(const std::string& account_id,
                  const std::string& gaia_id,
                  const std::string& username,
                  const std::string& password) override;
  std::unique_ptr<GaiaAuthFetcher> CreateGaiaAuthFetcher(
      GaiaAuthConsumer* consumer,
      const std::string& source,
      net::URLRequestContextGetter* getter) override;
  void PreGaiaLogout(base::OnceClosure callback) override;
  scoped_refptr<TokenWebData> GetDatabase() override;
  PrefService* GetPrefs() override;
  net::URLRequestContextGetter* GetURLRequestContext() override;
  void DoFinalInit() override;
  bool CanRevokeCredentials() override;
  std::string GetSigninScopedDeviceId() override;
  bool ShouldMergeSigninCredentialsIntoCookieJar() override;
  bool IsFirstRun() const override;
  bool AreSigninCookiesAllowed() override;
  void AddContentSettingsObserver(
      content_settings::Observer* observer) override;
  void RemoveContentSettingsObserver(
      content_settings::Observer* observer) override;
  std::unique_ptr<CookieChangeSubscription> AddCookieChangeCallback(
      const GURL& url,
      const std::string& name,
      net::CookieChangeCallback callback) override;
  void DelayNetworkCall(const base::Closure& callback) override;

  // SigninErrorController::Observer implementation.
  void OnErrorChanged() override;

 private:
  // SigninClient private implementation.
  void OnSignedOut() override;

  // Helper to delay callbacks until connection becomes online again.
  std::unique_ptr<WaitForNetworkCallbackHelper> network_callback_helper_;
  // The browser state associated with this service.
  ios::ChromeBrowserState* browser_state_;
  // Used to check for errors related to signing in.
  SigninErrorController* signin_error_controller_;
  // Used to check if sign in cookies are allowed.
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;
  // Used to add and remove content settings observers.
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  // The TokenWebData associated with this service.
  scoped_refptr<TokenWebData> token_web_data_;

  DISALLOW_COPY_AND_ASSIGN(IOSChromeSigninClient);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_CLIENT_H_
