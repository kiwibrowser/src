// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/fake_connect_to_device_operation_factory.h"

#include "chromeos/services/secure_channel/fake_connect_to_device_operation.h"

namespace chromeos {

namespace secure_channel {

FakeConnectToDeviceOperationFactory::FakeConnectToDeviceOperationFactory() =
    default;

FakeConnectToDeviceOperationFactory::~FakeConnectToDeviceOperationFactory() =
    default;

std::unique_ptr<ConnectToDeviceOperation<std::string>>
FakeConnectToDeviceOperationFactory::CreateOperation(
    ConnectToDeviceOperation<std::string>::ConnectionSuccessCallback
        success_callback,
    ConnectToDeviceOperation<std::string>::ConnectionFailedCallback
        failure_callback) {
  auto instance = std::make_unique<FakeConnectToDeviceOperation>(
      std::move(success_callback), std::move(failure_callback));
  last_created_instance_ = instance.get();
  ++num_instances_created_;
  return instance;
}

}  // namespace secure_channel

}  // namespace chromeos
