// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/quic/quic_client.h"

#include <algorithm>
#include <memory>

#include "platform/api/logging.h"

namespace openscreen {

QuicClient::QuicClient(
    MessageDemuxer* demuxer,
    std::unique_ptr<QuicConnectionFactory> connection_factory,
    ProtocolConnectionServiceObserver* observer)
    : ProtocolConnectionClient(demuxer, observer),
      connection_factory_(std::move(connection_factory)) {}

QuicClient::~QuicClient() {
  CloseAllConnections();
}

bool QuicClient::Start() {
  if (state_ == State::kRunning)
    return false;
  state_ = State::kRunning;
  observer_->OnRunning();
  return true;
}

bool QuicClient::Stop() {
  if (state_ == State::kStopped)
    return false;
  CloseAllConnections();
  state_ = State::kStopped;
  observer_->OnStopped();
  return true;
}

void QuicClient::RunTasks() {
  connection_factory_->RunTasks();
  for (auto& entry : connections_) {
    entry.second.delegate->DestroyClosedStreams();
    if (!entry.second.delegate->has_streams())
      entry.second.connection->Close();
  }

  for (auto& entry : delete_connections_)
    connections_.erase(entry);

  delete_connections_.clear();
}

QuicClient::ConnectRequest QuicClient::Connect(
    const IPEndpoint& endpoint,
    ConnectionRequestCallback* request) {
  if (state_ != State::kRunning)
    return ConnectRequest(this, 0);
  auto endpoint_entry = endpoint_map_.find(endpoint);
  if (endpoint_entry != endpoint_map_.end()) {
    auto immediate_result = CreateProtocolConnection(endpoint_entry->second);
    OSP_DCHECK(immediate_result);
    request->OnConnectionOpened(0, std::move(immediate_result));
    return ConnectRequest(this, 0);
  }

  return CreatePendingConnection(endpoint, request);
}

std::unique_ptr<ProtocolConnection> QuicClient::CreateProtocolConnection(
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

void QuicClient::OnConnectionDestroyed(QuicProtocolConnection* connection) {
  if (!connection->stream())
    return;

  auto connection_entry = connections_.find(connection->endpoint_id());
  if (connection_entry == connections_.end())
    return;

  connection_entry->second.delegate->DropProtocolConnection(connection);
}

uint64_t QuicClient::OnCryptoHandshakeComplete(
    ServiceConnectionDelegate* delegate,
    uint64_t connection_id) {
  const IPEndpoint& endpoint = delegate->endpoint();
  auto pending_entry = pending_connections_.find(endpoint);
  if (pending_entry == pending_connections_.end())
    return 0;

  ServiceConnectionData connection_data = std::move(pending_entry->second.data);
  auto* connection = connection_data.connection.get();
  uint64_t endpoint_id = next_endpoint_id_++;
  endpoint_map_[endpoint] = endpoint_id;
  connections_.emplace(endpoint_id, std::move(connection_data));

  for (auto& request : pending_entry->second.callbacks) {
    request_map_.erase(request.first);
    std::unique_ptr<QuicProtocolConnection> pc =
        QuicProtocolConnection::FromExisting(this, connection, delegate,
                                             endpoint_id);
    request_map_.erase(request.first);
    request.second->OnConnectionOpened(request.first, std::move(pc));
  }
  pending_connections_.erase(pending_entry);
  return endpoint_id;
}

void QuicClient::OnIncomingStream(
    std::unique_ptr<QuicProtocolConnection> connection) {
  // TODO(jophba): Change to just use OnIncomingConnection when the observer
  // is properly set up.
  connection->CloseWriteEnd();
  connection.reset();
}

void QuicClient::OnConnectionClosed(uint64_t endpoint_id,
                                    uint64_t connection_id) {
  // TODO(btolsch): Is this how handshake failure is communicated to the
  // delegate?
  auto connection_entry = connections_.find(endpoint_id);
  if (connection_entry == connections_.end())
    return;

  delete_connections_.emplace_back(connection_entry);

  // TODO(issue/42): If we reset request IDs when a connection is closed, we
  // might end up re-using request IDs when a new connection is created to the
  // same endpoint.
  endpoint_request_ids_.ResetRequestId(endpoint_id);
}

void QuicClient::OnDataReceived(uint64_t endpoint_id,
                                uint64_t connection_id,
                                const uint8_t* data,
                                size_t data_size) {
  demuxer_->OnStreamData(endpoint_id, connection_id, data, data_size);
}

QuicClient::PendingConnectionData::PendingConnectionData(
    ServiceConnectionData&& data)
    : data(std::move(data)) {}
QuicClient::PendingConnectionData::PendingConnectionData(
    PendingConnectionData&&) = default;
QuicClient::PendingConnectionData::~PendingConnectionData() = default;
QuicClient::PendingConnectionData& QuicClient::PendingConnectionData::operator=(
    PendingConnectionData&&) = default;

QuicClient::ConnectRequest QuicClient::CreatePendingConnection(
    const IPEndpoint& endpoint,
    ConnectionRequestCallback* request) {
  auto pending_entry = pending_connections_.find(endpoint);
  if (pending_entry == pending_connections_.end()) {
    uint64_t request_id = StartConnectionRequest(endpoint, request);
    return ConnectRequest(this, request_id);
  } else {
    uint64_t request_id = next_request_id_++;
    pending_entry->second.callbacks.emplace_back(request_id, request);
    return ConnectRequest(this, request_id);
  }
}

uint64_t QuicClient::StartConnectionRequest(
    const IPEndpoint& endpoint,
    ConnectionRequestCallback* request) {
  auto delegate = std::make_unique<ServiceConnectionDelegate>(this, endpoint);
  std::unique_ptr<QuicConnection> connection =
      connection_factory_->Connect(endpoint, delegate.get());
  if (!connection) {
    // TODO(btolsch): Need interface/handling for Connect() failures. Or, should
    // request->OnConnectionFailed() be called?
    OSP_DCHECK(false)
        << __func__
        << ": Factory connect failed, but requestor will never know.";
    return 0;
  }
  auto pending_result = pending_connections_.emplace(
      endpoint, PendingConnectionData(ServiceConnectionData(
                    std::move(connection), std::move(delegate))));
  uint64_t request_id = next_request_id_++;
  pending_result.first->second.callbacks.emplace_back(request_id, request);
  return request_id;
}

void QuicClient::CloseAllConnections() {
  for (auto& conn : pending_connections_)
    conn.second.data.connection->Close();

  pending_connections_.clear();
  for (auto& conn : connections_)
    conn.second.connection->Close();

  connections_.clear();
  endpoint_map_.clear();
  next_endpoint_id_ = 0;
  endpoint_request_ids_.Reset();
  for (auto& request : request_map_) {
    request.second.second->OnConnectionFailed(request.first);
  }
  request_map_.clear();
}

void QuicClient::CancelConnectRequest(uint64_t request_id) {
  auto request_entry = request_map_.find(request_id);
  if (request_entry == request_map_.end())
    return;

  auto pending_entry = pending_connections_.find(request_entry->second.first);
  if (pending_entry != pending_connections_.end()) {
    auto& callbacks = pending_entry->second.callbacks;
    callbacks.erase(
        std::remove_if(
            callbacks.begin(), callbacks.end(),
            [request_id](const std::pair<uint64_t, ConnectionRequestCallback*>&
                             callback) {
              return request_id == callback.first;
            }),
        callbacks.end());
    if (callbacks.empty())
      pending_connections_.erase(pending_entry);
  }
  request_map_.erase(request_entry);
}

}  // namespace openscreen
