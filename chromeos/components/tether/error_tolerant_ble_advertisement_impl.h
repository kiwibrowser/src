// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_ERROR_TOLERANT_BLE_ADVERTISEMENT_IMPL_H_
#define CHROMEOS_COMPONENTS_ERROR_TOLERANT_BLE_ADVERTISEMENT_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/components/tether/error_tolerant_ble_advertisement.h"
#include "components/cryptauth/foreground_eid_generator.h"
#include "device/bluetooth/bluetooth_advertisement.h"

namespace chromeos {

namespace tether {

class BleSynchronizerBase;

// Concrete ErrorTolerantBleAdvertisement implementation.
class ErrorTolerantBleAdvertisementImpl
    : public ErrorTolerantBleAdvertisement,
      public device::BluetoothAdvertisement::Observer {
 public:
  class Factory {
   public:
    static std::unique_ptr<ErrorTolerantBleAdvertisement> NewInstance(
        const std::string& device_id,
        std::unique_ptr<cryptauth::DataWithTimestamp> advertisement_data,
        BleSynchronizerBase* ble_synchronizer);

    static void SetInstanceForTesting(Factory* factory);

   protected:
    virtual std::unique_ptr<ErrorTolerantBleAdvertisement> BuildInstance(
        const std::string& device_id,
        std::unique_ptr<cryptauth::DataWithTimestamp> advertisement_data,
        BleSynchronizerBase* ble_synchronizer);

    virtual ~Factory();

   private:
    static Factory* factory_instance_;
  };

  ~ErrorTolerantBleAdvertisementImpl() override;

  // ErrorTolerantBleAdvertisement:
  void Stop(const base::Closure& callback) override;
  bool HasBeenStopped() override;

 protected:
  ErrorTolerantBleAdvertisementImpl(
      const std::string& device_id,
      std::unique_ptr<cryptauth::DataWithTimestamp> advertisement_data,
      BleSynchronizerBase* ble_synchronizer);

  // device::BluetoothAdvertisement::Observer
  void AdvertisementReleased(
      device::BluetoothAdvertisement* advertisement) override;

 private:
  friend class ErrorTolerantBleAdvertisementImplTest;

  const cryptauth::DataWithTimestamp& advertisement_data() const {
    return *advertisement_data_;
  }

  void UpdateRegistrationStatus();
  void AttemptRegistration();
  void AttemptUnregistration();

  std::unique_ptr<device::BluetoothAdvertisement::UUIDList> CreateServiceUuids()
      const;

  std::unique_ptr<device::BluetoothAdvertisement::ServiceData>
  CreateServiceData() const;

  void OnAdvertisementRegistered(
      scoped_refptr<device::BluetoothAdvertisement> advertisement);
  void OnErrorRegisteringAdvertisement(
      device::BluetoothAdvertisement::ErrorCode error_code);
  void OnAdvertisementUnregistered();
  void OnErrorUnregisteringAdvertisement(
      device::BluetoothAdvertisement::ErrorCode error_code);

  std::string device_id_;
  std::unique_ptr<cryptauth::DataWithTimestamp> advertisement_data_;
  BleSynchronizerBase* ble_synchronizer_;

  bool registration_in_progress_ = false;
  bool unregistration_in_progress_ = false;

  scoped_refptr<device::BluetoothAdvertisement> advertisement_;

  base::Closure stop_callback_;

  base::WeakPtrFactory<ErrorTolerantBleAdvertisementImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ErrorTolerantBleAdvertisementImpl);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_ERROR_TOLERANT_BLE_ADVERTISEMENT_IMPL_H_
