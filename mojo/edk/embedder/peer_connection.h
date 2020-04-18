// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_PEER_CONNECTION_H_
#define MOJO_EDK_EMBEDDER_PEER_CONNECTION_H_

#include "base/macros.h"
#include "mojo/edk/embedder/connection_params.h"
#include "mojo/edk/system/system_impl_export.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {
namespace edk {

// Used to connect to a peer process.
//
// NOTE: This should ONLY be used if there is no common ancestor for the
// processes being connected. Peer connections have limited capabilities with
// respect to Mojo IPC when compared to standard broker client connections (see
// OutgoingBrokerClientInvitation and IncomingBrokerClientInvitation), and in
// particular it's undefined behavior to attempt to forward any resources
// (message pipes or other system handles) received from a peer process over to
// any other process to which you're connected.
//
// Both processes must construct a PeerConnection with each one corresponding to
// one end of some shared connection medium (e.g. a platform channel.)
//
// Each PeerConnection gets an implicit cross-process message pipe, the local
// endpoint of which may be acquired by a one-time call to TakeMessagePipe().
//
// Once established, the connection to the remote peer will remain valid as long
// as each process keeps its respective PeerConnection object alive.
class MOJO_SYSTEM_IMPL_EXPORT PeerConnection {
 public:
  // Constructs a disconnected connection.
  PeerConnection();
  ~PeerConnection();

  // Connects to the peer and returns a primordial message pipe handle which
  // will be connected to a corresponding peer pipe in the remote process.
  ScopedMessagePipeHandle Connect(ConnectionParams params);

 private:
  bool is_connected_ = false;
  uint64_t connection_id_ = 0;

  DISALLOW_COPY_AND_ASSIGN(PeerConnection);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_PEER_CONNECTION_H_
