// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECTION_ATTEMPT_BASE_H_
#define CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECTION_ATTEMPT_BASE_H_

#include <unordered_map>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/services/secure_channel/connect_to_device_operation.h"
#include "chromeos/services/secure_channel/connect_to_device_operation_factory.h"
#include "chromeos/services/secure_channel/connection_attempt.h"
#include "chromeos/services/secure_channel/connection_details.h"
#include "chromeos/services/secure_channel/pending_connection_request.h"
#include "chromeos/services/secure_channel/pending_connection_request_delegate.h"
#include "chromeos/services/secure_channel/public/mojom/secure_channel.mojom.h"

namespace chromeos {

namespace secure_channel {

class AuthenticatedChannel;

// ConnectionAttempt implementation which stays active for as long as at least
// one of its requests has not yet completed. While a ConnectionAttemptBase is
// active, it starts one or more operations to connect to the device. If an
// operation succeeds in connecting, the ConnectionAttempt notifies its delegate
// of success.
//
// If an operation fails to connect, ConnectionAttemptBase alerts each of its
// PendingConnectionRequests of the failure to connect. Each request can
// decide to give up connecting due to the client canceling the request or
// due to handling too many failures of individual operations. A
// ConnectionAttemptBase alerts its delegate of a failure if all of its
// associated PendingConnectionRequests have given up trying to connect.
//
// When an operation fails but there still exist active requests,
// ConnectionAttempt simply starts up a new operation and retries the
// connection.
template <typename FailureDetailType>
class ConnectionAttemptBase : public ConnectionAttempt<FailureDetailType>,
                              public PendingConnectionRequestDelegate {
 protected:
  ConnectionAttemptBase(
      std::unique_ptr<ConnectToDeviceOperationFactory<FailureDetailType>>
          connect_to_device_operation_factory,
      ConnectionAttemptDelegate* delegate,
      const ConnectionDetails& connection_details,
      scoped_refptr<base::TaskRunner> task_runner =
          base::ThreadTaskRunnerHandle::Get())
      : ConnectionAttempt<FailureDetailType>(delegate, connection_details),
        connect_to_device_operation_factory_(
            std::move(connect_to_device_operation_factory)),
        task_runner_(task_runner),
        weak_ptr_factory_(this) {}

  ~ConnectionAttemptBase() override {
    if (current_operation_)
      current_operation_->Cancel();
  }

 private:
  // ConnectionAttempt<FailureDetailType>:
  void ProcessAddingNewConnectionRequest(
      std::unique_ptr<PendingConnectionRequest<FailureDetailType>> request)
      override {
    if (base::ContainsKey(id_to_request_map_, request->GetRequestId())) {
      PA_LOG(ERROR) << "ConnectionAttemptBase::"
                    << "ProcessAddingNewConnectionRequest(): Processing "
                    << "request whose ID has already been processed.";
      NOTREACHED();
    }

    bool was_empty = id_to_request_map_.empty();
    id_to_request_map_[request->GetRequestId()] = std::move(request);

    // In the case that this ConnectionAttempt was just created and had not yet
    // received a request yet, start up an operation.
    if (was_empty)
      StartNextConnectToDeviceOperation();
  }

  std::vector<ClientConnectionParameters> ExtractClientConnectionParameters()
      override {
    std::vector<ClientConnectionParameters> data_list;
    for (auto& map_entry : id_to_request_map_) {
      data_list.push_back(
          PendingConnectionRequest<FailureDetailType>::
              ExtractClientConnectionParameters(std::move(map_entry.second)));
    }
    return data_list;
  }

  // PendingConnectionRequestDelegate:
  void OnRequestFinishedWithoutConnection(
      const base::UnguessableToken& request_id,
      PendingConnectionRequestDelegate::FailedConnectionReason reason)
      override {
    size_t removed_element_count = id_to_request_map_.erase(request_id);
    if (removed_element_count != 1) {
      DCHECK(removed_element_count == 0);
      PA_LOG(ERROR) << "ConnectionAttemptBase::"
                    << "OnRequestFinishedWithoutConnection(): Request "
                    << "finished, but it was missing from the map.";
    }

    // If there are no longer any active entries, this attempt is finished.
    if (id_to_request_map_.empty())
      this->OnConnectionAttemptFinishedWithoutConnection();
  }

  void StartNextConnectToDeviceOperation() {
    DCHECK(!current_operation_);
    current_operation_ = connect_to_device_operation_factory_->CreateOperation(
        base::BindOnce(
            &ConnectionAttemptBase<
                FailureDetailType>::OnConnectToDeviceOperationSuccess,
            weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(
            &ConnectionAttemptBase<
                FailureDetailType>::OnConnectToDeviceOperationFailure,
            weak_ptr_factory_.GetWeakPtr()));
  }

  void OnConnectToDeviceOperationSuccess(
      std::unique_ptr<AuthenticatedChannel> authenticated_channel) {
    DCHECK(current_operation_);
    current_operation_.reset();
    this->OnConnectionAttemptSucceeded(std::move(authenticated_channel));
  }

  void OnConnectToDeviceOperationFailure(FailureDetailType failure_detail) {
    DCHECK(current_operation_);
    current_operation_.reset();

    // After all requests have been notified of the failure (below), start a
    // retry attempt. If all requests give up without a connection, this
    // instance's delegate will be notified and will, in response, delete
    // |this|. Thus, the retry attempt is posted as a task using a WeakPtr to
    // ensure that a segfault will not occur.
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &ConnectionAttemptBase<
                FailureDetailType>::StartNextConnectToDeviceOperation,
            weak_ptr_factory_.GetWeakPtr()));

    for (auto& map_entry : id_to_request_map_)
      map_entry.second->HandleConnectionFailure(failure_detail);
  }

  std::unique_ptr<ConnectToDeviceOperationFactory<FailureDetailType>>
      connect_to_device_operation_factory_;

  std::unique_ptr<ConnectToDeviceOperation<FailureDetailType>>
      current_operation_;
  std::unordered_map<
      base::UnguessableToken,
      std::unique_ptr<PendingConnectionRequest<FailureDetailType>>,
      base::UnguessableTokenHash>
      id_to_request_map_;
  scoped_refptr<base::TaskRunner> task_runner_;

  base::WeakPtrFactory<ConnectionAttemptBase<FailureDetailType>>
      weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionAttemptBase);
};

}  // namespace secure_channel

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECTION_ATTEMPT_BASE_H_
