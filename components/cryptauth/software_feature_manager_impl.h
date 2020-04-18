// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_SOFTWARE_FEATURE_MANAGER_IMPL_H_
#define COMPONENTS_CRYPTAUTH_SOFTWARE_FEATURE_MANAGER_IMPL_H_

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/containers/queue.h"
#include "base/memory/weak_ptr.h"
#include "components/cryptauth/cryptauth_client.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/software_feature_manager.h"

namespace cryptauth {

// Concrete SoftwareFeatureManager implementation. To query and/or set
// MultiDevice hosts, this class makes network requests to the CryptAuth
// back-end.
class SoftwareFeatureManagerImpl : public SoftwareFeatureManager {
 public:
  class Factory {
   public:
    static std::unique_ptr<SoftwareFeatureManager> NewInstance(
        CryptAuthClientFactory* cryptauth_client_factory);

    static void SetInstanceForTesting(Factory* test_factory);

   protected:
    virtual ~Factory();
    virtual std::unique_ptr<SoftwareFeatureManager> BuildInstance(
        CryptAuthClientFactory* cryptauth_client_factory);

   private:
    static Factory* test_factory_instance_;
  };

  ~SoftwareFeatureManagerImpl() override;

  // SoftwareFeatureManager:
  void SetSoftwareFeatureState(
      const std::string& public_key,
      SoftwareFeature software_feature,
      bool enabled,
      const base::Closure& success_callback,
      const base::Callback<void(const std::string&)>& error_callback,
      bool is_exclusive = false) override;
  void FindEligibleDevices(
      SoftwareFeature software_feature,
      const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                const std::vector<IneligibleDevice>&)>&
          success_callback,
      const base::Callback<void(const std::string&)>& error_callback) override;

 private:
  enum class RequestType {
    SET_SOFTWARE_FEATURE,
    FIND_ELIGIBLE_MULTIDEVICE_HOSTS,
  };

  struct Request {
    // Used for SET_SOFTWARE_FEATURE Requests.
    Request(std::unique_ptr<ToggleEasyUnlockRequest> toggle_request,
            const base::Closure& set_software_success_callback,
            const base::Callback<void(const std::string&)> error_callback);

    // Used for FIND_ELIGIBLE_MULTIDEVICE_HOSTS Requests.
    Request(std::unique_ptr<FindEligibleUnlockDevicesRequest> find_request,
            const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                      const std::vector<IneligibleDevice>&)>
                find_hosts_success_callback,
            const base::Callback<void(const std::string&)> error_callback);

    ~Request();

    const base::Callback<void(const std::string&)> error_callback;

    // Set for SET_SOFTWARE_FEATURE; unset otherwise.
    std::unique_ptr<ToggleEasyUnlockRequest> toggle_request;
    const base::Closure set_software_success_callback;

    // Set for FIND_ELIGIBLE_MULTIDEVICE_HOSTS; unset otherwise.
    std::unique_ptr<FindEligibleUnlockDevicesRequest> find_request;
    const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                              const std::vector<IneligibleDevice>&)>
        find_hosts_success_callback;
  };

  SoftwareFeatureManagerImpl(CryptAuthClientFactory* cryptauth_client_factory);

  void ProcessRequestQueue();
  void ProcessSetSoftwareFeatureStateRequest();
  void ProcessFindEligibleDevicesRequest();

  void OnToggleEasyUnlockResponse(const ToggleEasyUnlockResponse& response);
  void OnFindEligibleUnlockDevicesResponse(
      const FindEligibleUnlockDevicesResponse& response);
  void OnErrorResponse(const std::string& response);

  CryptAuthClientFactory* crypt_auth_client_factory_;

  std::unique_ptr<CryptAuthClient> current_cryptauth_client_;
  std::unique_ptr<Request> current_request_;
  base::queue<std::unique_ptr<Request>> pending_requests_;

  base::WeakPtrFactory<SoftwareFeatureManagerImpl> weak_ptr_factory_;
};
}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_SOFTWARE_FEATURE_MANAGER_IMPL_H_
