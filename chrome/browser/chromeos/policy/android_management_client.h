// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_ANDROID_MANAGEMENT_CLIENT_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_ANDROID_MANAGEMENT_CLIENT_H_

#include <memory>
#include <ostream>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "net/url_request/url_request_context_getter.h"

namespace enterprise_management {
class DeviceManagementResponse;
}

namespace policy {

class DeviceManagementRequestJob;
class DeviceManagementService;

// Interacts with the device management service and determines whether Android
// management is enabled for the user or not. Uses the OAuth2TokenService to
// acquire access tokens for the device management.
class AndroidManagementClient : public OAuth2TokenService::Consumer {
 public:
  // Indicates result of the android management check.
  enum class Result {
    MANAGED,    // Android management is enabled.
    UNMANAGED,  // Android management is disabled.
    ERROR,      // Received a error.
  };

  // A callback which receives Result status of an operation.
  using StatusCallback = base::Callback<void(Result)>;

  AndroidManagementClient(
      DeviceManagementService* service,
      scoped_refptr<net::URLRequestContextGetter> request_context,
      const std::string& account_id,
      OAuth2TokenService* token_service);
  ~AndroidManagementClient() override;

  // Starts sending of check Android management request to DM server, issues
  // access token if neccessary. |callback| is called on check Android
  // management completion.
  void StartCheckAndroidManagement(const StatusCallback& callback);

  // |access_token| is owned by caller and must exist before
  // StartCheckAndroidManagement is called for testing.
  static void SetAccessTokenForTesting(const char* access_token);

 private:
  // OAuth2TokenService::Consumer:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  // Requests an access token.
  void RequestAccessToken();

  // Sends a CheckAndroidManagementRequest to DM server.
  void CheckAndroidManagement(const std::string& access_token);

  // Callback for check Android management requests.
  void OnAndroidManagementChecked(
      DeviceManagementStatus status,
      int net_error,
      const enterprise_management::DeviceManagementResponse& response);

  // Used to communicate with the device management service.
  DeviceManagementService* const device_management_service_;
  scoped_refptr<net::URLRequestContextGetter> request_context_;
  std::unique_ptr<DeviceManagementRequestJob> request_job_;

  // The account ID that will be used for the access token fetch.
  const std::string account_id_;
  // The token service used to retrieve the access token.
  OAuth2TokenService* const token_service_;
  // The OAuth request to receive the access token.
  std::unique_ptr<OAuth2TokenService::Request> token_request_;

  StatusCallback callback_;

  base::WeakPtrFactory<AndroidManagementClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AndroidManagementClient);
};

// Outputs the stringified |result| to |os|. This is only for logging purposes.
std::ostream& operator<<(std::ostream& os,
                         AndroidManagementClient::Result result);

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_ANDROID_MANAGEMENT_CLIENT_H_
