// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/get_assertion_request_handler.h"

#include <utility>

#include "base/bind.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/fido_authenticator.h"
#include "device/fido/fido_cable_discovery.h"
#include "device/fido/get_assertion_task.h"

namespace device {

GetAssertionRequestHandler::GetAssertionRequestHandler(
    service_manager::Connector* connector,
    const base::flat_set<FidoTransportProtocol>& protocols,
    CtapGetAssertionRequest request,
    SignResponseCallback completion_callback)
    : FidoRequestHandler(connector, protocols, std::move(completion_callback)),
      request_(std::move(request)),
      weak_factory_(this) {
  if (base::ContainsKey(
          protocols, FidoTransportProtocol::kCloudAssistedBluetoothLowEnergy) &&
      request_.cable_extension()) {
    auto discovery =
        std::make_unique<FidoCableDiscovery>(*request_.cable_extension());
    discovery->set_observer(this);
    discoveries().push_back(std::move(discovery));
  }

  Start();
}

GetAssertionRequestHandler::~GetAssertionRequestHandler() = default;

void GetAssertionRequestHandler::DispatchRequest(
    FidoAuthenticator* authenticator) {
  authenticator->GetAssertion(
      request_,
      base::BindOnce(&GetAssertionRequestHandler::OnAuthenticatorResponse,
                     weak_factory_.GetWeakPtr(), authenticator));
}

}  // namespace device
