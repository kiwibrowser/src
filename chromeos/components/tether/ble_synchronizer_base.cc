// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_synchronizer_base.h"

#include <memory>

namespace chromeos {

namespace tether {

BleSynchronizerBase::RegisterArgs::RegisterArgs(
    std::unique_ptr<device::BluetoothAdvertisement::Data> advertisement_data,
    const device::BluetoothAdapter::CreateAdvertisementCallback& callback,
    const device::BluetoothAdapter::AdvertisementErrorCallback& error_callback)
    : advertisement_data(std::move(advertisement_data)),
      callback(callback),
      error_callback(error_callback) {}

BleSynchronizerBase::RegisterArgs::~RegisterArgs() = default;

BleSynchronizerBase::UnregisterArgs::UnregisterArgs(
    scoped_refptr<device::BluetoothAdvertisement> advertisement,
    const device::BluetoothAdvertisement::SuccessCallback& callback,
    const device::BluetoothAdvertisement::ErrorCallback& error_callback)
    : advertisement(std::move(advertisement)),
      callback(callback),
      error_callback(error_callback) {}

BleSynchronizerBase::UnregisterArgs::~UnregisterArgs() = default;

BleSynchronizerBase::StartDiscoveryArgs::StartDiscoveryArgs(
    const device::BluetoothAdapter::DiscoverySessionCallback& callback,
    const device::BluetoothAdapter::ErrorCallback& error_callback)
    : callback(callback), error_callback(error_callback) {}

BleSynchronizerBase::StartDiscoveryArgs::~StartDiscoveryArgs() = default;

BleSynchronizerBase::StopDiscoveryArgs::StopDiscoveryArgs(
    base::WeakPtr<device::BluetoothDiscoverySession> discovery_session,
    const base::Closure& callback,
    const device::BluetoothDiscoverySession::ErrorCallback& error_callback)
    : discovery_session(discovery_session),
      callback(callback),
      error_callback(error_callback) {}

BleSynchronizerBase::StopDiscoveryArgs::~StopDiscoveryArgs() = default;

BleSynchronizerBase::Command::Command(
    std::unique_ptr<RegisterArgs> register_args)
    : command_type(CommandType::REGISTER_ADVERTISEMENT),
      register_args(std::move(register_args)) {}

BleSynchronizerBase::Command::Command(
    std::unique_ptr<UnregisterArgs> unregister_args)
    : command_type(CommandType::UNREGISTER_ADVERTISEMENT),
      unregister_args(std::move(unregister_args)) {}

BleSynchronizerBase::Command::Command(
    std::unique_ptr<StartDiscoveryArgs> start_discovery_args)
    : command_type(CommandType::START_DISCOVERY),
      start_discovery_args(std::move(start_discovery_args)) {}

BleSynchronizerBase::Command::Command(
    std::unique_ptr<StopDiscoveryArgs> stop_discovery_args)
    : command_type(CommandType::STOP_DISCOVERY),
      stop_discovery_args(std::move(stop_discovery_args)) {}

BleSynchronizerBase::Command::~Command() = default;

BleSynchronizerBase::BleSynchronizerBase() = default;

BleSynchronizerBase::~BleSynchronizerBase() = default;

void BleSynchronizerBase::RegisterAdvertisement(
    std::unique_ptr<device::BluetoothAdvertisement::Data> advertisement_data,
    const device::BluetoothAdapter::CreateAdvertisementCallback& callback,
    const device::BluetoothAdapter::AdvertisementErrorCallback&
        error_callback) {
  command_queue_.emplace_back(
      std::make_unique<Command>(std::make_unique<RegisterArgs>(
          std::move(advertisement_data), callback, error_callback)));
  ProcessQueue();
}

void BleSynchronizerBase::UnregisterAdvertisement(
    scoped_refptr<device::BluetoothAdvertisement> advertisement,
    const device::BluetoothAdvertisement::SuccessCallback& callback,
    const device::BluetoothAdvertisement::ErrorCallback& error_callback) {
  command_queue_.emplace_back(
      std::make_unique<Command>(std::make_unique<UnregisterArgs>(
          std::move(advertisement), callback, error_callback)));
  ProcessQueue();
}

void BleSynchronizerBase::StartDiscoverySession(
    const device::BluetoothAdapter::DiscoverySessionCallback& callback,
    const device::BluetoothAdapter::ErrorCallback& error_callback) {
  command_queue_.emplace_back(std::make_unique<Command>(
      std::make_unique<StartDiscoveryArgs>(callback, error_callback)));
  ProcessQueue();
}

void BleSynchronizerBase::StopDiscoverySession(
    base::WeakPtr<device::BluetoothDiscoverySession> discovery_session,
    const base::Closure& callback,
    const device::BluetoothDiscoverySession::ErrorCallback& error_callback) {
  command_queue_.emplace_back(
      std::make_unique<Command>(std::make_unique<StopDiscoveryArgs>(
          discovery_session, callback, error_callback)));
  ProcessQueue();
}

}  // namespace tether

}  // namespace chromeos
