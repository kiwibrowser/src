// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/message_port/message_port_channel.h"

namespace blink {

MessagePortChannel::~MessagePortChannel() = default;

MessagePortChannel::MessagePortChannel() : state_(new State()) {}

MessagePortChannel::MessagePortChannel(const MessagePortChannel& other) =
    default;

MessagePortChannel& MessagePortChannel::operator=(
    const MessagePortChannel& other) {
  state_ = other.state_;
  return *this;
}

MessagePortChannel::MessagePortChannel(mojo::ScopedMessagePipeHandle handle)
    : state_(new State(std::move(handle))) {}

const mojo::ScopedMessagePipeHandle& MessagePortChannel::GetHandle() const {
  return state_->handle();
}

mojo::ScopedMessagePipeHandle MessagePortChannel::ReleaseHandle() const {
  return state_->TakeHandle();
}

// static
std::vector<mojo::ScopedMessagePipeHandle> MessagePortChannel::ReleaseHandles(
    const std::vector<MessagePortChannel>& ports) {
  std::vector<mojo::ScopedMessagePipeHandle> handles(ports.size());
  for (size_t i = 0; i < ports.size(); ++i)
    handles[i] = ports[i].ReleaseHandle();
  return handles;
}

// static
std::vector<MessagePortChannel> MessagePortChannel::CreateFromHandles(
    std::vector<mojo::ScopedMessagePipeHandle> handles) {
  std::vector<MessagePortChannel> ports(handles.size());
  for (size_t i = 0; i < handles.size(); ++i)
    ports[i] = MessagePortChannel(std::move(handles[i]));
  return ports;
}

MessagePortChannel::State::State() = default;

MessagePortChannel::State::State(mojo::ScopedMessagePipeHandle handle)
    : handle_(std::move(handle)) {}

mojo::ScopedMessagePipeHandle MessagePortChannel::State::TakeHandle() {
  base::AutoLock lock(lock_);
  return std::move(handle_);
}

MessagePortChannel::State::~State() = default;

}  // namespace blink
