// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/settings/device_oauth2_token_service.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace chromeos {

struct DeviceOAuth2TokenService::PendingRequest {
  PendingRequest(const base::WeakPtr<RequestImpl>& request,
                 const std::string& client_id,
                 const std::string& client_secret,
                 const ScopeSet& scopes)
      : request(request),
        client_id(client_id),
        client_secret(client_secret),
        scopes(scopes) {}

  const base::WeakPtr<RequestImpl> request;
  const std::string client_id;
  const std::string client_secret;
  const ScopeSet scopes;
};

void DeviceOAuth2TokenService::OnValidationCompleted(
    GoogleServiceAuthError::State error) {
  if (error == GoogleServiceAuthError::NONE)
    FlushPendingRequests(true, GoogleServiceAuthError::NONE);
  else
    FlushPendingRequests(false, error);
}

DeviceOAuth2TokenService::DeviceOAuth2TokenService(
    std::unique_ptr<DeviceOAuth2TokenServiceDelegate> delegate)
    : OAuth2TokenService(std::move(delegate)) {
  GetDeviceDelegate()->SetValidationStatusDelegate(this);
}

DeviceOAuth2TokenService::~DeviceOAuth2TokenService() {
  GetDeviceDelegate()->SetValidationStatusDelegate(nullptr);
  FlushPendingRequests(false, GoogleServiceAuthError::REQUEST_CANCELED);
}

// static
void DeviceOAuth2TokenService::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kDeviceRobotAnyApiRefreshToken,
                               std::string());
}

void DeviceOAuth2TokenService::SetAndSaveRefreshToken(
    const std::string& refresh_token,
    const StatusCallback& result_callback) {
  GetDeviceDelegate()->SetAndSaveRefreshToken(refresh_token, result_callback);
}

std::string DeviceOAuth2TokenService::GetRobotAccountId() const {
  return GetDeviceDelegate()->GetRobotAccountId();
}

void DeviceOAuth2TokenService::FetchOAuth2Token(
    RequestImpl* request,
    const std::string& account_id,
    net::URLRequestContextGetter* getter,
    const std::string& client_id,
    const std::string& client_secret,
    const ScopeSet& scopes) {
  switch (GetDeviceDelegate()->state_) {
    case DeviceOAuth2TokenServiceDelegate::STATE_VALIDATION_PENDING:
      // If this is the first request for a token, start validation.
      GetDeviceDelegate()->StartValidation();
      FALLTHROUGH;
    case DeviceOAuth2TokenServiceDelegate::STATE_LOADING:
    case DeviceOAuth2TokenServiceDelegate::STATE_VALIDATION_STARTED:
      // Add a pending request that will be satisfied once validation completes.
      pending_requests_.push_back(new PendingRequest(
          request->AsWeakPtr(), client_id, client_secret, scopes));
      GetDeviceDelegate()->RequestValidation();
      return;
    case DeviceOAuth2TokenServiceDelegate::STATE_NO_TOKEN:
      FailRequest(request, GoogleServiceAuthError::USER_NOT_SIGNED_UP);
      return;
    case DeviceOAuth2TokenServiceDelegate::STATE_TOKEN_INVALID:
      FailRequest(request, GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS);
      return;
    case DeviceOAuth2TokenServiceDelegate::STATE_TOKEN_VALID:
      // Pass through to OAuth2TokenService to satisfy the request.
      OAuth2TokenService::FetchOAuth2Token(
          request, account_id, getter, client_id, client_secret, scopes);
      return;
  }

  NOTREACHED() << "Unexpected state " << GetDeviceDelegate()->state_;
}

void DeviceOAuth2TokenService::FlushPendingRequests(
    bool token_is_valid,
    GoogleServiceAuthError::State error) {
  std::vector<PendingRequest*> requests;
  requests.swap(pending_requests_);
  for (std::vector<PendingRequest*>::iterator request(requests.begin());
       request != requests.end();
       ++request) {
    std::unique_ptr<PendingRequest> scoped_request(*request);
    if (!scoped_request->request)
      continue;

    if (token_is_valid) {
      OAuth2TokenService::FetchOAuth2Token(
          scoped_request->request.get(),
          scoped_request->request->GetAccountId(),
          GetDeviceDelegate()->GetRequestContext(), scoped_request->client_id,
          scoped_request->client_secret, scoped_request->scopes);
    } else {
      FailRequest(scoped_request->request.get(), error);
    }
  }
}

void DeviceOAuth2TokenService::FailRequest(
    RequestImpl* request,
    GoogleServiceAuthError::State error) {
  GoogleServiceAuthError auth_error =
      (error == GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS)
          ? GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
                GoogleServiceAuthError::InvalidGaiaCredentialsReason::
                    CREDENTIALS_REJECTED_BY_SERVER)
          : GoogleServiceAuthError(error);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&RequestImpl::InformConsumer, request->AsWeakPtr(),
                            auth_error, std::string(), base::Time()));
}

DeviceOAuth2TokenServiceDelegate*
DeviceOAuth2TokenService::GetDeviceDelegate() {
  return static_cast<DeviceOAuth2TokenServiceDelegate*>(GetDelegate());
}

const DeviceOAuth2TokenServiceDelegate*
DeviceOAuth2TokenService::GetDeviceDelegate() const {
  return static_cast<const DeviceOAuth2TokenServiceDelegate*>(GetDelegate());
}

}  // namespace chromeos
