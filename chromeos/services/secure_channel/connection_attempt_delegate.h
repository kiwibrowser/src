// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECTION_ATTEMPT_DELEGATE_H_
#define CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECTION_ATTEMPT_DELEGATE_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "chromeos/services/secure_channel/connection_details.h"

namespace chromeos {

namespace secure_channel {

class AuthenticatedChannel;

class ConnectionAttemptDelegate {
 public:
  ConnectionAttemptDelegate() = default;
  virtual ~ConnectionAttemptDelegate() = default;

  // Invoked when a ConnectionAttempt has successfully resulted in a connection.
  // |attempt_id| corresponds to the ID returned by
  // ConnectionAttempt::attempt_id().
  virtual void OnConnectionAttemptSucceeded(
      const ConnectionDetails& connection_details,
      std::unique_ptr<AuthenticatedChannel> authenticated_channel) = 0;

  // Invoked when a ConnectionAttempt has finished without achieving a
  // connection to the remote device (due to cancellation by the client or
  // connection failures). |attempt_id| corresponds to the ID returned by
  // ConnectionAttempt::attempt_id().
  virtual void OnConnectionAttemptFinishedWithoutConnection(
      const ConnectionDetails& connection_details) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ConnectionAttemptDelegate);
};

}  // namespace secure_channel

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECTION_ATTEMPT_DELEGATE_H_
