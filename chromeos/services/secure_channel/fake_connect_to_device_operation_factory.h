// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_CONNECT_TO_DEVICE_OPERATION_FACTORY_H_
#define CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_CONNECT_TO_DEVICE_OPERATION_FACTORY_H_

#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "chromeos/services/secure_channel/connect_to_device_operation_factory.h"

namespace chromeos {

namespace secure_channel {

class FakeConnectToDeviceOperation;

// Fake ConnectToDeviceOperationFactory implementation, whose FailureDetailType
// is std::string.
class FakeConnectToDeviceOperationFactory
    : public ConnectToDeviceOperationFactory<std::string> {
 public:
  FakeConnectToDeviceOperationFactory();
  ~FakeConnectToDeviceOperationFactory() override;

  FakeConnectToDeviceOperation* last_created_instance() {
    return last_created_instance_;
  }

  size_t num_instances_created() { return num_instances_created_; }

  // ConnectToDeviceOperationFactory<std::string>:
  std::unique_ptr<ConnectToDeviceOperation<std::string>> CreateOperation(
      ConnectToDeviceOperation<std::string>::ConnectionSuccessCallback
          success_callback,
      ConnectToDeviceOperation<std::string>::ConnectionFailedCallback
          failure_callback) override;

 private:
  FakeConnectToDeviceOperation* last_created_instance_ = nullptr;
  size_t num_instances_created_ = 0u;

  DISALLOW_COPY_AND_ASSIGN(FakeConnectToDeviceOperationFactory);
};

}  // namespace secure_channel

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_CONNECT_TO_DEVICE_OPERATION_FACTORY_H_
