// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAIRING_CONTROLLER_PAIRING_CONTROLLER_H_
#define COMPONENTS_PAIRING_CONTROLLER_PAIRING_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/macros.h"

namespace pairing_chromeos {

class ControllerPairingController {
 public:
  enum Stage {
    STAGE_NONE,
    STAGE_INITIALIZATION_ERROR,
    STAGE_DEVICES_DISCOVERY,
    STAGE_DEVICE_NOT_FOUND,
    STAGE_ESTABLISHING_CONNECTION,
    STAGE_ESTABLISHING_CONNECTION_ERROR,
    STAGE_WAITING_FOR_CODE_CONFIRMATION,
    STAGE_PAIRING_DONE,
    STAGE_HOST_NETWORK_ERROR,
    STAGE_HOST_UPDATE_IN_PROGRESS,
    STAGE_HOST_CONNECTION_LOST,
    STAGE_WAITING_FOR_CREDENTIALS,
    STAGE_HOST_ENROLLMENT_IN_PROGRESS,
    STAGE_HOST_ENROLLMENT_ERROR,
    STAGE_HOST_ENROLLMENT_SUCCESS,
    STAGE_FINISHED
  };

  class Observer {
   public:
    Observer();
    virtual ~Observer();

    // Called when pairing has moved on from one stage to another.
    virtual void PairingStageChanged(Stage new_stage) = 0;

    // Called when new device was discovered or existing device was lost.
    // This notification is made only on |STAGE_DEVICES_DISCOVERY| stage.
    virtual void DiscoveredDevicesListChanged() = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Observer);
  };

  typedef std::vector<std::string> DeviceIdList;

  ControllerPairingController();
  virtual ~ControllerPairingController();

  // Returns current stage of pairing process.
  virtual Stage GetCurrentStage() = 0;

  // Starts pairing process. Can be called only on |STAGE_NONE| stage.
  virtual void StartPairing() = 0;

  // Returns list of discovered devices. Can be called only on
  // |STAGE_DEVICES_DISCOVERY| stage.
  virtual DeviceIdList GetDiscoveredDevices() = 0;

  // This method is called to start pairing with the device having |device_id|
  // ID. Can be called only on |STAGE_DEVICES_DISCOVERY| stage.
  virtual void ChooseDeviceForPairing(const std::string& device_id) = 0;

  // Rescan for devices to pair with. Can be called only on
  // stages |STAGE_DEVICE_NOT_FOUND|, |STAGE_ESTABLISHING_CONNECTION_ERROR|,
  // |STAGE_HOST_ENROLLMENT_ERROR|.
  virtual void RepeatDiscovery() = 0;

  // Returns pairing confirmation code.
  // Could be called only on |STATE_WAITING_FOR_CODE_CONFIRMATION| stage.
  virtual std::string GetConfirmationCode() = 0;

  // Called to confirm or deny confirmation code. Can be called only on
  // |STAGE_WAITING_FOR_CODE_CONFIRMATION| stage.
  virtual void SetConfirmationCodeIsCorrect(bool correct) = 0;

  // Set the values that will be sent to the host to set its network.
  virtual void SetHostNetwork(const std::string& onc_spec) = 0;

  // Set the values that will be sent to the host if it needs to be configured.
  virtual void SetHostConfiguration(bool accepted_eula,
                                    const std::string& lang,
                                    const std::string& timezone,
                                    bool send_reports,
                                    const std::string& keyboard_layout) = 0;

  // Called when user successfully authenticated on GAIA page. Can be called
  // only on |STAGE_WAITING_FOR_CREDENTIALS| stage.
  // |auth_token| will be sent to the host to be used for enrollment.
  virtual void OnAuthenticationDone(const std::string& domain,
                                    const std::string& auth_token) = 0;

  // Installs app and starts session.
  // Can be called only on |STAGE_HOST_ENROLLMENT_SUCCESS| stage.
  virtual void StartSession() = 0;

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ControllerPairingController);
};

}  // namespace pairing_chromeos

#endif  // COMPONENTS_PAIRING_CONTROLLER_PAIRING_CONTROLLER_H_
