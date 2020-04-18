// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OAUTH2_TOKEN_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OAUTH2_TOKEN_SERVICE_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/settings/device_oauth2_token_service_delegate.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "net/url_request/url_request_context_getter.h"

namespace net {
class URLRequestContextGetter;
}

class PrefRegistrySimple;

namespace chromeos {

// DeviceOAuth2TokenService retrieves OAuth2 access tokens for a given
// set of scopes using the device-level OAuth2 any-api refresh token
// obtained during enterprise device enrollment.
//
// See |OAuth2TokenService| for usage details.
//
// When using DeviceOAuth2TokenService, a value of |GetRobotAccountId| should
// be used in places where API expects |account_id|.
//
// Note that requests must be made from the UI thread.
class DeviceOAuth2TokenService
    : public OAuth2TokenService,
      public DeviceOAuth2TokenServiceDelegate::ValidationStatusDelegate {
 public:
  typedef base::Callback<void(bool)> StatusCallback;

  // Persist the given refresh token on the device. Overwrites any previous
  // value. Should only be called during initial device setup. Signals
  // completion via the given callback, passing true if the operation succeeded.
  void SetAndSaveRefreshToken(const std::string& refresh_token,
                              const StatusCallback& callback);

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Pull the robot account ID from device policy.
  virtual std::string GetRobotAccountId() const;

 protected:
  // Implementation of OAuth2TokenService.
  void FetchOAuth2Token(RequestImpl* request,
                        const std::string& account_id,
                        net::URLRequestContextGetter* getter,
                        const std::string& client_id,
                        const std::string& client_secret,
                        const ScopeSet& scopes) override;
 private:
  friend class DeviceOAuth2TokenServiceFactory;
  friend class DeviceOAuth2TokenServiceTest;
  struct PendingRequest;

  // Implementation of
  // DeviceOAuth2TokenServiceDelegate::ValidationStatusDelegate.
  void OnValidationCompleted(GoogleServiceAuthError::State error) override;

  // Use DeviceOAuth2TokenServiceFactory to get an instance of this class.
  explicit DeviceOAuth2TokenService(
      std::unique_ptr<DeviceOAuth2TokenServiceDelegate> delegate);
  ~DeviceOAuth2TokenService() override;

  // Flushes |pending_requests_|, indicating the specified result.
  void FlushPendingRequests(bool token_is_valid,
                            GoogleServiceAuthError::State error);

  // Signals failure on the specified request, passing |error| as the reason.
  void FailRequest(RequestImpl* request, GoogleServiceAuthError::State error);

  DeviceOAuth2TokenServiceDelegate* GetDeviceDelegate();
  const DeviceOAuth2TokenServiceDelegate* GetDeviceDelegate() const;

  // Currently open requests that are waiting while loading the system salt or
  // validating the token.
  std::vector<PendingRequest*> pending_requests_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOAuth2TokenService);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OAUTH2_TOKEN_SERVICE_H_
