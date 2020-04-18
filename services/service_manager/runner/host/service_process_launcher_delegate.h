// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_RUNNER_HOST_SERVICE_PROCESS_LAUNCHER_DELEGATE_H_
#define SERVICES_SERVICE_MANAGER_RUNNER_HOST_SERVICE_PROCESS_LAUNCHER_DELEGATE_H_

namespace base {
class CommandLine;
}

namespace service_manager {

class Identity;

class ServiceProcessLauncherDelegate {
 public:
  // Called to adjust the commandline for launching the specified app.
  // WARNING: this is called on a background thread.
  virtual void AdjustCommandLineArgumentsForTarget(
      const Identity& target,
      base::CommandLine* command_line) = 0;

 protected:
  virtual ~ServiceProcessLauncherDelegate() {}
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_RUNNER_HOST_SERVICE_PROCESS_LAUNCHER_DELEGATE_H_
