// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_PUBLIC_PROTOCOL_CONNECTION_CLIENT_H_
#define OSP_PUBLIC_PROTOCOL_CONNECTION_CLIENT_H_

#include <memory>
#include <ostream>
#include <string>

#include "osp/public/endpoint_request_ids.h"
#include "osp/public/message_demuxer.h"
#include "osp/public/protocol_connection.h"
#include "osp_base/error.h"
#include "osp_base/ip_address.h"
#include "osp_base/macros.h"

namespace openscreen {

// Embedder's view of the network service that initiates OSP connections to OSP
// receivers.
//
// NOTE: This API closely resembles that for the ProtocolConnectionServer; the
// client currently lacks Suspend(). Consider factoring out a common
// ProtocolConnectionEndpoint when the two APIs are finalized.
class ProtocolConnectionClient {
 public:
  enum class State { kStopped = 0, kStarting, kRunning, kStopping };

  class ConnectionRequestCallback {
   public:
    virtual ~ConnectionRequestCallback() = default;

    // Called when a new connection was created between 5-tuples.
    virtual void OnConnectionOpened(
        uint64_t request_id,
        std::unique_ptr<ProtocolConnection> connection) = 0;
    virtual void OnConnectionFailed(uint64_t request_id) = 0;
  };

  class ConnectRequest {
   public:
    ConnectRequest();
    ConnectRequest(ProtocolConnectionClient* parent, uint64_t request_id);
    ConnectRequest(ConnectRequest&& other);
    ~ConnectRequest();
    ConnectRequest& operator=(ConnectRequest&& other);

    explicit operator bool() const { return request_id_; }

    uint64_t request_id() const { return request_id_; }

    // Records that the requested connect operation is complete so it doesn't
    // need to attempt a cancel on destruction.
    void MarkComplete() { request_id_ = 0; }

   private:
    ProtocolConnectionClient* parent_ = nullptr;
    uint64_t request_id_ = 0;
  };

  virtual ~ProtocolConnectionClient();

  // Starts the client using the config object.
  // Returns true if state() == kStopped and the service will be started,
  // false otherwise.
  virtual bool Start() = 0;

  // NOTE: Currently we do not support Suspend()/Resume() for the connection
  // client.  Add those if we can define behavior for the OSP protocol and QUIC
  // for those operations.
  // See: https://github.com/webscreens/openscreenprotocol/issues/108

  // Stops listening and cancels any search in progress.
  // Returns true if state() != (kStopped|kStopping).
  virtual bool Stop() = 0;

  virtual void RunTasks() = 0;

  // Open a new connection to |endpoint|.  This may succeed synchronously if
  // there are already connections open to |endpoint|, otherwise it will be
  // asynchronous.
  virtual ConnectRequest Connect(const IPEndpoint& endpoint,
                                 ConnectionRequestCallback* request) = 0;

  // Synchronously open a new connection to an endpoint identified by
  // |endpoint_id|.  Returns nullptr if it can't be completed synchronously
  // (e.g. there are no existing open connections to that endpoint).
  virtual std::unique_ptr<ProtocolConnection> CreateProtocolConnection(
      uint64_t endpoint_id) = 0;

  MessageDemuxer* message_demuxer() const { return demuxer_; }

  EndpointRequestIds* endpoint_request_ids() { return &endpoint_request_ids_; }

  // Returns the current state of the listener.
  State state() const { return state_; }

  // Returns the last error reported by this client.
  const Error& last_error() const { return last_error_; }

 protected:
  explicit ProtocolConnectionClient(
      MessageDemuxer* demuxer,
      ProtocolConnectionServiceObserver* observer);

  virtual void CancelConnectRequest(uint64_t request_id) = 0;

  State state_ = State::kStopped;
  Error last_error_;
  MessageDemuxer* const demuxer_;
  EndpointRequestIds endpoint_request_ids_;
  ProtocolConnectionServiceObserver* const observer_;

  OSP_DISALLOW_COPY_AND_ASSIGN(ProtocolConnectionClient);
};

std::ostream& operator<<(std::ostream& os,
                         ProtocolConnectionClient::State state);

}  // namespace openscreen

#endif  // OSP_PUBLIC_PROTOCOL_CONNECTION_CLIENT_H_
