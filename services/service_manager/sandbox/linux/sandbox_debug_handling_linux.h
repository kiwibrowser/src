// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_SANDBOX_DEBUG_HANDLING_LINUX_H_
#define SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_SANDBOX_DEBUG_HANDLING_LINUX_H_

#include "base/macros.h"
#include "services/service_manager/sandbox/export.h"

namespace service_manager {

class SERVICE_MANAGER_SANDBOX_EXPORT SandboxDebugHandling {
 public:
  // Depending on the command line, set the current process as
  // non dumpable. Also set any signal handlers for sandbox
  // debugging.
  static bool SetDumpableStatusAndHandlers();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SandboxDebugHandling);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_SANDBOX_DEBUG_HANDLING_LINUX_H_
