// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/fake_ble_synchronizer.h"

namespace chromeos {

namespace tether {

FakeBleSynchronizer::FakeBleSynchronizer() = default;

FakeBleSynchronizer::~FakeBleSynchronizer() = default;

size_t FakeBleSynchronizer::GetNumCommands() {
  return command_queue().size();
}

device::BluetoothAdvertisement::Data& FakeBleSynchronizer::GetAdvertisementData(
    size_t index) {
  DCHECK(command_queue().size() >= index);
  DCHECK(command_queue()[index]->command_type ==
         CommandType::REGISTER_ADVERTISEMENT);
  return *command_queue()[index]->register_args->advertisement_data;
}

const device::BluetoothAdapter::CreateAdvertisementCallback&
FakeBleSynchronizer::GetRegisterCallback(size_t index) {
  DCHECK(command_queue().size() >= index);
  DCHECK(command_queue()[index]->command_type ==
         CommandType::REGISTER_ADVERTISEMENT);
  return command_queue()[index]->register_args->callback;
}

const device::BluetoothAdapter::AdvertisementErrorCallback&
FakeBleSynchronizer::GetRegisterErrorCallback(size_t index) {
  DCHECK(command_queue().size() >= index);
  DCHECK(command_queue()[index]->command_type ==
         CommandType::REGISTER_ADVERTISEMENT);
  return command_queue()[index]->register_args->error_callback;
}

const device::BluetoothAdvertisement::SuccessCallback&
FakeBleSynchronizer::GetUnregisterCallback(size_t index) {
  DCHECK(command_queue().size() >= index);
  DCHECK(command_queue()[index]->command_type ==
         CommandType::UNREGISTER_ADVERTISEMENT);
  return command_queue()[index]->unregister_args->callback;
}

const device::BluetoothAdvertisement::ErrorCallback&
FakeBleSynchronizer::GetUnregisterErrorCallback(size_t index) {
  DCHECK(command_queue().size() >= index);
  DCHECK(command_queue()[index]->command_type ==
         CommandType::UNREGISTER_ADVERTISEMENT);
  return command_queue()[index]->unregister_args->error_callback;
}

const device::BluetoothAdapter::DiscoverySessionCallback&
FakeBleSynchronizer::GetStartDiscoveryCallback(size_t index) {
  DCHECK(command_queue().size() >= index);
  DCHECK(command_queue()[index]->command_type == CommandType::START_DISCOVERY);
  return command_queue()[index]->start_discovery_args->callback;
}

const device::BluetoothAdapter::ErrorCallback&
FakeBleSynchronizer::GetStartDiscoveryErrorCallback(size_t index) {
  DCHECK(command_queue().size() >= index);
  DCHECK(command_queue()[index]->command_type == CommandType::START_DISCOVERY);
  return command_queue()[index]->start_discovery_args->error_callback;
}

const base::Closure& FakeBleSynchronizer::GetStopDiscoveryCallback(
    size_t index) {
  DCHECK(command_queue().size() >= index);
  DCHECK(command_queue()[index]->command_type == CommandType::STOP_DISCOVERY);
  return command_queue()[index]->stop_discovery_args->callback;
}

const device::BluetoothDiscoverySession::ErrorCallback&
FakeBleSynchronizer::GetStopDiscoveryErrorCallback(size_t index) {
  DCHECK(command_queue().size() >= index);
  DCHECK(command_queue()[index]->command_type == CommandType::STOP_DISCOVERY);
  return command_queue()[index]->stop_discovery_args->error_callback;
}

// Left intentionally blank. The test double does not need to process any queued
// commands.
void FakeBleSynchronizer::ProcessQueue() {}

}  // namespace tether

}  // namespace chromeos
