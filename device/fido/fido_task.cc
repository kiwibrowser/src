// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_task.h"

#include <utility>

#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "device/fido/ctap_empty_authenticator_request.h"
#include "device/fido/device_response_converter.h"
#include "device/fido/fido_constants.h"

namespace device {

FidoTask::FidoTask(FidoDevice* device) : device_(device), weak_factory_(this) {
  DCHECK(device_);
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&FidoTask::StartTask, weak_factory_.GetWeakPtr()));
}

FidoTask::~FidoTask() = default;

void FidoTask::CancelTask() {
  if (device()->supported_protocol() != ProtocolVersion::kCtap)
    return;

  device()->Cancel();
}

void FidoTask::GetAuthenticatorInfo(base::OnceClosure ctap_callback,
                                    base::OnceClosure u2f_callback) {
  device()->DeviceTransact(
      AuthenticatorGetInfoRequest().Serialize(),
      base::BindOnce(&FidoTask::OnAuthenticatorInfoReceived,
                     weak_factory_.GetWeakPtr(), std::move(ctap_callback),
                     std::move(u2f_callback)));
}

void FidoTask::OnAuthenticatorInfoReceived(
    base::OnceClosure ctap_callback,
    base::OnceClosure u2f_callback,
    base::Optional<std::vector<uint8_t>> response) {
  device()->set_state(FidoDevice::State::kReady);

  base::Optional<AuthenticatorGetInfoResponse> get_info_response;
  if (!response || !(get_info_response = ReadCTAPGetInfoResponse(*response))) {
    device()->set_supported_protocol(ProtocolVersion::kU2f);
    std::move(u2f_callback).Run();
    return;
  }

  device()->SetDeviceInfo(std::move(*get_info_response));
  device()->set_supported_protocol(ProtocolVersion::kCtap);
  std::move(ctap_callback).Run();
}

}  // namespace device
