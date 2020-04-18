// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_STANDALONE_SERVICE_STANDALONE_SERVICE_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_STANDALONE_SERVICE_STANDALONE_SERVICE_H_

#include "base/callback.h"
#include "services/service_manager/public/mojom/service.mojom.h"

namespace service_manager {

using StandaloneServiceCallback = base::Callback<void(mojom::ServiceRequest)>;

// Runs a standalone service in the current process. This takes care of setting
// up a boilerplate environment, including initializing //base objects, Mojo
// IPC, running a MessageLoop, and establishing a connection to the Service
// Manager via canonical command-line arguments. Starts the sandbox on Linux.
//
// Once a Service request is obtained, |callback| is invoked with it. This call
// blocks until |callback| returns.
//
// NOTE: A typical service should also link against the main() defined in
// main.cc (next to this header) and thus have no need to call this function
// directly.
void RunStandaloneService(const StandaloneServiceCallback& callback);

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_STANDALONE_SERVICE_STANDALONE_SERVICE_H_
