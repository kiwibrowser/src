// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECT_TO_DEVICE_OPERATION_H_
#define CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECT_TO_DEVICE_OPERATION_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/components/proximity_auth/logging/logging.h"

namespace chromeos {

namespace secure_channel {

class AuthenticatedChannel;

// Performs an operation which creates a connection to a remote device. A
// ConnectToDeviceOperation can only be used for a single connection attempt; if
// clients wish to retry a failed connection attempt, a new
// ConnectToDeviceOperation object should be created.
template <typename FailureDetailType>
class ConnectToDeviceOperation {
 public:
  using ConnectionSuccessCallback =
      base::OnceCallback<void(std::unique_ptr<AuthenticatedChannel>)>;
  using ConnectionFailedCallback = base::OnceCallback<void(FailureDetailType)>;

  virtual ~ConnectToDeviceOperation() {
    if (!is_active_)
      return;

    PA_LOG(ERROR) << "ConnectToDeviceOperation::~ConnectToDeviceOperation(): "
                  << "Operation deleted before it finished or was canceled.";
  }

  // Note: Canceling an ongoing connection attempt will not cause either of the
  // success/failure callbacks passed to the constructor to be invoked.
  void Cancel() {
    if (!is_active_) {
      PA_LOG(ERROR) << "ConnectToDeviceOperation::Cancel(): Tried to cancel "
                    << "operation after it had already finished.";
      return;
    }

    is_active_ = false;
    PerformCancellation();
  }

 protected:
  ConnectToDeviceOperation(ConnectionSuccessCallback success_callback,
                           ConnectionFailedCallback failure_callback)
      : success_callback_(std::move(success_callback)),
        failure_callback_(std::move(failure_callback)) {}

  // Derived types should implement this function to perform the actual
  // cancellation logic.
  virtual void PerformCancellation() = 0;

  void OnSuccessfulConnectionAttempt(
      std::unique_ptr<AuthenticatedChannel> authenticated_channel) {
    if (!is_active_) {
      PA_LOG(ERROR) << "ConnectToDeviceOperation::"
                    << "OnSuccessfulConnectionAttempt(): Tried to "
                    << "complete operation after it had already finished.";
      return;
    }

    is_active_ = false;
    std::move(success_callback_).Run(std::move(authenticated_channel));
  }

  void OnFailedConnectionAttempt(FailureDetailType failure_detail) {
    if (!is_active_) {
      PA_LOG(ERROR) << "ConnectToDeviceOperation::"
                    << "OnFailedConnectionAttempt(): Tried to "
                    << "complete operation after it had already finished.";
      return;
    }

    is_active_ = false;
    std::move(failure_callback_).Run(failure_detail);
  }

 private:
  bool is_active_ = true;

  ConnectionSuccessCallback success_callback_;
  ConnectionFailedCallback failure_callback_;

  DISALLOW_COPY_AND_ASSIGN(ConnectToDeviceOperation);
};

}  // namespace secure_channel

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECT_TO_DEVICE_OPERATION_H_
