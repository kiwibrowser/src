// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_SANDBOX_SANDBOX_DELEGATE_H_
#define SERVICES_SERVICE_MANAGER_SANDBOX_SANDBOX_DELEGATE_H_

#include <string>

#include "base/process/process.h"
#include "build/build_config.h"
#include "services/service_manager/sandbox/sandbox_type.h"

namespace sandbox {
class TargetPolicy;
}

namespace service_manager {

class SandboxDelegate {
 public:
  virtual ~SandboxDelegate() {}

  // Returns the SandboxType to enforce on the process, or
  // SANDBOX_TYPE_NO_SANDBOX to run without a sandbox policy.
  virtual service_manager::SandboxType GetSandboxType() = 0;

#if defined(OS_WIN)
  // Whether to disable the default policy specified in
  // AddPolicyForSandboxedProcess.
  virtual bool DisableDefaultPolicy() = 0;

  // Get the AppContainer ID for the sandbox. If this returns false then the
  // AppContainer will not be enabled for the process.
  virtual bool GetAppContainerId(std::string* appcontainer_id) = 0;

  // Called right before spawning the process. Returns false on failure.
  virtual bool PreSpawnTarget(sandbox::TargetPolicy* policy) = 0;

  // Called right after the process is launched, but before its thread is run.
  virtual void PostSpawnTarget(base::ProcessHandle process) = 0;
#endif  // defined(OS_WIN)
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_SANDBOX_SANDBOX_DELEGATE_H_
