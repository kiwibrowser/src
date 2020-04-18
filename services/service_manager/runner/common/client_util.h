// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_RUNNER_COMMON_CLIENT_UTIL_H_
#define SERVICES_SERVICE_MANAGER_RUNNER_COMMON_CLIENT_UTIL_H_

#include "services/service_manager/public/mojom/service.mojom.h"

namespace base {
class CommandLine;
}

namespace mojo {
namespace edk {
class IncomingBrokerClientInvitation;
class OutgoingBrokerClientInvitation;
}
}

namespace service_manager {

// Creates a new Service pipe for connection to a not-yet-launched child process
// and returns one end of it. The other end is passed via a token in
// |command_line|. The launched process may extract the corresponding
// ServiceRequest by calling GetServiceRequestFromCommandLine().
mojom::ServicePtr PassServiceRequestOnCommandLine(
    mojo::edk::OutgoingBrokerClientInvitation* invitation,
    base::CommandLine* command_line);

// Extracts a ServiceRequest from the command line of the current process.
// The parent of this process should have passed a request using
// PassServiceRequestOnCommandLine().
mojom::ServiceRequest GetServiceRequestFromCommandLine(
    mojo::edk::IncomingBrokerClientInvitation* invitation);

// Returns true if the ServiceRequest came via the command line from a service
// manager
// instance in another process.
bool ServiceManagerIsRemote();

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_RUNNER_COMMON_CLIENT_UTIL_H_
