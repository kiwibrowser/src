// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_TEST_SIGNIN_CLIENT_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_TEST_SIGNIN_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/signin/core/browser/signin_client.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "net/url_request/url_request_test_util.h"

class PrefService;

// An implementation of SigninClient for use in unittests. Instantiates test
// versions of the various objects that SigninClient is required to provide as
// part of its interface.
class TestSigninClient : public SigninClient {
 public:
  TestSigninClient(PrefService* pref_service);
  ~TestSigninClient() override;

  // SigninClient implementation that is specialized for unit tests.

  void DoFinalInit() override;

  // Returns NULL.
  // NOTE: This should be changed to return a properly-initalized PrefService
  // once there is a unit test that requires it.
  PrefService* GetPrefs() override;

  // Returns a pointer to a loaded database.
  scoped_refptr<TokenWebData> GetDatabase() override;

  // Returns true.
  bool CanRevokeCredentials() override;

  // Returns empty string.
  std::string GetSigninScopedDeviceId() override;

  // Does nothing.
  void OnSignedOut() override;

  // Trace that this was called.
  void PostSignedIn(const std::string& account_id,
                    const std::string& username,
                    const std::string& password) override;

  std::string get_signed_in_password() { return signed_in_password_; }

  // Returns the empty string.
  std::string GetProductVersion() override;

  // Returns a TestURLRequestContextGetter or an manually provided
  // URLRequestContextGetter.
  net::URLRequestContextGetter* GetURLRequestContext() override;

  // For testing purposes, can override the TestURLRequestContextGetter created
  // in the default constructor.
  void SetURLRequestContext(net::URLRequestContextGetter* request_context);

  // Returns true.
  bool ShouldMergeSigninCredentialsIntoCookieJar() override;

  // Registers |callback| and returns the subscription.
  // Note that |callback| will never be called.
  std::unique_ptr<SigninClient::CookieChangeSubscription>
  AddCookieChangeCallback(const GURL& url,
                          const std::string& name,
                          net::CookieChangeCallback callback) override;

  void set_are_signin_cookies_allowed(bool value) {
    are_signin_cookies_allowed_ = value;
  }

  // When |value| is true, network calls posted through DelayNetworkCall() are
  // delayed indefinitely.
  // When |value| is false, all pending calls are unblocked, and new calls are
  // executed immediately.
  void SetNetworkCallsDelayed(bool value);

  // SigninClient overrides:
  bool IsFirstRun() const override;
  base::Time GetInstallDate() override;
  bool AreSigninCookiesAllowed() override;
  void AddContentSettingsObserver(
      content_settings::Observer* observer) override;
  void RemoveContentSettingsObserver(
      content_settings::Observer* observer) override;
  void DelayNetworkCall(const base::Closure& callback) override;
  std::unique_ptr<GaiaAuthFetcher> CreateGaiaAuthFetcher(
      GaiaAuthConsumer* consumer,
      const std::string& source,
      net::URLRequestContextGetter* getter) override;
  void PreGaiaLogout(base::OnceClosure callback) override;

  // Loads the token database.
  void LoadTokenDatabase();

 private:
  base::ScopedTempDir temp_dir_;
  scoped_refptr<net::URLRequestContextGetter> request_context_;
  scoped_refptr<TokenWebData> database_;
  PrefService* pref_service_;
  bool are_signin_cookies_allowed_;
  bool network_calls_delayed_;
  std::vector<base::OnceClosure> delayed_network_calls_;

  // Pointer to be filled by PostSignedIn.
  std::string signed_in_password_;

  DISALLOW_COPY_AND_ASSIGN(TestSigninClient);
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_TEST_SIGNIN_CLIENT_H_
