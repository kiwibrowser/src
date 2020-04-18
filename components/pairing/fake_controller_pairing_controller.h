// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAIRING_FAKE_CONTROLLER_PAIRING_CONTROLLER_H_
#define COMPONENTS_PAIRING_FAKE_CONTROLLER_PAIRING_CONTROLLER_H_

#include <stddef.h>

#include <set>
#include <utility>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "components/pairing/controller_pairing_controller.h"

namespace pairing_chromeos {

class FakeControllerPairingController
    : public ControllerPairingController,
      public ControllerPairingController::Observer {
 public:
  typedef ControllerPairingController::Observer Observer;

  enum DiscoveryEventType { DEVICE_FOUND, DEVICE_LOST, NOTHING_FOUND };

  typedef std::pair<DiscoveryEventType, std::string> DiscoveryEvent;
  typedef std::vector<DiscoveryEvent> DiscoveryScenario;

  // Config is a coma separated list of key-value pairs separated by colon.
  // Supported options:
  // * async_duration - integer. Default: 3000.
  // * fail_connecting - {0,1}. Default: 0.
  // * connection_lost - integer-integer. Default: 0-0.
  // * fail_enrollment - {0,1}. Default: 0.
  // * discovery - {F,L,N}-string(~{F,L,N}-string)*. Default:
  //   F-Device_1~F-Device_5~F-Device_3~L-Device_3~L-Device_1~F-Device_1
  // * code - 6 digits or empty string. Default: empty string. If strings is
  // empty, random code is generated.
  explicit FakeControllerPairingController(const std::string& config);
  ~FakeControllerPairingController() override;

  // Applies given |config| to controller.
  void ApplyConfig(const std::string& config);

  // Sets delay for asynchronous operations. like device searching or host
  // enrollment. Default value is 3 seconds.
  void set_async_duration(base::TimeDelta async_duration) {
    async_duration_ = async_duration;
  }

  // Causing an error on |STAGE_ESTABLISHING_CONNECTION| stage once.
  void SetShouldFailOnConnecting();

  // Causing a connection loss on |stage_begin| and a connection recovery
  // on |stage_end| once.
  void SetShouldLoseConnection(Stage stage_begin, Stage stage_end);

  // Makes host enrollment to fail once.
  void SetEnrollmentShouldFail();

  // Sets devices discovery scenario for |STAGE_DEVICES_DESCOVERY| stage.
  // Events are executed with interval of |async_duration_|.
  // For default scenario refer to implementation.
  void SetDiscoveryScenario(const DiscoveryScenario& discovery_scenario);

  // Overridden from ControllerPairingController:
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  Stage GetCurrentStage() override;
  void StartPairing() override;
  DeviceIdList GetDiscoveredDevices() override;
  void ChooseDeviceForPairing(const std::string& device_id) override;
  void RepeatDiscovery() override;
  std::string GetConfirmationCode() override;
  void SetConfirmationCodeIsCorrect(bool correct) override;
  void SetHostNetwork(const std::string& onc_spec) override;
  void SetHostConfiguration(bool accepted_eula,
                            const std::string& lang,
                            const std::string& timezone,
                            bool send_reports,
                            const std::string& keyboard_layout) override;
  void OnAuthenticationDone(const std::string& domain,
                            const std::string& auth_token) override;
  void StartSession() override;

 private:
  void ChangeStage(Stage new_stage);
  void ChangeStageLater(Stage new_stage);
  void ExecuteDiscoveryEvent(size_t event_position);
  void DeviceFound(const std::string& device_id);
  void DeviceLost(const std::string& device_id);

  // Overridden from ui::ControllerPairingController::Observer:
  void PairingStageChanged(Stage new_stage) override;
  void DiscoveredDevicesListChanged() override;

  base::ObserverList<ControllerPairingController::Observer> observers_;
  Stage current_stage_;
  std::string confirmation_code_;
  std::string preset_confirmation_code_;
  base::TimeDelta async_duration_;
  DiscoveryScenario discovery_scenario_;
  std::set<std::string> discovered_devices_;
  std::string choosen_device_;
  bool should_fail_on_connecting_;
  Stage connection_lost_begin_;
  Stage connection_lost_end_;
  bool enrollment_should_fail_;

  DISALLOW_COPY_AND_ASSIGN(FakeControllerPairingController);
};

}  // namespace pairing_chromeos

#endif  // COMPONENTS_PAIRING_FAKE_CONTROLLER_PAIRING_CONTROLLER_H_
