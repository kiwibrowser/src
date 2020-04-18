// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_GET_ASSERTION_REQUEST_HANDLER_H_
#define DEVICE_FIDO_GET_ASSERTION_REQUEST_HANDLER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "device/fido/ctap_get_assertion_request.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_request_handler.h"
#include "device/fido/fido_transport_protocol.h"

namespace service_manager {
class Connector;
};  // namespace service_manager

namespace device {

class FidoAuthenticator;
class AuthenticatorGetAssertionResponse;

using SignResponseCallback =
    base::OnceCallback<void(FidoReturnCode,
                            base::Optional<AuthenticatorGetAssertionResponse>)>;

class COMPONENT_EXPORT(DEVICE_FIDO) GetAssertionRequestHandler
    : public FidoRequestHandler<AuthenticatorGetAssertionResponse> {
 public:
  GetAssertionRequestHandler(
      service_manager::Connector* connector,
      const base::flat_set<FidoTransportProtocol>& protocols,
      CtapGetAssertionRequest request_parameter,
      SignResponseCallback completion_callback);
  ~GetAssertionRequestHandler() override;

 private:
  // FidoRequestHandlerBase:
  void DispatchRequest(FidoAuthenticator* authenticator) override;

  CtapGetAssertionRequest request_;
  base::WeakPtrFactory<GetAssertionRequestHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(GetAssertionRequestHandler);
};

}  // namespace device

#endif  // DEVICE_FIDO_GET_ASSERTION_REQUEST_HANDLER_H_
