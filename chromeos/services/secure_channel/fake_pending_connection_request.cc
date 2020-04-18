// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/fake_pending_connection_request.h"

#include "base/logging.h"

namespace chromeos {

namespace secure_channel {

FakePendingConnectionRequest::FakePendingConnectionRequest(
    PendingConnectionRequestDelegate* delegate)
    : PendingConnectionRequest<std::string>(delegate),
      id_(base::UnguessableToken::Create()) {}

FakePendingConnectionRequest::~FakePendingConnectionRequest() = default;

const base::UnguessableToken& FakePendingConnectionRequest::GetRequestId()
    const {
  return id_;
}

void FakePendingConnectionRequest::HandleConnectionFailure(
    std::string failure_detail) {
  handled_failure_details_.push_back(failure_detail);
}

ClientConnectionParameters
FakePendingConnectionRequest::ExtractClientConnectionParameters() {
  DCHECK(client_data_for_extraction_);
  return std::move(*client_data_for_extraction_);
}

}  // namespace secure_channel

}  // namespace chromeos
