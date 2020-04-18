// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_CONNECT_TO_DEVICE_OPERATION_H_
#define CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_CONNECT_TO_DEVICE_OPERATION_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/services/secure_channel/connect_to_device_operation.h"

namespace chromeos {

namespace secure_channel {

// Fake ConnectToDeviceOperation implementation, whose FailureDetailType is
// std::string.
class FakeConnectToDeviceOperation
    : public ConnectToDeviceOperation<std::string> {
 public:
  FakeConnectToDeviceOperation(
      ConnectToDeviceOperation<std::string>::ConnectionSuccessCallback
          success_callback,
      ConnectToDeviceOperation<std::string>::ConnectionFailedCallback
          failure_callback);
  ~FakeConnectToDeviceOperation() override;

  bool canceled() const { return canceled_; }

  void set_destructor_callback(base::OnceClosure destructor_callback) {
    destructor_callback_ = std::move(destructor_callback);
  }

  // ConnectToDeviceOperation<std::string>:
  void PerformCancellation() override;

  // Make On{Successful|Failed}ConnectionAttempt() public for testing.
  using ConnectToDeviceOperation<std::string>::OnSuccessfulConnectionAttempt;
  using ConnectToDeviceOperation<std::string>::OnFailedConnectionAttempt;

 private:
  bool canceled_ = false;
  base::OnceClosure destructor_callback_;

  DISALLOW_COPY_AND_ASSIGN(FakeConnectToDeviceOperation);
};

}  // namespace secure_channel

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_CONNECT_TO_DEVICE_OPERATION_H_
