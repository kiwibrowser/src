// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_PROXIMITY_AUTH_WEBUI_REACHABLE_PHONE_FLOW_H_
#define CHROMEOS_COMPONENTS_PROXIMITY_AUTH_WEBUI_REACHABLE_PHONE_FLOW_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace cryptauth {
class CryptAuthClient;
class CryptAuthClientFactory;
class ExternalDeviceInfo;
class FindEligibleUnlockDevicesResponse;
class SendDeviceSyncTickleResponse;
};  // namespace cryptauth

namespace proximity_auth {

// Run this flow to find the user's phones that actively respond to a CryptAuth
// ping. We are confident that phones responding to the ping are currently
// online and immediately reachable.
class ReachablePhoneFlow {
 public:
  // Creates the ReachablePhoneFlow instance:
  // |client_factory|: Factory for creating CryptAuthClient instances. Not owned
  //     and must outlive |this| instance.
  explicit ReachablePhoneFlow(
      cryptauth::CryptAuthClientFactory* client_factory);

  ~ReachablePhoneFlow();

  // Starts the flow and invokes |callback| with the reachable devices upon
  // completion. If the flow fails, |callback| will be invoked with an empty
  // vector. Do not reuse this class after calling |Run()|, but instead create a
  // new instance.
  typedef base::Callback<void(
      const std::vector<cryptauth::ExternalDeviceInfo>&)>
      ReachablePhonesCallback;
  void Run(const ReachablePhonesCallback& callback);

 private:
  // Callback when a CryptAuth API fails.
  void OnApiCallError(const std::string& error);

  // Callback for the SyncTickle CryptAuth request.
  void OnSyncTickleSuccess(
      const cryptauth::SendDeviceSyncTickleResponse& response);

  // Makes the CryptAuth request to get the phones that responded to the ping.
  void QueryReachablePhones();

  // Callback for the FindEligibleUnlockDevicesResponse CryptAuth request.
  void OnFindEligibleUnlockDevicesSuccess(
      const cryptauth::FindEligibleUnlockDevicesResponse& response);

  // Factory for creating CryptAuthClient instances. Not owned and must outlive
  // |this| instance.
  cryptauth::CryptAuthClientFactory* client_factory_;

  // Callback invoked when the flow completes.
  ReachablePhonesCallback callback_;

  // The client making the current CryptAuth API call.
  std::unique_ptr<cryptauth::CryptAuthClient> client_;

  base::WeakPtrFactory<ReachablePhoneFlow> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ReachablePhoneFlow);
};

}  // namespace proximity_auth

#endif  // CHROMEOS_COMPONENTS_PROXIMITY_AUTH_WEBUI_REACHABLE_PHONE_FLOW_H_
