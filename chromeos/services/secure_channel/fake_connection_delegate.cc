// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/fake_connection_delegate.h"

#include "base/bind.h"
#include "base/logging.h"

namespace chromeos {

namespace secure_channel {

FakeConnectionDelegate::FakeConnectionDelegate() : weak_ptr_factory_(this) {}

FakeConnectionDelegate::~FakeConnectionDelegate() = default;

mojom::ConnectionDelegatePtr FakeConnectionDelegate::GenerateInterfacePtr() {
  mojom::ConnectionDelegatePtr interface_ptr;
  bindings_.AddBinding(this, mojo::MakeRequest(&interface_ptr));
  return interface_ptr;
}

void FakeConnectionDelegate::DisconnectGeneratedPtrs() {
  bindings_.CloseAllBindings();
}

void FakeConnectionDelegate::OnConnectionAttemptFailure(
    mojom::ConnectionAttemptFailureReason reason) {
  connection_attempt_failure_reason_ = reason;

  if (closure_for_next_delegate_callback_)
    std::move(closure_for_next_delegate_callback_).Run();
}

void FakeConnectionDelegate::OnConnection(
    mojom::ChannelPtr channel,
    mojom::MessageReceiverRequest message_receiver_request) {
  DCHECK(message_receiver_);
  DCHECK(!message_receiver_binding_);

  channel_ = std::move(channel);
  channel_.set_connection_error_with_reason_handler(
      base::BindOnce(&FakeConnectionDelegate::OnChannelDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  message_receiver_binding_ =
      std::make_unique<mojo::Binding<mojom::MessageReceiver>>(
          message_receiver_.get(), std::move(message_receiver_request));
}

void FakeConnectionDelegate::OnChannelDisconnected(
    uint32_t disconnection_reason,
    const std::string& disconnection_description) {
  disconnection_reason_ = disconnection_reason;
  channel_.reset();
}

}  // namespace secure_channel

}  // namespace chromeos
