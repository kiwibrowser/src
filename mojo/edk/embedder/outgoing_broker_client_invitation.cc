// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"

#include "base/logging.h"
#include "mojo/edk/system/core.h"
#include "mojo/edk/system/node_controller.h"
#include "mojo/edk/system/ports/port_ref.h"
#include "mojo/edk/system/request_context.h"

namespace mojo {
namespace edk {

OutgoingBrokerClientInvitation::OutgoingBrokerClientInvitation() = default;

OutgoingBrokerClientInvitation::~OutgoingBrokerClientInvitation() {
  RequestContext request_context;
  for (auto& entry : attached_ports_)
    Core::Get()->GetNodeController()->ClosePort(entry.second);
}

ScopedMessagePipeHandle OutgoingBrokerClientInvitation::AttachMessagePipe(
    const std::string& name) {
  DCHECK(!sent_);
  ports::PortRef port;
  ScopedMessagePipeHandle pipe = ScopedMessagePipeHandle(
      MessagePipeHandle(Core::Get()->CreatePartialMessagePipe(&port)));
  attached_ports_.emplace_back(name, port);
  return pipe;
}

ScopedMessagePipeHandle
OutgoingBrokerClientInvitation::ExtractInProcessMessagePipe(
    const std::string& name) {
  // NOTE: Efficiency is not really important here. This is not used in normal
  // production code and is in practice only called when |attached_ports_| has
  // a single entry.
  for (auto it = attached_ports_.begin(); it != attached_ports_.end(); ++it) {
    if (it->first == name) {
      ScopedMessagePipeHandle pipe = ScopedMessagePipeHandle(
          MessagePipeHandle(Core::Get()->CreatePartialMessagePipe(it->second)));
      attached_ports_.erase(it);
      return pipe;
    }
  }

  NOTREACHED();
  return ScopedMessagePipeHandle();
}

void OutgoingBrokerClientInvitation::Send(
    base::ProcessHandle target_process,
    ConnectionParams params,
    const ProcessErrorCallback& error_callback) {
  DCHECK(!sent_);
  sent_ = true;
  Core::Get()->SendBrokerClientInvitation(target_process, std::move(params),
                                          attached_ports_, error_callback);
  attached_ports_.clear();
}

}  // namespace edk
}  // namespace mojo
