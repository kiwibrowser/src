// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_request_handler_base.h"

#include <utility>

#include "base/strings/string_piece.h"
#include "build/build_config.h"
#include "device/fido/fido_device.h"
#include "device/fido/fido_task.h"
#include "services/service_manager/public/cpp/connector.h"

#if defined(OS_MACOSX)
#include "device/fido/mac/authenticator.h"
#endif

namespace device {

FidoRequestHandlerBase::FidoRequestHandlerBase(
    service_manager::Connector* connector,
    const base::flat_set<FidoTransportProtocol>& transports) {
  for (const auto transport : transports) {
    // Construction of CaBleDiscovery is handled by the implementing class as it
    // requires an extension passed on from the relying party.
    if (transport == FidoTransportProtocol::kCloudAssistedBluetoothLowEnergy)
      continue;

    if (transport == FidoTransportProtocol::kInternal) {
      use_platform_authenticator_ = true;
      continue;
    }

    auto discovery = FidoDiscovery::Create(transport, connector);
    if (discovery == nullptr) {
      // This can occur in tests when a ScopedVirtualU2fDevice is in effect and
      // HID transports are not configured.
      continue;
    }
    discovery->set_observer(this);
    discoveries_.push_back(std::move(discovery));
  }
}

FidoRequestHandlerBase::~FidoRequestHandlerBase() = default;

void FidoRequestHandlerBase::CancelOngoingTasks(
    base::StringPiece exclude_device_id) {
  for (auto task_it = active_authenticators_.begin();
       task_it != active_authenticators_.end();) {
    DCHECK(!task_it->first.empty());
    if (task_it->first != exclude_device_id) {
      DCHECK(task_it->second);
      task_it->second->Cancel();
      task_it = active_authenticators_.erase(task_it);
    } else {
      ++task_it;
    }
  }
}

void FidoRequestHandlerBase::Start() {
  for (const auto& discovery : discoveries_) {
    discovery->Start();
  }
  if (use_platform_authenticator_) {
    MaybeAddPlatformAuthenticator();
  }
}

void FidoRequestHandlerBase::MaybeAddPlatformAuthenticator() {
#if defined(OS_MACOSX)
  if (__builtin_available(macOS 10.12.2, *)) {
    auto authenticator = fido::mac::TouchIdAuthenticator::CreateIfAvailable();
    if (!authenticator) {
      return;
    }
    AddAuthenticator(std::move(authenticator));
  }
#endif
}

void FidoRequestHandlerBase::DiscoveryStarted(FidoDiscovery* discovery,
                                              bool success) {}

void FidoRequestHandlerBase::DeviceAdded(FidoDiscovery* discovery,
                                         FidoDevice* device) {
  DCHECK(!base::ContainsKey(active_authenticators(), device->GetId()));
  // All devices are initially assumed to support CTAP protocol and thus
  // AuthenticatorGetInfo command is sent to all connected devices. If device
  // errors out, then it is assumed to support U2F protocol.
  device->set_supported_protocol(ProtocolVersion::kCtap);
  AddAuthenticator(CreateAuthenticatorFromDevice(device));
}

std::unique_ptr<FidoDeviceAuthenticator>
FidoRequestHandlerBase::CreateAuthenticatorFromDevice(FidoDevice* device) {
  return std::make_unique<FidoDeviceAuthenticator>(device);
}

void FidoRequestHandlerBase::DeviceRemoved(FidoDiscovery* discovery,
                                           FidoDevice* device) {
  // Device connection has been lost or device has already been removed.
  // Thus, calling CancelTask() is not necessary. Also, below
  // ongoing_tasks_.erase() will have no effect for the devices that have been
  // already removed due to processing error or due to invocation of
  // CancelOngoingTasks().
  active_authenticators_.erase(device->GetId());
}

void FidoRequestHandlerBase::AddAuthenticator(
    std::unique_ptr<FidoAuthenticator> authenticator) {
  DCHECK(!base::ContainsKey(active_authenticators(), authenticator->GetId()));
  FidoAuthenticator* authenticator_ptr = authenticator.get();
  active_authenticators_.emplace(authenticator->GetId(),
                                 std::move(authenticator));
  DispatchRequest(authenticator_ptr);
}

}  // namespace device
