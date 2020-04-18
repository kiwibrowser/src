// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/channel_impl.h"

#include "base/bind.h"

namespace chromeos {

namespace secure_channel {

namespace {
const char kReasonForDisconnection[] = "Remote device disconnected.";
}  // namespace

ChannelImpl::ChannelImpl(Delegate* delegate)
    : delegate_(delegate), binding_(this) {}

ChannelImpl::~ChannelImpl() = default;

mojom::ChannelPtr ChannelImpl::GenerateInterfacePtr() {
  // Only one InterfacePtr should be generated from this instance.
  DCHECK(!binding_);

  mojom::ChannelPtr interface_ptr;
  binding_.Bind(mojo::MakeRequest(&interface_ptr));

  binding_.set_connection_error_handler(base::BindOnce(
      &ChannelImpl::OnBindingDisconnected, base::Unretained(this)));

  return interface_ptr;
}

void ChannelImpl::HandleRemoteDeviceDisconnection() {
  DCHECK(binding_);

  // If the RemoteDevice disconnected, alert clients by providing them a
  // reason specific to this event.
  binding_.CloseWithReason(mojom::Channel::kConnectionDroppedReason,
                           kReasonForDisconnection);
}

void ChannelImpl::SendMessage(const std::string& message,
                              SendMessageCallback callback) {
  delegate_->OnSendMessageRequested(message, std::move(callback));
}

void ChannelImpl::GetConnectionMetadata(
    GetConnectionMetadataCallback callback) {
  std::move(callback).Run(delegate_->GetConnectionMetadata().Clone());
}

void ChannelImpl::OnBindingDisconnected() {
  delegate_->OnClientDisconnected();
}

}  // namespace secure_channel

}  // namespace chromeos
