// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/fake_authenticated_channel.h"

namespace chromeos {

namespace secure_channel {

FakeAuthenticatedChannel::FakeAuthenticatedChannel() : AuthenticatedChannel() {}

FakeAuthenticatedChannel::~FakeAuthenticatedChannel() = default;

const mojom::ConnectionMetadata&
FakeAuthenticatedChannel::GetConnectionMetadata() const {
  return connection_metadata_;
}

void FakeAuthenticatedChannel::PerformSendMessage(
    const std::string& feature,
    const std::string& payload,
    base::OnceClosure on_sent_callback) {
  sent_messages_.push_back(
      std::make_tuple(feature, payload, std::move(on_sent_callback)));
}

void FakeAuthenticatedChannel::PerformDisconnection() {
  has_disconnection_been_requested_ = true;
}

FakeAuthenticatedChannelObserver::FakeAuthenticatedChannelObserver() = default;

FakeAuthenticatedChannelObserver::~FakeAuthenticatedChannelObserver() = default;

void FakeAuthenticatedChannelObserver::OnDisconnected() {
  has_been_notified_of_disconnection_ = true;
}

void FakeAuthenticatedChannelObserver::OnMessageReceived(
    const std::string& feature,
    const std::string& payload) {
  received_messages_.push_back(std::make_pair(feature, payload));
}

}  // namespace secure_channel

}  // namespace chromeos
