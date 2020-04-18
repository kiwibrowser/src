// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/settings/device_oauth2_token_service_delegate.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/settings/token_encryptor.h"
#include "chrome/common/pref_names.h"
#include "chromeos/cryptohome/system_salt_getter.h"
#include "chromeos/settings/cros_settings_names.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/gaia_urls.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "google_apis/gaia/oauth2_access_token_fetcher_impl.h"

namespace chromeos {

void DeviceOAuth2TokenServiceDelegate::OnServiceAccountIdentityChanged() {
  if (!GetRobotAccountId().empty() && !refresh_token_.empty())
    FireRefreshTokenAvailable(GetRobotAccountId());
}

DeviceOAuth2TokenServiceDelegate::DeviceOAuth2TokenServiceDelegate(
    net::URLRequestContextGetter* getter,
    PrefService* local_state)
    : url_request_context_getter_(getter),
      local_state_(local_state),
      state_(STATE_LOADING),
      max_refresh_token_validation_retries_(3),
      validation_requested_(false),
      validation_status_delegate_(nullptr),
      service_account_identity_subscription_(
          CrosSettings::Get()
              ->AddSettingsObserver(
                  kServiceAccountIdentity,
                  base::Bind(&DeviceOAuth2TokenServiceDelegate::
                                 OnServiceAccountIdentityChanged,
                             base::Unretained(this)))),
      weak_ptr_factory_(this) {
  // Pull in the system salt.
  SystemSaltGetter::Get()->GetSystemSalt(
      base::Bind(&DeviceOAuth2TokenServiceDelegate::DidGetSystemSalt,
                 weak_ptr_factory_.GetWeakPtr()));
}

DeviceOAuth2TokenServiceDelegate::~DeviceOAuth2TokenServiceDelegate() {
  FlushTokenSaveCallbacks(false);
}

void DeviceOAuth2TokenServiceDelegate::SetAndSaveRefreshToken(
    const std::string& refresh_token,
    const StatusCallback& result_callback) {
  ReportServiceError(GoogleServiceAuthError::REQUEST_CANCELED);

  bool waiting_for_salt = state_ == STATE_LOADING;
  refresh_token_ = refresh_token;
  state_ = STATE_VALIDATION_PENDING;

  // If the robot account ID is not available yet, do not announce the token. It
  // will be done from OnServiceAccountIdentityChanged() once the robot account
  // ID becomes available as well.
  if (!GetRobotAccountId().empty())
    FireRefreshTokenAvailable(GetRobotAccountId());

  token_save_callbacks_.push_back(result_callback);
  if (!waiting_for_salt) {
    if (system_salt_.empty())
      FlushTokenSaveCallbacks(false);
    else
      EncryptAndSaveToken();
  }
}

bool DeviceOAuth2TokenServiceDelegate::RefreshTokenIsAvailable(
    const std::string& account_id) const {
  switch (state_) {
    case STATE_NO_TOKEN:
    case STATE_TOKEN_INVALID:
      return false;
    case STATE_LOADING:
    case STATE_VALIDATION_PENDING:
    case STATE_VALIDATION_STARTED:
    case STATE_TOKEN_VALID:
      return account_id == GetRobotAccountId();
  }

  NOTREACHED() << "Unhandled state " << state_;
  return false;
}

std::string DeviceOAuth2TokenServiceDelegate::GetRobotAccountId() const {
  std::string result;
  CrosSettings::Get()->GetString(kServiceAccountIdentity, &result);
  return result;
}

void DeviceOAuth2TokenServiceDelegate::OnRefreshTokenResponse(
    const std::string& access_token,
    int expires_in_seconds) {
  gaia_oauth_client_->GetTokenInfo(access_token,
                                   max_refresh_token_validation_retries_, this);
}

void DeviceOAuth2TokenServiceDelegate::OnGetTokenInfoResponse(
    std::unique_ptr<base::DictionaryValue> token_info) {
  std::string gaia_robot_id;
  token_info->GetString("email", &gaia_robot_id);
  gaia_oauth_client_.reset();

  CheckRobotAccountId(gaia_robot_id);
}

void DeviceOAuth2TokenServiceDelegate::OnOAuthError() {
  gaia_oauth_client_.reset();
  state_ = STATE_TOKEN_INVALID;
  ReportServiceError(GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS);
}

void DeviceOAuth2TokenServiceDelegate::OnNetworkError(int response_code) {
  gaia_oauth_client_.reset();

  // Go back to pending validation state. That'll allow a retry on subsequent
  // token minting requests.
  state_ = STATE_VALIDATION_PENDING;
  ReportServiceError(GoogleServiceAuthError::CONNECTION_FAILED);
}

std::string DeviceOAuth2TokenServiceDelegate::GetRefreshToken(
    const std::string& account_id) const {
  switch (state_) {
    case STATE_LOADING:
    case STATE_NO_TOKEN:
    case STATE_TOKEN_INVALID:
      // This shouldn't happen: GetRefreshToken() is only called for actual
      // token minting operations. In above states, requests are either queued
      // or short-circuited to signal error immediately, so no actual token
      // minting via OAuth2TokenService::FetchOAuth2Token should be triggered.
      NOTREACHED();
      return std::string();
    case STATE_VALIDATION_PENDING:
    case STATE_VALIDATION_STARTED:
    case STATE_TOKEN_VALID:
      return refresh_token_;
  }

  NOTREACHED() << "Unhandled state " << state_;
  return std::string();
}

net::URLRequestContextGetter*
DeviceOAuth2TokenServiceDelegate::GetRequestContext() const {
  return url_request_context_getter_.get();
}

OAuth2AccessTokenFetcher*
DeviceOAuth2TokenServiceDelegate::CreateAccessTokenFetcher(
    const std::string& account_id,
    net::URLRequestContextGetter* getter,
    OAuth2AccessTokenConsumer* consumer) {
  std::string refresh_token = GetRefreshToken(account_id);
  DCHECK(!refresh_token.empty());
  return new OAuth2AccessTokenFetcherImpl(consumer, getter, refresh_token);
}

void DeviceOAuth2TokenServiceDelegate::DidGetSystemSalt(
    const std::string& system_salt) {
  system_salt_ = system_salt;

  // Bail out if system salt is not available.
  if (system_salt_.empty()) {
    LOG(ERROR) << "Failed to get system salt.";
    FlushTokenSaveCallbacks(false);
    state_ = STATE_NO_TOKEN;
    FireRefreshTokensLoaded();
    return;
  }

  // If the token has been set meanwhile, write it to |local_state_|.
  if (!refresh_token_.empty()) {
    EncryptAndSaveToken();
    FireRefreshTokensLoaded();
    return;
  }

  // Otherwise, load the refresh token from |local_state_|.
  std::string encrypted_refresh_token =
      local_state_->GetString(prefs::kDeviceRobotAnyApiRefreshToken);
  if (!encrypted_refresh_token.empty()) {
    CryptohomeTokenEncryptor encryptor(system_salt_);
    refresh_token_ = encryptor.DecryptWithSystemSalt(encrypted_refresh_token);
    if (refresh_token_.empty()) {
      LOG(ERROR) << "Failed to decrypt refresh token.";
      state_ = STATE_NO_TOKEN;
      FireRefreshTokensLoaded();
      return;
    }
  }

  state_ = STATE_VALIDATION_PENDING;

  // If there are pending requests, start a validation.
  if (validation_requested_)
    StartValidation();

  // Announce the token.
  FireRefreshTokenAvailable(GetRobotAccountId());
  FireRefreshTokensLoaded();
}

void DeviceOAuth2TokenServiceDelegate::CheckRobotAccountId(
    const std::string& gaia_robot_id) {
  // Make sure the value returned by GetRobotAccountId has been validated
  // against current device settings.
  switch (CrosSettings::Get()->PrepareTrustedValues(
      base::Bind(&DeviceOAuth2TokenServiceDelegate::CheckRobotAccountId,
                 weak_ptr_factory_.GetWeakPtr(), gaia_robot_id))) {
    case CrosSettingsProvider::TRUSTED:
      // All good, compare account ids below.
      break;
    case CrosSettingsProvider::TEMPORARILY_UNTRUSTED:
      // The callback passed to PrepareTrustedValues above will trigger a
      // re-check eventually.
      return;
    case CrosSettingsProvider::PERMANENTLY_UNTRUSTED:
      // There's no trusted account id, which is equivalent to no token present.
      LOG(WARNING) << "Device settings permanently untrusted.";
      state_ = STATE_NO_TOKEN;
      ReportServiceError(GoogleServiceAuthError::USER_NOT_SIGNED_UP);
      return;
  }

  std::string policy_robot_id = GetRobotAccountId();
  if (policy_robot_id == gaia_robot_id) {
    state_ = STATE_TOKEN_VALID;
    ReportServiceError(GoogleServiceAuthError::NONE);
  } else {
    if (gaia_robot_id.empty()) {
      LOG(WARNING) << "Device service account owner in policy is empty.";
    } else {
      LOG(WARNING) << "Device service account owner in policy does not match "
                   << "refresh token owner \"" << gaia_robot_id << "\".";
    }
    state_ = STATE_TOKEN_INVALID;
    ReportServiceError(GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS);
  }
}

void DeviceOAuth2TokenServiceDelegate::EncryptAndSaveToken() {
  DCHECK_NE(state_, STATE_LOADING);

  CryptohomeTokenEncryptor encryptor(system_salt_);
  std::string encrypted_refresh_token =
      encryptor.EncryptWithSystemSalt(refresh_token_);
  bool result = true;
  if (encrypted_refresh_token.empty()) {
    LOG(ERROR) << "Failed to encrypt refresh token; save aborted.";
    result = false;
  } else {
    local_state_->SetString(prefs::kDeviceRobotAnyApiRefreshToken,
                            encrypted_refresh_token);
  }

  FlushTokenSaveCallbacks(result);
}

void DeviceOAuth2TokenServiceDelegate::StartValidation() {
  DCHECK_EQ(state_, STATE_VALIDATION_PENDING);
  DCHECK(!gaia_oauth_client_);

  state_ = STATE_VALIDATION_STARTED;

  gaia_oauth_client_.reset(
      new gaia::GaiaOAuthClient(g_browser_process->system_request_context()));

  GaiaUrls* gaia_urls = GaiaUrls::GetInstance();
  gaia::OAuthClientInfo client_info;
  client_info.client_id = gaia_urls->oauth2_chrome_client_id();
  client_info.client_secret = gaia_urls->oauth2_chrome_client_secret();

  gaia_oauth_client_->RefreshToken(
      client_info, refresh_token_,
      std::vector<std::string>(1, GaiaConstants::kOAuthWrapBridgeUserInfoScope),
      max_refresh_token_validation_retries_, this);
}

void DeviceOAuth2TokenServiceDelegate::FlushTokenSaveCallbacks(bool result) {
  std::vector<StatusCallback> callbacks;
  callbacks.swap(token_save_callbacks_);
  for (std::vector<StatusCallback>::iterator callback(callbacks.begin());
       callback != callbacks.end(); ++callback) {
    if (!callback->is_null())
      callback->Run(result);
  }
}

void DeviceOAuth2TokenServiceDelegate::RequestValidation() {
  validation_requested_ = true;
}

void DeviceOAuth2TokenServiceDelegate::SetValidationStatusDelegate(
    ValidationStatusDelegate* delegate) {
  validation_status_delegate_ = delegate;
}

void DeviceOAuth2TokenServiceDelegate::ReportServiceError(
    GoogleServiceAuthError::State error) {
  if (validation_status_delegate_) {
    validation_status_delegate_->OnValidationCompleted(error);
  }
}

}  // namespace chromeos
