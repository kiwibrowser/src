// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OAUTH2_TOKEN_SERVICE_DELEGATE_H_
#define CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OAUTH2_TOKEN_SERVICE_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "google_apis/gaia/gaia_oauth_client.h"
#include "google_apis/gaia/oauth2_token_service_delegate.h"
#include "net/url_request/url_request_context_getter.h"

namespace gaia {
class GaiaOAuthClient;
}

namespace net {
class URLRequestContextGetter;
}

class PrefService;

namespace chromeos {

class DeviceOAuth2TokenServiceDelegate
    : public OAuth2TokenServiceDelegate,
      public gaia::GaiaOAuthClient::Delegate {
 public:
  DeviceOAuth2TokenServiceDelegate(net::URLRequestContextGetter* getter,
                                   PrefService* local_state);
  ~DeviceOAuth2TokenServiceDelegate() override;

  typedef base::Callback<void(bool)> StatusCallback;

  // Persist the given refresh token on the device. Overwrites any previous
  // value. Should only be called during initial device setup. Signals
  // completion via the given callback, passing true if the operation succeeded.
  void SetAndSaveRefreshToken(const std::string& refresh_token,
                              const StatusCallback& callback);

  // Pull the robot account ID from device policy.
  std::string GetRobotAccountId() const;

  // Implementation of OAuth2TokenServiceDelegate.
  bool RefreshTokenIsAvailable(const std::string& account_id) const override;

  net::URLRequestContextGetter* GetRequestContext() const override;

  OAuth2AccessTokenFetcher* CreateAccessTokenFetcher(
      const std::string& account_id,
      net::URLRequestContextGetter* getter,
      OAuth2AccessTokenConsumer* consumer) override;

  // gaia::GaiaOAuthClient::Delegate implementation.
  void OnRefreshTokenResponse(const std::string& access_token,
                              int expires_in_seconds) override;
  void OnGetTokenInfoResponse(
      std::unique_ptr<base::DictionaryValue> token_info) override;
  void OnOAuthError() override;
  void OnNetworkError(int response_code) override;

  class ValidationStatusDelegate {
   public:
    virtual void OnValidationCompleted(GoogleServiceAuthError::State error) {}
  };

 private:
  friend class DeviceOAuth2TokenService;
  friend class DeviceOAuth2TokenServiceTest;

  // Describes the operational state of this object.
  enum State {
    // Pending system salt / refresh token load.
    STATE_LOADING,
    // No token available.
    STATE_NO_TOKEN,
    // System salt loaded, validation not started yet.
    STATE_VALIDATION_PENDING,
    // Refresh token validation underway.
    STATE_VALIDATION_STARTED,
    // Token validation failed.
    STATE_TOKEN_INVALID,
    // Refresh token is valid.
    STATE_TOKEN_VALID,
  };

  // Invoked by CrosSettings when the robot account ID becomes available.
  void OnServiceAccountIdentityChanged();

  // Returns the refresh token for account_id.
  std::string GetRefreshToken(const std::string& account_id) const;

  // Handles completion of the system salt input.
  void DidGetSystemSalt(const std::string& system_salt);

  // Checks whether |gaia_robot_id| matches the expected account ID indicated in
  // device settings.
  void CheckRobotAccountId(const std::string& gaia_robot_id);

  // Encrypts and saves the refresh token. Should only be called when the system
  // salt is available.
  void EncryptAndSaveToken();

  // Starts the token validation flow, i.e. token info fetch.
  void StartValidation();

  // Flushes |token_save_callbacks_|, indicating the specified result.
  void FlushTokenSaveCallbacks(bool result);

  void RequestValidation();

  void SetValidationStatusDelegate(ValidationStatusDelegate* delegate);

  void ReportServiceError(GoogleServiceAuthError::State error);

  // Dependencies.
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  PrefService* local_state_;

  // Current operational state.
  State state_;

  // Token save callbacks waiting to be completed.
  std::vector<StatusCallback> token_save_callbacks_;

  // The system salt for encrypting and decrypting the refresh token.
  std::string system_salt_;

  int max_refresh_token_validation_retries_;

  // Flag to indicate whether there are pending requests.
  bool validation_requested_;

  // Validation status delegate
  ValidationStatusDelegate* validation_status_delegate_;

  // Cache the decrypted refresh token, so we only decrypt once.
  std::string refresh_token_;

  std::unique_ptr<gaia::GaiaOAuthClient> gaia_oauth_client_;

  std::unique_ptr<CrosSettings::ObserverSubscription>
      service_account_identity_subscription_;

  base::WeakPtrFactory<DeviceOAuth2TokenServiceDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOAuth2TokenServiceDelegate);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OAUTH2_TOKEN_SERVICE_DELEGATE_H_
