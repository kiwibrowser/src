// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/software_feature_manager_impl.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

// static
SoftwareFeatureManagerImpl::Factory*
    SoftwareFeatureManagerImpl::Factory::test_factory_instance_ = nullptr;

// static
std::unique_ptr<SoftwareFeatureManager>
SoftwareFeatureManagerImpl::Factory::NewInstance(
    CryptAuthClientFactory* cryptauth_client_factory) {
  if (test_factory_instance_)
    return test_factory_instance_->BuildInstance(cryptauth_client_factory);

  static base::NoDestructor<Factory> factory;
  return factory->BuildInstance(cryptauth_client_factory);
}

void SoftwareFeatureManagerImpl::Factory::SetInstanceForTesting(
    Factory* test_factory) {
  test_factory_instance_ = test_factory;
}

SoftwareFeatureManagerImpl::Factory::~Factory() = default;

std::unique_ptr<SoftwareFeatureManager>
SoftwareFeatureManagerImpl::Factory::BuildInstance(
    CryptAuthClientFactory* cryptauth_client_factory) {
  return base::WrapUnique(
      new SoftwareFeatureManagerImpl(cryptauth_client_factory));
}

SoftwareFeatureManagerImpl::Request::Request(
    std::unique_ptr<ToggleEasyUnlockRequest> toggle_request,
    const base::Closure& set_software_success_callback,
    const base::Callback<void(const std::string&)> error_callback)
    : error_callback(error_callback),
      toggle_request(std::move(toggle_request)),
      set_software_success_callback(set_software_success_callback) {}

SoftwareFeatureManagerImpl::Request::Request(
    std::unique_ptr<FindEligibleUnlockDevicesRequest> find_request,
    const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                              const std::vector<IneligibleDevice>&)>
        find_hosts_success_callback,
    const base::Callback<void(const std::string&)> error_callback)
    : error_callback(error_callback),
      find_request(std::move(find_request)),
      find_hosts_success_callback(find_hosts_success_callback) {}

SoftwareFeatureManagerImpl::Request::~Request() = default;

SoftwareFeatureManagerImpl::SoftwareFeatureManagerImpl(
    CryptAuthClientFactory* cryptauth_client_factory)
    : crypt_auth_client_factory_(cryptauth_client_factory),
      weak_ptr_factory_(this) {}

SoftwareFeatureManagerImpl::~SoftwareFeatureManagerImpl() = default;

void SoftwareFeatureManagerImpl::SetSoftwareFeatureState(
    const std::string& public_key,
    SoftwareFeature software_feature,
    bool enabled,
    const base::Closure& success_callback,
    const base::Callback<void(const std::string&)>& error_callback,
    bool is_exclusive) {
  // Note: For legacy reasons, this proto message mentions "ToggleEasyUnlock"
  // instead of "SetSoftwareFeature" in its name.
  auto request = std::make_unique<ToggleEasyUnlockRequest>();
  request->set_public_key(public_key);
  request->set_feature(software_feature);
  request->set_enable(enabled);
  request->set_is_exclusive(enabled && is_exclusive);

  // Special case for EasyUnlock: if EasyUnlock is being disabled, set the
  // apply_to_all property to true.
  request->set_apply_to_all(!enabled && software_feature ==
                                            SoftwareFeature::EASY_UNLOCK_HOST);

  pending_requests_.emplace(std::make_unique<Request>(
      std::move(request), success_callback, error_callback));
  ProcessRequestQueue();
}

void SoftwareFeatureManagerImpl::FindEligibleDevices(
    SoftwareFeature software_feature,
    const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                              const std::vector<IneligibleDevice>&)>&
        success_callback,
    const base::Callback<void(const std::string&)>& error_callback) {
  // Note: For legacy reasons, this proto message mentions "UnlockDevices"
  // instead of "MultiDeviceHosts" in its name.
  auto request = std::make_unique<FindEligibleUnlockDevicesRequest>();
  request->set_feature(software_feature);

  pending_requests_.emplace(std::make_unique<Request>(
      std::move(request), success_callback, error_callback));
  ProcessRequestQueue();
}

void SoftwareFeatureManagerImpl::ProcessRequestQueue() {
  if (current_request_ || pending_requests_.empty())
    return;

  current_request_ = std::move(pending_requests_.front());
  pending_requests_.pop();

  if (current_request_->toggle_request)
    ProcessSetSoftwareFeatureStateRequest();
  else
    ProcessFindEligibleDevicesRequest();
}

void SoftwareFeatureManagerImpl::ProcessSetSoftwareFeatureStateRequest() {
  DCHECK(!current_cryptauth_client_);
  current_cryptauth_client_ = crypt_auth_client_factory_->CreateInstance();

  current_cryptauth_client_->ToggleEasyUnlock(
      *current_request_->toggle_request,
      base::Bind(&SoftwareFeatureManagerImpl::OnToggleEasyUnlockResponse,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&SoftwareFeatureManagerImpl::OnErrorResponse,
                 weak_ptr_factory_.GetWeakPtr()));
}

void SoftwareFeatureManagerImpl::ProcessFindEligibleDevicesRequest() {
  DCHECK(!current_cryptauth_client_);
  current_cryptauth_client_ = crypt_auth_client_factory_->CreateInstance();

  current_cryptauth_client_->FindEligibleUnlockDevices(
      *current_request_->find_request,
      base::Bind(
          &SoftwareFeatureManagerImpl::OnFindEligibleUnlockDevicesResponse,
          weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&SoftwareFeatureManagerImpl::OnErrorResponse,
                 weak_ptr_factory_.GetWeakPtr()));
}

void SoftwareFeatureManagerImpl::OnToggleEasyUnlockResponse(
    const ToggleEasyUnlockResponse& response) {
  current_cryptauth_client_.reset();
  current_request_->set_software_success_callback.Run();
  current_request_.reset();
  ProcessRequestQueue();
}

void SoftwareFeatureManagerImpl::OnFindEligibleUnlockDevicesResponse(
    const FindEligibleUnlockDevicesResponse& response) {
  current_cryptauth_client_.reset();
  current_request_->find_hosts_success_callback.Run(
      std::vector<ExternalDeviceInfo>(response.eligible_devices().begin(),
                                      response.eligible_devices().end()),
      std::vector<IneligibleDevice>(response.ineligible_devices().begin(),
                                    response.ineligible_devices().end()));
  current_request_.reset();
  ProcessRequestQueue();
}

void SoftwareFeatureManagerImpl::OnErrorResponse(const std::string& response) {
  current_cryptauth_client_.reset();
  current_request_->error_callback.Run(response);
  current_request_.reset();
  ProcessRequestQueue();
}

}  // namespace cryptauth
