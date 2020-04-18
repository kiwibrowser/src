// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_STANDALONE_SERVICE_MACH_BROKER_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_STANDALONE_SERVICE_MACH_BROKER_H_

#include "base/mac/mach_port_broker.h"

namespace service_manager {

// A global singleton |MachBroker| is used by the service manager to provide
// access to
// Mach task ports for service manager out-of-process applications.
class MachBroker {
 public:
  // Sends the task port of the current process to the parent over Mach IPC.
  // For use in child processes.
  static void SendTaskPortToParent();

  // Returns the global |MachBroker|. For use in the service manager.
  static MachBroker* GetInstance();

  // Registers |pid| with a MACH_PORT_NULL task port in the port provider. A
  // child's pid must be registered before the broker will accept a task port
  // from that child.
  // Callers MUST acquire the lock given by GetLock() before calling this method
  // (and release the lock afterwards).
  void ExpectPid(base::ProcessHandle pid);

  // Removes |pid| from the port provider.
  // Callers MUST acquire the lock given by GetLock() before calling this method
  // (and release the lock afterwards).
  void RemovePid(base::ProcessHandle pid);

  base::Lock& GetLock() { return broker_.GetLock(); }
  base::PortProvider* port_provider() { return &broker_; }

 private:
  MachBroker();
  ~MachBroker();

  base::MachPortBroker broker_;
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_STANDALONE_SERVICE_MACH_BROKER_H_
