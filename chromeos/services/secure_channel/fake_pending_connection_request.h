// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_PENDING_CONNECTION_REQUEST_H_
#define CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_PENDING_CONNECTION_REQUEST_H_

#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/optional.h"
#include "base/unguessable_token.h"
#include "chromeos/services/secure_channel/client_connection_parameters.h"
#include "chromeos/services/secure_channel/pending_connection_request.h"
#include "chromeos/services/secure_channel/public/mojom/secure_channel.mojom.h"

namespace chromeos {

namespace secure_channel {

class PendingConnectionRequestDelegate;

// Fake PendingConnectionRequest implementation, whose FailureDetailType is
// std::string.
class FakePendingConnectionRequest
    : public PendingConnectionRequest<std::string> {
 public:
  FakePendingConnectionRequest(PendingConnectionRequestDelegate* delegate);
  ~FakePendingConnectionRequest() override;

  const std::vector<std::string>& handled_failure_details() const {
    return handled_failure_details_;
  }

  void set_client_data_for_extraction(
      ClientConnectionParameters client_data_for_extraction) {
    client_data_for_extraction_ = std::move(client_data_for_extraction);
  }

  // PendingConnectionRequest<std::string>:
  const base::UnguessableToken& GetRequestId() const override;

  // Make NotifyRequestFinishedWithoutConnection() public for testing.
  using PendingConnectionRequest<
      std::string>::NotifyRequestFinishedWithoutConnection;

 private:
  // PendingConnectionRequest<std::string>:
  void HandleConnectionFailure(std::string failure_detail) override;
  ClientConnectionParameters ExtractClientConnectionParameters() override;

  const base::UnguessableToken id_;

  std::vector<std::string> handled_failure_details_;

  base::Optional<ClientConnectionParameters> client_data_for_extraction_;

  DISALLOW_COPY_AND_ASSIGN(FakePendingConnectionRequest);
};

}  // namespace secure_channel

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_SECURE_CHANNEL_FAKE_PENDING_CONNECTION_REQUEST_H_
