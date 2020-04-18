// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/runner/common/switches.h"

namespace service_manager {
namespace switches {

// Specified on the command line of service processes to indicate which service
// should be run. Useful when the service process binary may act as one of many
// different embedded services.
const char kServiceName[] = "service-name";

// Provides a child process with a token string they can exchange for a message
// pipe whose other end is bound to a service_manager::Service binding in the
// Service Manager.
const char kServicePipeToken[] = "service-pipe-token";

}  // namespace switches
}  // namespace service_manager
