// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAIRING_FAKE_HOST_PAIRING_CONTROLLER_H_
#define COMPONENTS_PAIRING_FAKE_HOST_PAIRING_CONTROLLER_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "components/pairing/host_pairing_controller.h"

namespace pairing_chromeos {

class FakeHostPairingController
    : public HostPairingController,
      public HostPairingController::Observer {
 public:
  typedef HostPairingController::Observer Observer;

  // Config is a comma separated list of key-value pairs separated by colon.
  // Supported options:
  // * async_duration - integer. Default: 3000.
  // * start_after_update - {0,1}. Default: 0.
  // * fail_enrollment - {0,1}. Default: 0.
  // * code - 6 digits or empty string. Default: empty string. If strings is
  // empty, random code is generated.
  // * device_name - string. Default: "Chromebox-01".
  // * domain - string. Default: "example.com".
  explicit FakeHostPairingController(const std::string& config);
  ~FakeHostPairingController() override;

  // Applies given |config| to flow.
  void ApplyConfig(const std::string& config);

 private:
  void ChangeStage(Stage new_stage);
  void ChangeStageLater(Stage new_stage);

  // HostPairingController:
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  Stage GetCurrentStage() override;
  void StartPairing() override;
  std::string GetDeviceName() override;
  std::string GetConfirmationCode() override;
  std::string GetEnrollmentDomain() override;
  void OnNetworkConnectivityChanged(Connectivity connectivity_status) override;
  void OnUpdateStatusChanged(UpdateStatus update_status) override;
  void OnEnrollmentStatusChanged(EnrollmentStatus enrollment_status) override;
  void SetPermanentId(const std::string& permanent_id) override;
  void SetErrorCodeAndMessage(int error_code,
                              const std::string& error_message) override;
  void Reset() override;

  // HostPairingController::Observer:
  void PairingStageChanged(Stage new_stage) override;

  base::ObserverList<Observer> observers_;
  Stage current_stage_;
  std::string device_name_;
  std::string confirmation_code_;
  base::TimeDelta async_duration_;

  // If this flag is true error happens on |STAGE_ENROLLING| once.
  bool enrollment_should_fail_;

  // Controller starts its work like if update and reboot already happened.
  bool start_after_update_;

  std::string enrollment_domain_;

  DISALLOW_COPY_AND_ASSIGN(FakeHostPairingController);
};

}  // namespace pairing_chromeos

#endif  // COMPONENTS_PAIRING_FAKE_HOST_PAIRING_CONTROLLER_H_
