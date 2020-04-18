// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/fake_connection_attempt_delegate.h"

#include "base/logging.h"
#include "chromeos/services/secure_channel/authenticated_channel.h"

namespace chromeos {

namespace secure_channel {

FakeConnectionAttemptDelegate::FakeConnectionAttemptDelegate() = default;

FakeConnectionAttemptDelegate::~FakeConnectionAttemptDelegate() = default;

void FakeConnectionAttemptDelegate::OnConnectionAttemptSucceeded(
    const ConnectionDetails& connection_details,
    std::unique_ptr<AuthenticatedChannel> authenticated_channel) {
  DCHECK(!connection_details_);
  connection_details_ = connection_details;
  authenticated_channel_ = std::move(authenticated_channel);
}

void FakeConnectionAttemptDelegate::
    OnConnectionAttemptFinishedWithoutConnection(
        const ConnectionDetails& connection_details) {
  DCHECK(!connection_details_);
  connection_details_ = connection_details;
}

}  // namespace secure_channel

}  // namespace chromeos
