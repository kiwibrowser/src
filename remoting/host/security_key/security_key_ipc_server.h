// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_SECURITY_KEY_SECURITY_KEY_IPC_SERVER_H_
#define REMOTING_HOST_SECURITY_KEY_SECURITY_KEY_IPC_SERVER_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/time/time.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "remoting/host/security_key/security_key_auth_handler.h"

namespace remoting {

class ClientSessionDetails;
class SecurityKeyIpcServerFactory;

// Responsible for handing the server end of the IPC channel between the
// network process (server) and the remote_security_key process (client).
class SecurityKeyIpcServer {
 public:
  virtual ~SecurityKeyIpcServer() {}

  // Creates a new SecurityKeyIpcServer instance.
  static std::unique_ptr<SecurityKeyIpcServer> Create(
      int connection_id,
      ClientSessionDetails* client_session_details,
      base::TimeDelta initial_connect_timeout,
      const SecurityKeyAuthHandler::SendMessageCallback& message_callback,
      const base::Closure& connect_callback,
      const base::Closure& done_callback);

  // Used to set a Factory to generate fake/mock SecurityKeyIpcServer
  // instances for testing.
  static void SetFactoryForTest(SecurityKeyIpcServerFactory* factory);

  // Creates and starts listening on an IPC channel with the given name.
  virtual bool CreateChannel(
      const mojo::edk::NamedPlatformHandle& channel_handle,
      base::TimeDelta request_timeout) = 0;

  // Sends a security key response IPC message via the IPC channel.
  virtual bool SendResponse(const std::string& message_data) = 0;
};

// Used to allow for creating Fake/Mock SecurityKeyIpcServer for testing.
class SecurityKeyIpcServerFactory {
 public:
  virtual ~SecurityKeyIpcServerFactory() {}

  virtual std::unique_ptr<SecurityKeyIpcServer> Create(
      int connection_id,
      ClientSessionDetails* client_session_details,
      base::TimeDelta connect_timeout,
      const SecurityKeyAuthHandler::SendMessageCallback& message_callback,
      const base::Closure& connect_callback,
      const base::Closure& done_callback) = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_SECURITY_KEY_SECURITY_KEY_IPC_SERVER_H_
