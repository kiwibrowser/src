// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_MAKE_CREDENTIAL_REQUEST_HANDLER_H_
#define DEVICE_FIDO_MAKE_CREDENTIAL_REQUEST_HANDLER_H_

#include <memory>

#include "base/callback.h"
#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "device/fido/authenticator_selection_criteria.h"
#include "device/fido/ctap_make_credential_request.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_request_handler.h"
#include "device/fido/fido_transport_protocol.h"

namespace service_manager {
class Connector;
};  // namespace service_manager

namespace device {

class FidoAuthenticator;
class AuthenticatorMakeCredentialResponse;

using RegisterResponseCallback = base::OnceCallback<
    void(FidoReturnCode, base::Optional<AuthenticatorMakeCredentialResponse>)>;

class COMPONENT_EXPORT(DEVICE_FIDO) MakeCredentialRequestHandler
    : public FidoRequestHandler<AuthenticatorMakeCredentialResponse> {
 public:
  MakeCredentialRequestHandler(
      service_manager::Connector* connector,
      const base::flat_set<FidoTransportProtocol>& protocols,
      CtapMakeCredentialRequest request_parameter,
      AuthenticatorSelectionCriteria authenticator_criteria,
      RegisterResponseCallback completion_callback);
  ~MakeCredentialRequestHandler() override;

 private:
  // FidoRequestHandlerBase:
  void DispatchRequest(FidoAuthenticator* authenticator) final;

  CtapMakeCredentialRequest request_parameter_;
  AuthenticatorSelectionCriteria authenticator_selection_criteria_;
  base::WeakPtrFactory<MakeCredentialRequestHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MakeCredentialRequestHandler);
};

}  // namespace device

#endif  // DEVICE_FIDO_MAKE_CREDENTIAL_REQUEST_HANDLER_H_
