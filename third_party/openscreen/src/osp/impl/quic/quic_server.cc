// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/quic/quic_server.h"

#include <memory>

#include "platform/api/logging.h"

namespace openscreen {

QuicServer::QuicServer(
    const ServerConfig& config,
    MessageDemuxer* demuxer,
    std::unique_ptr<QuicConnectionFactory> connection_factory,
    ProtocolConnectionServer::Observer* observer)
    : ProtocolConnectionServer(demuxer, observer),
      connection_endpoints_(config.connection_endpoints),
      connection_factory_(std::move(connection_factory)) {}

QuicServer::~QuicServer() {
  CloseAllConnections();
}

bool QuicServer::Start() {
  if (state_ != State::kStopped)
    return false;
  state_ = State::kRunning;
  connection_factory_->SetServerDelegate(this, connection_endpoints_);
  observer_->OnRunning();
  return true;
}

bool QuicServer::Stop() {
  if (state_ != State::kRunning && state_ != State::kSuspended)
    return false;
  connection_factory_->SetServerDelegate(nullptr, {});
  CloseAllConnections();
  state_ = State::kStopped;
  observer_->OnStopped();
  return true;
}

bool QuicServer::Suspend() {
  // TODO(btolsch): QuicStreams should either buffer or reject writes.
  if (state_ != State::kRunning)
    return false;
  state_ = State::kSuspended;
  observer_->OnSuspended();
  return true;
}

bool QuicServer::Resume() {
  if (state_ != State::kSuspended)
    return false;
  state_ = State::kRunning;
  observer_->OnRunning();
  return true;
}

void QuicServer::RunTasks() {
  if (state_ == State::kRunning)
    connection_factory_->RunTasks();
  for (auto& entry : connections_)
    entry.second.delegate->DestroyClosedStreams();

  for (auto& entry : delete_connections_)
    connections_.erase(entry);

  delete_connections_.clear();
}

std::unique_ptr<ProtocolConnection> QuicServer::CreateProtocolConnection(
    uint64_t endpoint_id) {
  if (state_ != State::kRunning)
    return nullptr;
  auto connection_entry = connections_.find(endpoint_id);
  if (connection_entry == connections_.end())
    return nullptr;
  return QuicProtocolConnection::FromExisting(
      this, connection_entry->second.connection.get(),
      connection_entry->second.delegate.get(), endpoint_id);
}

void QuicServer::OnConnectionDestroyed(QuicProtocolConnection* connection) {
  if (!connection->stream())
    return;

  auto connection_entry = connections_.find(connection->endpoint_id());
  if (connection_entry == connections_.end())
    return;

  connection_entry->second.delegate->DropProtocolConnection(connection);
}

uint64_t QuicServer::OnCryptoHandshakeComplete(
    ServiceConnectionDelegate* delegate,
    uint64_t connection_id) {
  OSP_DCHECK_EQ(state_, State::kRunning);
  const IPEndpoint& endpoint = delegate->endpoint();
  auto pending_entry = pending_connections_.find(endpoint);
  if (pending_entry == pending_connections_.end())
    return 0;
  ServiceConnectionData connection_data = std::move(pending_entry->second);
  pending_connections_.erase(pending_entry);
  uint64_t endpoint_id = next_endpoint_id_++;
  endpoint_map_[endpoint] = endpoint_id;
  connections_.emplace(endpoint_id, std::move(connection_data));
  return endpoint_id;
}

void QuicServer::OnIncomingStream(
    std::unique_ptr<QuicProtocolConnection> connection) {
  OSP_DCHECK_EQ(state_, State::kRunning);
  observer_->OnIncomingConnection(std::move(connection));
}

void QuicServer::OnConnectionClosed(uint64_t endpoint_id,
                                    uint64_t connection_id) {
  OSP_DCHECK_EQ(state_, State::kRunning);
  auto connection_entry = connections_.find(endpoint_id);
  if (connection_entry == connections_.end())
    return;

  delete_connections_.emplace_back(connection_entry);

  // TODO(issue/42): If we reset request IDs when a connection is closed, we
  // might end up re-using request IDs when a new connection is created to the
  // same endpoint.
  endpoint_request_ids_.ResetRequestId(endpoint_id);
}

void QuicServer::OnDataReceived(uint64_t endpoint_id,
                                uint64_t connection_id,
                                const uint8_t* data,
                                size_t data_size) {
  OSP_DCHECK_EQ(state_, State::kRunning);
  demuxer_->OnStreamData(endpoint_id, connection_id, data, data_size);
}

void QuicServer::CloseAllConnections() {
  for (auto& conn : pending_connections_)
    conn.second.connection->Close();

  pending_connections_.clear();

  for (auto& conn : connections_)
    conn.second.connection->Close();

  connections_.clear();
  endpoint_map_.clear();
  next_endpoint_id_ = 0;
  endpoint_request_ids_.Reset();
}

QuicConnection::Delegate* QuicServer::NextConnectionDelegate(
    const IPEndpoint& source) {
  OSP_DCHECK_EQ(state_, State::kRunning);
  OSP_DCHECK(!pending_connection_delegate_);
  pending_connection_delegate_ =
      std::make_unique<ServiceConnectionDelegate>(this, source);
  return pending_connection_delegate_.get();
}

void QuicServer::OnIncomingConnection(
    std::unique_ptr<QuicConnection> connection) {
  OSP_DCHECK_EQ(state_, State::kRunning);
  const IPEndpoint& endpoint = pending_connection_delegate_->endpoint();
  pending_connections_.emplace(
      endpoint, ServiceConnectionData(std::move(connection),
                                      std::move(pending_connection_delegate_)));
}

}  // namespace openscreen
