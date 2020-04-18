// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/runner/common/client_util.h"

#include <string>

#include "base/command_line.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "services/service_manager/runner/common/switches.h"

namespace service_manager {

mojom::ServicePtr PassServiceRequestOnCommandLine(
    mojo::edk::OutgoingBrokerClientInvitation* invitation,
    base::CommandLine* command_line) {
  mojom::ServicePtr client;
  std::string token = mojo::edk::GenerateRandomToken();
  client.Bind(mojom::ServicePtrInfo(invitation->AttachMessagePipe(token), 0));
  command_line->AppendSwitchASCII(switches::kServicePipeToken, token);
  return client;
}

mojom::ServiceRequest GetServiceRequestFromCommandLine(
    mojo::edk::IncomingBrokerClientInvitation* invitation) {
  std::string token =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kServicePipeToken);
  mojom::ServiceRequest request;
  if (!token.empty())
    request = mojom::ServiceRequest(invitation->ExtractMessagePipe(token));
  return request;
}

bool ServiceManagerIsRemote() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kServicePipeToken);
}

}  // namespace service_manager
