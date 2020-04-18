// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_discovery.h"

#include <utility>

#include "build/build_config.h"
#include "device/fido/fido_ble_discovery.h"
#include "device/fido/fido_device.h"

// HID is not supported on Android.
#if !defined(OS_ANDROID)
#include "device/fido/fido_hid_discovery.h"
#endif  // !defined(OS_ANDROID)

namespace device {

namespace {

std::unique_ptr<FidoDiscovery> CreateFidoDiscoveryImpl(
    FidoTransportProtocol transport,
    service_manager::Connector* connector) {
  switch (transport) {
    case FidoTransportProtocol::kUsbHumanInterfaceDevice:
#if !defined(OS_ANDROID)
      DCHECK(connector);
      return std::make_unique<FidoHidDiscovery>(connector);
#else
      NOTREACHED() << "USB HID not supported on Android.";
      return nullptr;
#endif  // !defined(OS_ANDROID)
    case FidoTransportProtocol::kBluetoothLowEnergy:
      return std::make_unique<FidoBleDiscovery>();
    // FidoCaBleDiscovery is not constructed using FidoDiscovery factory
    // function and instead constructed in FidoGetAssertionRequestHandler as it
    // requires extensions passed on from the relying party.
    case FidoTransportProtocol::kCloudAssistedBluetoothLowEnergy:
      NOTREACHED()
          << "Cable discovery is not constructed using factory method.";
      return nullptr;
    case FidoTransportProtocol::kNearFieldCommunication:
      // TODO(https://crbug.com/825949): Add NFC support.
      return nullptr;
    case FidoTransportProtocol::kInternal:
      NOTREACHED() << "Internal authenticators should be handled separately.";
      return nullptr;
  }
  NOTREACHED() << "Unhandled transport type";
  return nullptr;
}

}  // namespace

FidoDiscovery::Observer::~Observer() = default;

// static
FidoDiscovery::FactoryFuncPtr FidoDiscovery::g_factory_func_ =
    &CreateFidoDiscoveryImpl;

// static
std::unique_ptr<FidoDiscovery> FidoDiscovery::Create(
    FidoTransportProtocol transport,
    service_manager::Connector* connector) {
  return (*g_factory_func_)(transport, connector);
}

FidoDiscovery::FidoDiscovery(FidoTransportProtocol transport)
    : transport_(transport) {}

FidoDiscovery::~FidoDiscovery() = default;

void FidoDiscovery::Start() {
  DCHECK_EQ(state_, State::kIdle);
  state_ = State::kStarting;
  // TODO(hongjunchoi): Fix so that NotifiyStarted() is never called
  // synchronously after StartInternal().
  // See: https://crbug.com/823686
  StartInternal();
}

void FidoDiscovery::NotifyDiscoveryStarted(bool success) {
  DCHECK_EQ(state_, State::kStarting);
  if (success)
    state_ = State::kRunning;
  if (!observer_)
    return;
  observer_->DiscoveryStarted(this, success);
}

void FidoDiscovery::NotifyDeviceAdded(FidoDevice* device) {
  DCHECK_NE(state_, State::kIdle);
  if (!observer_)
    return;
  observer_->DeviceAdded(this, device);
}

void FidoDiscovery::NotifyDeviceRemoved(FidoDevice* device) {
  DCHECK_NE(state_, State::kIdle);
  if (!observer_)
    return;
  observer_->DeviceRemoved(this, device);
}

std::vector<FidoDevice*> FidoDiscovery::GetDevices() {
  std::vector<FidoDevice*> devices;
  devices.reserve(devices_.size());
  for (const auto& device : devices_)
    devices.push_back(device.second.get());
  return devices;
}

std::vector<const FidoDevice*> FidoDiscovery::GetDevices() const {
  std::vector<const FidoDevice*> devices;
  devices.reserve(devices_.size());
  for (const auto& device : devices_)
    devices.push_back(device.second.get());
  return devices;
}

FidoDevice* FidoDiscovery::GetDevice(base::StringPiece device_id) {
  return const_cast<FidoDevice*>(
      static_cast<const FidoDiscovery*>(this)->GetDevice(device_id));
}

const FidoDevice* FidoDiscovery::GetDevice(base::StringPiece device_id) const {
  auto found = devices_.find(device_id);
  return found != devices_.end() ? found->second.get() : nullptr;
}

bool FidoDiscovery::AddDevice(std::unique_ptr<FidoDevice> device) {
  std::string device_id = device->GetId();
  const auto result = devices_.emplace(std::move(device_id), std::move(device));
  if (result.second)
    NotifyDeviceAdded(result.first->second.get());
  return result.second;
}

bool FidoDiscovery::RemoveDevice(base::StringPiece device_id) {
  auto found = devices_.find(device_id);
  if (found == devices_.end())
    return false;

  auto device = std::move(found->second);
  devices_.erase(found);
  NotifyDeviceRemoved(device.get());
  return true;
}

// ScopedFidoDiscoveryFactory -------------------------------------------------

namespace internal {

ScopedFidoDiscoveryFactory::ScopedFidoDiscoveryFactory() {
  DCHECK(!g_current_factory);
  g_current_factory = this;
  original_factory_func_ =
      std::exchange(FidoDiscovery::g_factory_func_,
                    &ForwardCreateFidoDiscoveryToCurrentFactory);
}

ScopedFidoDiscoveryFactory::~ScopedFidoDiscoveryFactory() {
  g_current_factory = nullptr;
  FidoDiscovery::g_factory_func_ = original_factory_func_;
}

// static
std::unique_ptr<FidoDiscovery>
ScopedFidoDiscoveryFactory::ForwardCreateFidoDiscoveryToCurrentFactory(
    FidoTransportProtocol transport,
    ::service_manager::Connector* connector) {
  DCHECK(g_current_factory);
  return g_current_factory->CreateFidoDiscovery(transport, connector);
}

// static
ScopedFidoDiscoveryFactory* ScopedFidoDiscoveryFactory::g_current_factory =
    nullptr;

}  // namespace internal
}  // namespace device
