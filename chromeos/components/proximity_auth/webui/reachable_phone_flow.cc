// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/proximity_auth/webui/reachable_phone_flow.h"

#include <algorithm>

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "components/cryptauth/cryptauth_client.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace proximity_auth {

namespace {

// The time, in milliseconds, to wait for phones to respond to the CryptAuth
// ping before querying for reachable devices.
const int kWaitTimeMillis = 7000;

}  // namespace

ReachablePhoneFlow::ReachablePhoneFlow(
    cryptauth::CryptAuthClientFactory* client_factory)
    : client_factory_(client_factory), weak_ptr_factory_(this) {}

ReachablePhoneFlow::~ReachablePhoneFlow() {}

void ReachablePhoneFlow::Run(const ReachablePhonesCallback& callback) {
  if (!callback_.is_null()) {
    PA_LOG(ERROR) << "Flow already started.";
    callback.Run(std::vector<cryptauth::ExternalDeviceInfo>());
    return;
  }

  callback_ = callback;
  client_ = client_factory_->CreateInstance();

  // Ping the user's devices to update themselves with CryptAuth.
  cryptauth::SendDeviceSyncTickleRequest tickle_request;
  tickle_request.set_tickle_type(cryptauth::UPDATE_ENROLLMENT);
  net::PartialNetworkTrafficAnnotationTag partial_traffic_annotation =
      net::DefinePartialNetworkTrafficAnnotation("cryptauth_device_sync_tickle",
                                                 "oauth2_api_call_flow", R"(
      semantics {
        sender: "EasyUnlock Debug UI"
        description:
          "Triggers a sync on all other device (for the same user) registered "
          "on CryptAuth."
        trigger: "User manually opens the EasyUnlock debug UI."
        data: "OAuth 2.0 token and the device public key."
        destination: GOOGLE_OWNED_SERVICE
      }
      policy {
        setting:
          "This feature cannot be disabled in settings, but this request will "
          "only be sent if the user opens the EasyUnlock debug UI."
        chrome_policy {
          SigninAllowed {
            SigninAllowed: false
          }
        }
      })");
  client_->SendDeviceSyncTickle(
      tickle_request,
      base::Bind(&ReachablePhoneFlow::OnSyncTickleSuccess,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&ReachablePhoneFlow::OnApiCallError,
                 weak_ptr_factory_.GetWeakPtr()),
      partial_traffic_annotation);
}

void ReachablePhoneFlow::OnSyncTickleSuccess(
    const cryptauth::SendDeviceSyncTickleResponse& response) {
  PA_LOG(INFO) << "Waiting " << kWaitTimeMillis
               << "ms for phones to callback to CryptAuth...";
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ReachablePhoneFlow::QueryReachablePhones,
                     weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(kWaitTimeMillis));
}

void ReachablePhoneFlow::QueryReachablePhones() {
  // Ask CryptAuth for the devices that updated themselves within the last
  // |kWaitTimeMillis| milliseconds.
  client_ = client_factory_->CreateInstance();
  cryptauth::FindEligibleUnlockDevicesRequest find_devices_request;
  find_devices_request.set_max_last_update_time_delta_millis(kWaitTimeMillis);
  client_->FindEligibleUnlockDevices(
      find_devices_request,
      base::Bind(&ReachablePhoneFlow::OnFindEligibleUnlockDevicesSuccess,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&ReachablePhoneFlow::OnApiCallError,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ReachablePhoneFlow::OnFindEligibleUnlockDevicesSuccess(
    const cryptauth::FindEligibleUnlockDevicesResponse& response) {
  PA_LOG(INFO) << "Found " << response.eligible_devices_size()
               << " reachable phone(s).";
  std::vector<cryptauth::ExternalDeviceInfo> reachable_phones(
      response.eligible_devices_size());
  std::copy(response.eligible_devices().begin(),
            response.eligible_devices().end(), reachable_phones.begin());
  callback_.Run(reachable_phones);
}

void ReachablePhoneFlow::OnApiCallError(const std::string& error) {
  PA_LOG(ERROR) << "Error making api call: " << error;
  callback_.Run(std::vector<cryptauth::ExternalDeviceInfo>());
}

}  // namespace proximity_auth
