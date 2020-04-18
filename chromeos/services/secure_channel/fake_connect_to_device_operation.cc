// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/fake_connect_to_device_operation.h"

namespace chromeos {

namespace secure_channel {

FakeConnectToDeviceOperation::FakeConnectToDeviceOperation(
    ConnectToDeviceOperation<std::string>::ConnectionSuccessCallback
        success_callback,
    ConnectToDeviceOperation<std::string>::ConnectionFailedCallback
        failure_callback)
    : ConnectToDeviceOperation<std::string>(std::move(success_callback),
                                            std::move(failure_callback)) {}

FakeConnectToDeviceOperation::~FakeConnectToDeviceOperation() {
  if (destructor_callback_)
    std::move(destructor_callback_).Run();
}

void FakeConnectToDeviceOperation::PerformCancellation() {
  canceled_ = true;
}

}  // namespace secure_channel

}  // namespace chromeos
