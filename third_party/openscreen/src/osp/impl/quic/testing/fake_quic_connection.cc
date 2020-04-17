// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/quic/testing/fake_quic_connection.h"

#include <memory>

#include "osp/impl/quic/testing/fake_quic_connection_factory.h"
#include "platform/api/logging.h"

namespace openscreen {

FakeQuicStream::FakeQuicStream(Delegate* delegate, uint64_t id)
    : QuicStream(delegate, id) {}

FakeQuicStream::~FakeQuicStream() = default;

void FakeQuicStream::ReceiveData(const uint8_t* data, size_t size) {
  OSP_DCHECK(!read_end_closed_);
  read_buffer_.insert(read_buffer_.end(), data, data + size);
}

void FakeQuicStream::CloseReadEnd() {
  read_end_closed_ = true;
}

std::vector<uint8_t> FakeQuicStream::TakeReceivedData() {
  return std::move(read_buffer_);
}

std::vector<uint8_t> FakeQuicStream::TakeWrittenData() {
  return std::move(write_buffer_);
}

void FakeQuicStream::Write(const uint8_t* data, size_t size) {
  OSP_DCHECK(!write_end_closed_);
  write_buffer_.insert(write_buffer_.end(), data, data + size);
}

void FakeQuicStream::CloseWriteEnd() {
  write_end_closed_ = true;
}

FakeQuicConnection::FakeQuicConnection(
    FakeQuicConnectionFactoryBridge* parent_factory,
    uint64_t connection_id,
    Delegate* delegate)
    : QuicConnection(delegate),
      parent_factory_(parent_factory),
      connection_id_(connection_id) {}

FakeQuicConnection::~FakeQuicConnection() = default;

std::unique_ptr<FakeQuicStream> FakeQuicConnection::MakeIncomingStream() {
  uint64_t stream_id = next_stream_id_++;
  auto result = std::make_unique<FakeQuicStream>(
      delegate()->NextStreamDelegate(id(), stream_id), stream_id);
  streams_.emplace(result->id(), result.get());
  return result;
}

void FakeQuicConnection::OnDataReceived(const platform::ReceivedData& data) {
  OSP_DCHECK(false) << "data should go directly to fake streams";
}

std::unique_ptr<QuicStream> FakeQuicConnection::MakeOutgoingStream(
    QuicStream::Delegate* delegate) {
  auto result = std::make_unique<FakeQuicStream>(delegate, next_stream_id_++);
  streams_.emplace(result->id(), result.get());
  parent_factory_->OnOutgoingStream(this, result.get());
  return result;
}

void FakeQuicConnection::Close() {
  parent_factory_->OnConnectionClosed(this);
  delegate()->OnConnectionClosed(connection_id_);
  for (auto& stream : streams_) {
    stream.second->delegate()->OnClose(stream.first);
    stream.second->delegate()->OnReceived(stream.second, nullptr, 0);
  }
}

}  // namespace openscreen
