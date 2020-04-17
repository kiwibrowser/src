// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/protocol_connection_client.h"

namespace openscreen {

ProtocolConnectionClient::ConnectRequest::ConnectRequest() = default;

ProtocolConnectionClient::ConnectRequest::ConnectRequest(
    ProtocolConnectionClient* parent,
    uint64_t request_id)
    : parent_(parent), request_id_(request_id) {}

ProtocolConnectionClient::ConnectRequest::ConnectRequest(ConnectRequest&& other)
    : parent_(other.parent_), request_id_(other.request_id_) {
  other.request_id_ = 0;
}

ProtocolConnectionClient::ConnectRequest::~ConnectRequest() {
  if (request_id_)
    parent_->CancelConnectRequest(request_id_);
}

ProtocolConnectionClient::ConnectRequest&
ProtocolConnectionClient::ConnectRequest::operator=(ConnectRequest&& other) {
  using std::swap;
  swap(parent_, other.parent_);
  swap(request_id_, other.request_id_);
  return *this;
}

ProtocolConnectionClient::ProtocolConnectionClient(
    MessageDemuxer* demuxer,
    ProtocolConnectionServiceObserver* observer)
    : demuxer_(demuxer),
      endpoint_request_ids_(EndpointRequestIds::Role::kClient),
      observer_(observer) {}

ProtocolConnectionClient::~ProtocolConnectionClient() = default;

std::ostream& operator<<(std::ostream& os,
                         ProtocolConnectionClient::State state) {
  switch (state) {
    case ProtocolConnectionClient::State::kStopped:
      return os << "STOPPED";
    case ProtocolConnectionClient::State::kStarting:
      return os << "STARTING";
    case ProtocolConnectionClient::State::kRunning:
      return os << "RUNNING";
    case ProtocolConnectionClient::State::kStopping:
      return os << "STOPPING";
    default:
      return os << "UNKNOWN";
  }
}

}  // namespace openscreen
