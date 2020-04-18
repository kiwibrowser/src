// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_FAKE_SOFTWARE_FEATURE_MANAGER_H_
#define COMPONENTS_CRYPTAUTH_FAKE_SOFTWARE_FEATURE_MANAGER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/software_feature_manager.h"

namespace cryptauth {

// Test implementation of SoftwareFeatureManager.
class FakeSoftwareFeatureManager : public SoftwareFeatureManager {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void OnSetSoftwareFeatureStateCalled() {}
    virtual void OnFindEligibleDevicesCalled() {}
  };

  struct SetSoftwareFeatureStateArgs {
    SetSoftwareFeatureStateArgs(
        const std::string& public_key,
        SoftwareFeature software_feature,
        bool enabled,
        const base::Closure& success_callback,
        const base::Callback<void(const std::string&)>& error_callback,
        bool is_exclusive);
    ~SetSoftwareFeatureStateArgs();

    std::string public_key;
    SoftwareFeature software_feature;
    bool enabled;
    base::Closure success_callback;
    base::Callback<void(const std::string&)> error_callback;
    bool is_exclusive;

   private:
    DISALLOW_COPY_AND_ASSIGN(SetSoftwareFeatureStateArgs);
  };

  struct FindEligibleDevicesArgs {
    FindEligibleDevicesArgs(
        SoftwareFeature software_feature,
        const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                  const std::vector<IneligibleDevice>&)>&
            success_callback,
        const base::Callback<void(const std::string&)>& error_callback);
    ~FindEligibleDevicesArgs();

    SoftwareFeature software_feature;
    base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                        const std::vector<IneligibleDevice>&)>
        success_callback;
    base::Callback<void(const std::string&)> error_callback;

   private:
    DISALLOW_COPY_AND_ASSIGN(FindEligibleDevicesArgs);
  };

  FakeSoftwareFeatureManager();
  ~FakeSoftwareFeatureManager() override;

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  const std::vector<std::unique_ptr<SetSoftwareFeatureStateArgs>>&
  set_software_feature_state_calls() {
    return set_software_feature_state_calls_;
  }

  const std::vector<std::unique_ptr<FindEligibleDevicesArgs>>&
  find_eligible_multidevice_host_calls() {
    return find_eligible_multidevice_host_calls_;
  }

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
  Delegate* delegate_ = nullptr;

  std::vector<std::unique_ptr<SetSoftwareFeatureStateArgs>>
      set_software_feature_state_calls_;
  std::vector<std::unique_ptr<FindEligibleDevicesArgs>>
      find_eligible_multidevice_host_calls_;

  DISALLOW_COPY_AND_ASSIGN(FakeSoftwareFeatureManager);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_FAKE_SOFTWARE_FEATURE_MANAGER_H_
