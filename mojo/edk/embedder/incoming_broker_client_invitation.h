// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_INCOMING_BROKER_CLIENT_INVITATION_H_
#define MOJO_EDK_EMBEDDER_INCOMING_BROKER_CLIENT_INVITATION_H_

#include <memory>

#include "base/macros.h"
#include "mojo/edk/embedder/connection_params.h"
#include "mojo/edk/embedder/transport_protocol.h"
#include "mojo/edk/system/system_impl_export.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {
namespace edk {

// Mojo embedders may use this to accept a broker client invitation from another
// embedder in the system.
class MOJO_SYSTEM_IMPL_EXPORT IncomingBrokerClientInvitation {
 public:
  ~IncomingBrokerClientInvitation();

  // Accepts an incoming invitation received via the connection medium in
  // |params|.
  static std::unique_ptr<IncomingBrokerClientInvitation> Accept(
      ConnectionParams params);

  // Accepts an incoming invitation from the command line. The command line is
  // expected to have a |PlatformChannelPair::kMojoPlatformChannelHandleSwitch|
  // switch whose value is an integer platform handle identifier (e.g. FD or
  // HANDLE) in the calling process. The handle should correspond to one end of
  // an OS pipe whose other end was used by another process to send an
  // OutgoingBrokerClientInvitation.
  static std::unique_ptr<IncomingBrokerClientInvitation> AcceptFromCommandLine(
      TransportProtocol protocol);

  // Extracts a named message pipe from the accepted invitation. Must be called
  // after Accept() or AcceptFromCommandLine().
  //
  // Note that while this returns a usable pipe handle immediately, extraction
  // and internal pipe connection is an asynchronous process. Therefore this
  // method always returns a valid handle even if no such pipe was attached; in
  // such cases where there was no attached pipe named |name|, the returned pipe
  // handle will imminently signal peer closure.
  ScopedMessagePipeHandle ExtractMessagePipe(const std::string& name);

 private:
  explicit IncomingBrokerClientInvitation(ConnectionParams params);

  DISALLOW_COPY_AND_ASSIGN(IncomingBrokerClientInvitation);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_INCOMING_BROKER_CLIENT_INVITATION_H_
