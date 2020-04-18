// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_SIGNIN_IOS_WEB_VIEW_SIGNIN_CLIENT_H_
#define IOS_WEB_VIEW_INTERNAL_SIGNIN_IOS_WEB_VIEW_SIGNIN_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/signin/core/browser/signin_client.h"
#include "components/signin/core/browser/signin_error_controller.h"
#include "components/signin/ios/browser/wait_for_network_callback_helper.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "net/url_request/url_request_context_getter.h"

@class CWVAuthenticationController;

// iOS WebView specific signin client.
class IOSWebViewSigninClient : public SigninClient,
                               public SigninErrorController::Observer {
 public:
  IOSWebViewSigninClient(
      PrefService* pref_service,
      net::URLRequestContextGetter* url_request_context,
      SigninErrorController* signin_error_controller,
      scoped_refptr<content_settings::CookieSettings> cookie_settings,
      scoped_refptr<HostContentSettingsMap> host_content_settings_map,
      scoped_refptr<TokenWebData> token_web_data);

  ~IOSWebViewSigninClient() override;

  // KeyedService implementation.
  void Shutdown() override;

  // SigninClient implementation.
  std::string GetProductVersion() override;
  base::Time GetInstallDate() override;
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
  std::unique_ptr<GaiaAuthFetcher> CreateGaiaAuthFetcher(
      GaiaAuthConsumer* consumer,
      const std::string& source,
      net::URLRequestContextGetter* getter) override;

  // SigninErrorController::Observer implementation.
  void OnErrorChanged() override;

  // Setter and getter for |authentication_controller_|.
  void SetAuthenticationController(
      CWVAuthenticationController* authentication_controller);
  CWVAuthenticationController* GetAuthenticationController();

 private:
  // SigninClient private implementation.
  void OnSignedOut() override;

  // Helper to delay callbacks until connection becomes online again.
  std::unique_ptr<WaitForNetworkCallbackHelper> network_callback_helper_;
  // The PrefService associated with this service.
  PrefService* pref_service_;
  // The URLRequestContext associated with this service.
  net::URLRequestContextGetter* url_request_context_;
  // Used to check for errors related to signing in.
  SigninErrorController* signin_error_controller_;
  // Used to check if sign in cookies are allowed.
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;
  // Used to add and remove content settings observers.
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  // The TokenWebData associated with this service.
  scoped_refptr<TokenWebData> token_web_data_;

  // The CWVAuthenticationController associated with this service.
  __weak CWVAuthenticationController* authentication_controller_;

  DISALLOW_COPY_AND_ASSIGN(IOSWebViewSigninClient);
};

#endif  // IOS_WEB_VIEW_INTERNAL_SIGNIN_IOS_WEB_VIEW_SIGNIN_CLIENT_H_
