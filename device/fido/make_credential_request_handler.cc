// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/make_credential_request_handler.h"

#include <utility>

#include "base/bind.h"
#include "device/fido/authenticator_make_credential_response.h"
#include "device/fido/fido_authenticator.h"
#include "device/fido/make_credential_task.h"
#include "services/service_manager/public/cpp/connector.h"

namespace device {

MakeCredentialRequestHandler::MakeCredentialRequestHandler(
    service_manager::Connector* connector,
    const base::flat_set<FidoTransportProtocol>& protocols,
    CtapMakeCredentialRequest request_parameter,
    AuthenticatorSelectionCriteria authenticator_selection_criteria,
    RegisterResponseCallback completion_callback)
    : FidoRequestHandler(connector, protocols, std::move(completion_callback)),
      request_parameter_(std::move(request_parameter)),
      authenticator_selection_criteria_(
          std::move(authenticator_selection_criteria)),
      weak_factory_(this) {
  Start();
}

MakeCredentialRequestHandler::~MakeCredentialRequestHandler() = default;

void MakeCredentialRequestHandler::DispatchRequest(
    FidoAuthenticator* authenticator) {
  return authenticator->MakeCredential(
      authenticator_selection_criteria_, request_parameter_,
      base::BindOnce(&MakeCredentialRequestHandler::OnAuthenticatorResponse,
                     weak_factory_.GetWeakPtr(), authenticator));
}

}  // namespace device
