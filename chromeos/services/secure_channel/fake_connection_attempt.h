// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_CONNECTION_ATTEMPT_H_
#define CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_CONNECTION_ATTEMPT_H_

#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "base/unguessable_token.h"
#include "chromeos/services/secure_channel/client_connection_parameters.h"
#include "chromeos/services/secure_channel/connection_attempt.h"
#include "chromeos/services/secure_channel/pending_connection_request.h"

namespace chromeos {

namespace secure_channel {

class ConnectionAttemptDelegate;

// Fake ConnectionAttempt implementation, whose FailureDetailType is
// std::string.
class FakeConnectionAttempt : public ConnectionAttempt<std::string> {
 public:
  FakeConnectionAttempt(ConnectionAttemptDelegate* delegate,
                        const ConnectionDetails& connection_details);
  ~FakeConnectionAttempt() override;

  using IdToRequestMap =
      std::unordered_map<base::UnguessableToken,
                         std::unique_ptr<PendingConnectionRequest<std::string>>,
                         base::UnguessableTokenHash>;
  const IdToRequestMap& id_to_request_map() const { return id_to_request_map_; }

  void set_client_data_for_extraction(
      std::vector<ClientConnectionParameters> client_data_for_extraction) {
    client_data_for_extraction_ = std::move(client_data_for_extraction);
  }

  // Make OnConnectionAttempt{Succeeded|FinishedWithoutConnection}() public for
  // testing.
  using ConnectionAttempt<std::string>::OnConnectionAttemptSucceeded;
  using ConnectionAttempt<
      std::string>::OnConnectionAttemptFinishedWithoutConnection;

 private:
  // ConnectionAttempt<std::string>:
  void ProcessAddingNewConnectionRequest(
      std::unique_ptr<PendingConnectionRequest<std::string>> request) override;
  std::vector<ClientConnectionParameters> ExtractClientConnectionParameters()
      override;

  IdToRequestMap id_to_request_map_;

  std::vector<ClientConnectionParameters> client_data_for_extraction_;

  DISALLOW_COPY_AND_ASSIGN(FakeConnectionAttempt);
};

}  // namespace secure_channel

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_CONNECTION_ATTEMPT_H_
