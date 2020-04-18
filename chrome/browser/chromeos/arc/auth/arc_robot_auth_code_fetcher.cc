// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/auth/arc_robot_auth_code_fetcher.h"

#include <string>

#include "base/bind.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/url_request/url_request_context_getter.h"

namespace {

// OAuth2 Client id of Android.
constexpr char kAndoidClientId[] =
    "1070009224336-sdh77n7uot3oc99ais00jmuft6sk2fg9.apps.googleusercontent.com";

policy::DeviceManagementService* GetDeviceManagementService() {
  policy::BrowserPolicyConnectorChromeOS* const connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  return connector->device_management_service();
}

const policy::CloudPolicyClient* GetCloudPolicyClient() {
  const policy::BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  const policy::DeviceCloudPolicyManagerChromeOS* policy_manager =
      connector->GetDeviceCloudPolicyManager();
  return policy_manager->core()->client();
}

}  // namespace

namespace arc {

ArcRobotAuthCodeFetcher::ArcRobotAuthCodeFetcher() : weak_ptr_factory_(this) {}

ArcRobotAuthCodeFetcher::~ArcRobotAuthCodeFetcher() = default;

void ArcRobotAuthCodeFetcher::Fetch(const FetchCallback& callback) {
  DCHECK(!fetch_request_job_);
  const policy::CloudPolicyClient* client = GetCloudPolicyClient();

  policy::DeviceManagementService* service = GetDeviceManagementService();
  fetch_request_job_.reset(service->CreateJob(
      policy::DeviceManagementRequestJob::TYPE_API_AUTH_CODE_FETCH,
      g_browser_process->system_request_context()));

  fetch_request_job_->SetDMToken(client->dm_token());
  fetch_request_job_->SetClientID(client->client_id());

  enterprise_management::DeviceServiceApiAccessRequest* request =
      fetch_request_job_->GetRequest()->mutable_service_api_access_request();
  request->set_oauth2_client_id(kAndoidClientId);
  request->add_auth_scope(GaiaConstants::kAnyApiOAuth2Scope);
  request->set_device_type(
      enterprise_management::DeviceServiceApiAccessRequest::ANDROID_OS);

  fetch_request_job_->Start(
      base::Bind(&ArcRobotAuthCodeFetcher::OnFetchRobotAuthCodeCompleted,
                 weak_ptr_factory_.GetWeakPtr(), callback));
}

void ArcRobotAuthCodeFetcher::OnFetchRobotAuthCodeCompleted(
    FetchCallback callback,
    policy::DeviceManagementStatus status,
    int net_error,
    const enterprise_management::DeviceManagementResponse& response) {
  fetch_request_job_.reset();

  if (status == policy::DM_STATUS_SUCCESS &&
      (!response.has_service_api_access_response())) {
    LOG(WARNING) << "Invalid service api access response.";
    status = policy::DM_STATUS_RESPONSE_DECODING_ERROR;
  }

  if (status != policy::DM_STATUS_SUCCESS) {
    LOG(ERROR) << "Fetching of robot auth code failed. DM Status: " << status;
    callback.Run(false /* success */, std::string());
    return;
  }

  callback.Run(true /* success */,
               response.service_api_access_response().auth_code());
}

}  // namespace arc
