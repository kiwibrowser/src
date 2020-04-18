// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECT_TO_DEVICE_OPERATION_FACTORY_H_
#define CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECT_TO_DEVICE_OPERATION_FACTORY_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "chromeos/services/secure_channel/connect_to_device_operation.h"

namespace chromeos {

namespace secure_channel {

// Factory for creating ConnectToDeviceOperation instances.
template <typename FailureDetailType>
class ConnectToDeviceOperationFactory {
 public:
  virtual ~ConnectToDeviceOperationFactory() = default;

  virtual std::unique_ptr<ConnectToDeviceOperation<FailureDetailType>>
  CreateOperation(
      typename ConnectToDeviceOperation<
          FailureDetailType>::ConnectionSuccessCallback success_callback,
      typename ConnectToDeviceOperation<
          FailureDetailType>::ConnectionFailedCallback failure_callback) = 0;

 protected:
  ConnectToDeviceOperationFactory() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(ConnectToDeviceOperationFactory);
};

}  // namespace secure_channel

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECT_TO_DEVICE_OPERATION_FACTORY_H_
