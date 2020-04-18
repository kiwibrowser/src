// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_REQUEST_HANDLER_BASE_H_
#define DEVICE_FIDO_FIDO_REQUEST_HANDLER_BASE_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/strings/string_piece_forward.h"
#include "device/fido/fido_device_authenticator.h"
#include "device/fido/fido_discovery.h"
#include "device/fido/fido_transport_protocol.h"

namespace service_manager {
class Connector;
};  // namespace service_manager

namespace device {

class FidoAuthenticator;
class FidoDevice;
class FidoTask;

// Base class that handles device discovery/removal. Each FidoRequestHandlerBase
// is owned by FidoRequestManager and its lifetime is equivalent to that of a
// single WebAuthn request. For each authenticator, the per-device work is
// carried out by one FidoTask instance, which is constructed on DeviceAdded(),
// and destroyed either on DeviceRemoved() or CancelOutgoingTaks().
class COMPONENT_EXPORT(DEVICE_FIDO) FidoRequestHandlerBase
    : public FidoDiscovery::Observer {
 public:
  using AuthenticatorMap =
      std::map<std::string, std::unique_ptr<FidoAuthenticator>, std::less<>>;

  FidoRequestHandlerBase(
      service_manager::Connector* connector,
      const base::flat_set<FidoTransportProtocol>& transports);
  ~FidoRequestHandlerBase() override;

  // Triggers cancellation of all per-device FidoTasks, except for the device
  // with |exclude_device_id|, if one is provided. Cancelled tasks are
  // immediately removed from |ongoing_tasks_|.
  //
  // This function is invoked either when:
  //  (a) the entire WebAuthn API request is canceled or,
  //  (b) a successful response or "invalid state error" is received from the
  //  any one of the connected authenticators, in which case all other
  //  per-device tasks are cancelled.
  // https://w3c.github.io/webauthn/#iface-pkcredential
  void CancelOngoingTasks(base::StringPiece exclude_device_id = nullptr);

 protected:
  // Subclasses implement this method to dispatch their request onto the given
  // FidoAuthenticator. The FidoAuthenticator is owned by this
  // FidoRequestHandler and stored in active_authenticators().
  virtual void DispatchRequest(FidoAuthenticator*) = 0;

  void Start();

  // Testing seam to allow unit tests to inject a fake authenticator.
  virtual std::unique_ptr<FidoDeviceAuthenticator>
  CreateAuthenticatorFromDevice(FidoDevice* device);

  AuthenticatorMap& active_authenticators() { return active_authenticators_; }
  std::vector<std::unique_ptr<FidoDiscovery>>& discoveries() {
    return discoveries_;
  }

 private:
  // FidoDiscovery::Observer
  void DiscoveryStarted(FidoDiscovery* discovery, bool success) final;
  void DeviceAdded(FidoDiscovery* discovery, FidoDevice* device) final;
  void DeviceRemoved(FidoDiscovery* discovery, FidoDevice* device) final;

  void AddAuthenticator(std::unique_ptr<FidoAuthenticator> authenticator);

  void MaybeAddPlatformAuthenticator();

  AuthenticatorMap active_authenticators_;
  std::vector<std::unique_ptr<FidoDiscovery>> discoveries_;

  // If set to true at any point before calling Start(), the request handler
  // will try to create a platform authenticator to handle the request
  // (currently only TouchIdAuthenticator on macOS).
  bool use_platform_authenticator_ = false;

  DISALLOW_COPY_AND_ASSIGN(FidoRequestHandlerBase);
};

}  // namespace device

#endif  // DEVICE_FIDO_FIDO_REQUEST_HANDLER_BASE_H_
