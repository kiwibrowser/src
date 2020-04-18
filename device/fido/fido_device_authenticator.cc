// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_device_authenticator.h"

#include <utility>

#include "device/fido/authenticator_selection_criteria.h"
#include "device/fido/ctap_get_assertion_request.h"
#include "device/fido/ctap_make_credential_request.h"
#include "device/fido/fido_device.h"
#include "device/fido/get_assertion_task.h"
#include "device/fido/make_credential_task.h"

namespace device {

FidoDeviceAuthenticator::FidoDeviceAuthenticator(FidoDevice* device)
    : device_(device) {}
FidoDeviceAuthenticator::~FidoDeviceAuthenticator() = default;

void FidoDeviceAuthenticator::MakeCredential(
    AuthenticatorSelectionCriteria authenticator_selection_criteria,
    CtapMakeCredentialRequest request,
    MakeCredentialCallback callback) {
  DCHECK(!task_);
  // TODO(martinkr): Change FidoTasks to take all request parameters by const
  // reference, so we can avoid copying these from the RequestHandler.
  task_ = std::make_unique<MakeCredentialTask>(
      device_, std::move(request), std::move(authenticator_selection_criteria),
      std::move(callback));
}

void FidoDeviceAuthenticator::GetAssertion(CtapGetAssertionRequest request,
                                           GetAssertionCallback callback) {
  task_ = std::make_unique<GetAssertionTask>(device_, std::move(request),
                                             std::move(callback));
}

void FidoDeviceAuthenticator::Cancel() {
  if (!task_)
    return;

  task_->CancelTask();
}

std::string FidoDeviceAuthenticator::GetId() const {
  return device_->GetId();
}

void FidoDeviceAuthenticator::SetTaskForTesting(
    std::unique_ptr<FidoTask> task) {
  task_ = std::move(task);
}

}  // namespace device
