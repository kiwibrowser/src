// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/quic/quic_service_common.h"

#include <memory>

#include "platform/api/logging.h"

namespace openscreen {

// static
std::unique_ptr<QuicProtocolConnection> QuicProtocolConnection::FromExisting(
    Owner* owner,
    QuicConnection* connection,
    ServiceConnectionDelegate* delegate,
    uint64_t endpoint_id) {
  OSP_VLOG << "QUIC stream created for endpoint " << endpoint_id;
  std::unique_ptr<QuicStream> stream = connection->MakeOutgoingStream(delegate);
  auto pc = std::make_unique<QuicProtocolConnection>(owner, endpoint_id,
                                                     stream->id());
  pc->set_stream(stream.get());
  delegate->AddStreamPair(ServiceStreamPair(std::move(stream), pc.get()));
  return pc;
}

QuicProtocolConnection::QuicProtocolConnection(Owner* owner,
                                               uint64_t endpoint_id,
                                               uint64_t connection_id)
    : ProtocolConnection(endpoint_id, connection_id), owner_(owner) {}

QuicProtocolConnection::~QuicProtocolConnection() {
  if (stream_) {
    stream_->CloseWriteEnd();
    owner_->OnConnectionDestroyed(this);
    stream_ = nullptr;
  }
}

void QuicProtocolConnection::Write(const uint8_t* data, size_t data_size) {
  if (stream_)
    stream_->Write(data, data_size);
}

void QuicProtocolConnection::CloseWriteEnd() {
  if (stream_)
    stream_->CloseWriteEnd();
}

void QuicProtocolConnection::OnClose() {
  if (observer_)
    observer_->OnConnectionClosed(*this);
}

ServiceStreamPair::ServiceStreamPair(
    std::unique_ptr<QuicStream> stream,
    QuicProtocolConnection* protocol_connection)
    : stream(std::move(stream)),
      connection_id(protocol_connection->id()),
      protocol_connection(std::move(protocol_connection)) {}
ServiceStreamPair::~ServiceStreamPair() = default;

ServiceStreamPair::ServiceStreamPair(ServiceStreamPair&& other) = default;

ServiceStreamPair& ServiceStreamPair::operator=(ServiceStreamPair&& other) =
    default;

ServiceConnectionDelegate::ServiceConnectionDelegate(ServiceDelegate* parent,
                                                     const IPEndpoint& endpoint)
    : parent_(parent), endpoint_(endpoint) {}

ServiceConnectionDelegate::~ServiceConnectionDelegate() {
  void DestroyClosedStreams();
  OSP_DCHECK(streams_.empty());
}

void ServiceConnectionDelegate::AddStreamPair(ServiceStreamPair&& stream_pair) {
  uint64_t stream_id = stream_pair.stream->id();
  streams_.emplace(stream_id, std::move(stream_pair));
}

void ServiceConnectionDelegate::DropProtocolConnection(
    QuicProtocolConnection* connection) {
  auto stream_entry = streams_.find(connection->stream()->id());
  if (stream_entry == streams_.end())
    return;
  stream_entry->second.protocol_connection = nullptr;
}

void ServiceConnectionDelegate::DestroyClosedStreams() {
  closed_streams_.clear();
}

void ServiceConnectionDelegate::OnCryptoHandshakeComplete(
    uint64_t connection_id) {
  endpoint_id_ = parent_->OnCryptoHandshakeComplete(this, connection_id);
  OSP_VLOG << "QUIC connection handshake complete for endpoint "
           << endpoint_id_;
}

void ServiceConnectionDelegate::OnIncomingStream(
    uint64_t connection_id,
    std::unique_ptr<QuicStream> stream) {
  OSP_VLOG << "Incoming QUIC stream from endpoint " << endpoint_id_;
  pending_connection_->set_stream(stream.get());
  AddStreamPair(
      ServiceStreamPair(std::move(stream), pending_connection_.get()));
  parent_->OnIncomingStream(std::move(pending_connection_));
}

void ServiceConnectionDelegate::OnConnectionClosed(uint64_t connection_id) {
  OSP_VLOG << "QUIC connection closed for endpoint " << endpoint_id_;
  parent_->OnConnectionClosed(endpoint_id_, connection_id);
}

QuicStream::Delegate* ServiceConnectionDelegate::NextStreamDelegate(
    uint64_t connection_id,
    uint64_t stream_id) {
  OSP_DCHECK(!pending_connection_);
  pending_connection_ = std::make_unique<QuicProtocolConnection>(
      parent_, endpoint_id_, stream_id);
  return this;
}

void ServiceConnectionDelegate::OnReceived(QuicStream* stream,
                                           const char* data,
                                           size_t data_size) {
  auto stream_entry = streams_.find(stream->id());
  if (stream_entry == streams_.end())
    return;
  ServiceStreamPair& stream_pair = stream_entry->second;
  parent_->OnDataReceived(endpoint_id_, stream_pair.connection_id,
                          reinterpret_cast<const uint8_t*>(data), data_size);
}

void ServiceConnectionDelegate::OnClose(uint64_t stream_id) {
  OSP_VLOG << "QUIC stream closed for endpoint " << endpoint_id_;
  auto stream_entry = streams_.find(stream_id);
  if (stream_entry == streams_.end())
    return;
  ServiceStreamPair& stream_pair = stream_entry->second;
  parent_->OnDataReceived(endpoint_id_, stream_pair.connection_id, nullptr, 0);
  if (stream_pair.protocol_connection) {
    stream_pair.protocol_connection->set_stream(nullptr);
    stream_pair.protocol_connection->OnClose();
  }
  // NOTE: If this OnClose is the result of the read end closing when the write
  // end was already closed, there will likely still be a call to OnReceived.
  // We need to delay actually destroying the stream object until the end of the
  // event loop.
  closed_streams_.push_back(std::move(stream_entry->second));
  streams_.erase(stream_entry);
}

ServiceConnectionData::ServiceConnectionData(
    std::unique_ptr<QuicConnection> connection,
    std::unique_ptr<ServiceConnectionDelegate> delegate)
    : connection(std::move(connection)), delegate(std::move(delegate)) {}
ServiceConnectionData::ServiceConnectionData(ServiceConnectionData&&) = default;
ServiceConnectionData::~ServiceConnectionData() = default;
ServiceConnectionData& ServiceConnectionData::operator=(
    ServiceConnectionData&&) = default;

}  // namespace openscreen
