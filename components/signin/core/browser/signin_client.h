// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_CLIENT_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_CLIENT_H_

#include <memory>

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/signin/core/browser/webdata/token_web_data.h"
#include "google_apis/gaia/gaia_auth_fetcher.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "url/gurl.h"

class PrefService;
class TokenWebData;

namespace content_settings {
class Observer;
}

namespace net {
class URLRequestContextGetter;
}

// An interface that needs to be supplied to the Signin component by its
// embedder.
class SigninClient : public KeyedService {
 public:
  // The subcription for cookie changed notifications.
  class CookieChangeSubscription {
   public:
    virtual ~CookieChangeSubscription() = default;
  };

  ~SigninClient() override = default;

  // If |for_ephemeral| is true, special kind of device ID for ephemeral users
  // is generated.
  static std::string GenerateSigninScopedDeviceID(bool for_ephemeral);

  // Sign out.
  void SignOut();

  // Call when done local initialization and SigninClient can initiate any work
  // it has to do that may require other components (like ProfileManager) to be
  // available.
  virtual void DoFinalInit() = 0;

  // Gets the preferences associated with the client.
  virtual PrefService* GetPrefs() = 0;

  // Gets the TokenWebData instance associated with the client.
  virtual scoped_refptr<TokenWebData> GetDatabase() = 0;

  // Returns whether it is possible to revoke credentials.
  virtual bool CanRevokeCredentials() = 0;

  // Returns device id that is scoped to single signin. This device id will be
  // regenerated if user signs out and signs back in.
  // When refresh token is requested for this user it will be annotated with
  // this device id.
  virtual std::string GetSigninScopedDeviceId() = 0;

  // Returns the URL request context information associated with the client.
  virtual net::URLRequestContextGetter* GetURLRequestContext() = 0;

  // Returns whether the user's credentials should be merged into the cookie
  // jar on signin completion.
  virtual bool ShouldMergeSigninCredentialsIntoCookieJar() = 0;

  // Returns a string containing the version info of the product in which the
  // Signin component is being used.
  virtual std::string GetProductVersion() = 0;

  // Adds a callback to be called each time a cookie for |url| with name |name|
  // changes.
  // Note that |callback| will always be called on the thread that
  // |AddCookieChangeCallback| was called on.
  virtual std::unique_ptr<CookieChangeSubscription> AddCookieChangeCallback(
      const GURL& url,
      const std::string& name,
      net::CookieChangeCallback callback) = 0;

  // Called after Google signin has succeeded.
  virtual void OnSignedIn(const std::string& account_id,
                          const std::string& gaia_id,
                          const std::string& username,
                          const std::string& password) {}

  // Called after Google signin has succeeded and GetUserInfo has returned.
  virtual void PostSignedIn(const std::string& account_id,
                            const std::string& username,
                            const std::string& password) {}

  // Called before Google signout started, call |sign_out| to start the sign out
  // process.
  virtual void PreSignOut(const base::Callback<void()>& sign_out,
                          signin_metrics::ProfileSignout signout_source_metric);

  // Called before calling the GAIA logout endpoint.
  // For iOS, cookies should be cleaned up.
  virtual void PreGaiaLogout(base::OnceClosure callback);

  virtual bool IsFirstRun() const = 0;
  virtual base::Time GetInstallDate() = 0;

  // Returns true if GAIA cookies are allowed in the content area.
  virtual bool AreSigninCookiesAllowed() = 0;

  // Adds an observer to listen for changes to the state of sign in cookie
  // settings.
  virtual void AddContentSettingsObserver(
      content_settings::Observer* observer) = 0;
  virtual void RemoveContentSettingsObserver(
      content_settings::Observer* observer) = 0;

  // Execute |callback| if and when there is a network connection.
  virtual void DelayNetworkCall(const base::Closure& callback) = 0;

  // Creates a new platform-specific GaiaAuthFetcher.
  virtual std::unique_ptr<GaiaAuthFetcher> CreateGaiaAuthFetcher(
      GaiaAuthConsumer* consumer,
      const std::string& source,
      net::URLRequestContextGetter* getter) = 0;

  // Called once the credentials has been copied to another SigninManager.
  virtual void AfterCredentialsCopied() {}

  // Schedules migration to happen at next startup.
  virtual void SetReadyForDiceMigration(bool is_ready) {}

 protected:
  // Returns device id that is scoped to single signin.
  // Stores the ID in the kGoogleServicesSigninScopedDeviceId pref.
  std::string GetOrCreateScopedDeviceIdPref(PrefService* prefs);

 private:
  // Perform Chrome-specific sign out. This happens when user signs out or about
  // to sign in.
  // This method should not be called from the outside of SigninClient. External
  // callers must use SignOut() instead.
  virtual void OnSignedOut() = 0;
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_CLIENT_H_
