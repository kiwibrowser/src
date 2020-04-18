// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/android_management_client.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/guid.h"
#include "base/logging.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace em = enterprise_management;

namespace policy {

AndroidManagementClient::AndroidManagementClient(
    DeviceManagementService* device_management_service,
    scoped_refptr<net::URLRequestContextGetter> request_context,
    const std::string& account_id,
    OAuth2TokenService* token_service)
    : OAuth2TokenService::Consumer("android_management_client"),
      device_management_service_(device_management_service),
      request_context_(request_context),
      account_id_(account_id),
      token_service_(token_service),
      weak_ptr_factory_(this) {}

AndroidManagementClient::~AndroidManagementClient() {}

void AndroidManagementClient::StartCheckAndroidManagement(
    const StatusCallback& callback) {
  DCHECK(device_management_service_);
  DCHECK(callback_.is_null());

  callback_ = callback;
  RequestAccessToken();
}

void AndroidManagementClient::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  DCHECK_EQ(token_request_.get(), request);
  token_request_.reset();

  CheckAndroidManagement(access_token);
}

void AndroidManagementClient::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  DCHECK_EQ(token_request_.get(), request);
  DCHECK(!callback_.is_null());
  token_request_.reset();
  LOG(ERROR) << "Token request failed: " << error.ToString();

  base::ResetAndReturn(&callback_).Run(Result::ERROR);
}

void AndroidManagementClient::RequestAccessToken() {
  DCHECK(!token_request_);
  // The user must be signed in already.
  DCHECK(token_service_->RefreshTokenIsAvailable(account_id_));

  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(GaiaConstants::kDeviceManagementServiceOAuth);
  scopes.insert(GaiaConstants::kOAuthWrapBridgeUserInfoScope);
  token_request_ = token_service_->StartRequest(account_id_, scopes, this);
}

void AndroidManagementClient::CheckAndroidManagement(
    const std::string& access_token) {
  request_job_.reset(device_management_service_->CreateJob(
      DeviceManagementRequestJob::TYPE_ANDROID_MANAGEMENT_CHECK,
      request_context_.get()));
  request_job_->SetOAuthToken(access_token);
  request_job_->SetClientID(base::GenerateGUID());
  request_job_->GetRequest()->mutable_check_android_management_request();

  request_job_->Start(
      base::Bind(&AndroidManagementClient::OnAndroidManagementChecked,
                 weak_ptr_factory_.GetWeakPtr()));
}

void AndroidManagementClient::OnAndroidManagementChecked(
    DeviceManagementStatus status,
    int net_error,
    const em::DeviceManagementResponse& response) {
  DCHECK(!callback_.is_null());
  if (status == DM_STATUS_SUCCESS &&
      !response.has_check_android_management_response()) {
    LOG(WARNING) << "Invalid check android management response.";
    status = DM_STATUS_RESPONSE_DECODING_ERROR;
  }

  Result result;
  switch (status) {
    case DM_STATUS_SUCCESS:
      result = Result::UNMANAGED;
      break;
    case DM_STATUS_SERVICE_DEVICE_ID_CONFLICT:
      result = Result::MANAGED;
      break;
    default:
      result = Result::ERROR;
  }

  request_job_.reset();
  base::ResetAndReturn(&callback_).Run(result);
}

std::ostream& operator<<(std::ostream& os,
                         AndroidManagementClient::Result result) {
  switch (result) {
    case AndroidManagementClient::Result::MANAGED:
      return os << "MANAGED";
    case AndroidManagementClient::Result::UNMANAGED:
      return os << "UNMANAGED";
    case AndroidManagementClient::Result::ERROR:
      return os << "ERROR";
  }
  NOTREACHED();
  return os;
}

}  // namespace policy
