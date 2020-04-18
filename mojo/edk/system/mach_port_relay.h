// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_MACH_PORT_RELAY_H_
#define MOJO_EDK_SYSTEM_MACH_PORT_RELAY_H_

#include <set>
#include <vector>

#include "base/macros.h"
#include "base/process/port_provider_mac.h"
#include "base/synchronization/lock.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/channel.h"

namespace mojo {
namespace edk {

// The MachPortRelay is used by a privileged process, usually the root process,
// to manipulate Mach ports in a child process. Ports can be added to and
// extracted from a child process that has registered itself with the
// |base::PortProvider| used by this class.
class MachPortRelay : public base::PortProvider::Observer {
 public:
  class Observer {
   public:
    // Called by the MachPortRelay to notify observers that a new process is
    // ready for Mach ports to be sent/received. There are no guarantees about
    // the thread this is called on, including the presence of a MessageLoop.
    // Implementations must not call AddObserver() or RemoveObserver() during
    // this function, as doing so will deadlock.
    virtual void OnProcessReady(base::ProcessHandle process) = 0;
  };

  // Used by a child process to receive Mach ports from a sender (privileged)
  // process. Each Mach port in |handles| is interpreted as an intermediate Mach
  // port. It replaces each Mach port with the final Mach port received from the
  // intermediate port. This method takes ownership of the intermediate Mach
  // port and gives ownership of the final Mach port to the caller. Any handles
  // that are not Mach ports will remain unchanged, and the number and ordering
  // of handles is preserved.
  // On failure, the Mach port is replaced with MACH_PORT_NULL.
  //
  // See SendPortsToProcess() for the definition of intermediate and final Mach
  // ports.
  static void ReceivePorts(std::vector<ScopedInternalPlatformHandle>* handles);

  explicit MachPortRelay(base::PortProvider* port_provider);
  ~MachPortRelay() override;

  // Sends the Mach ports attached to |message| to |process|.
  // For each Mach port attached to |message|, a new Mach port, the intermediate
  // port, is created in |process|. The message's Mach port is then sent over
  // this intermediate port and the message is modified to refer to the name of
  // the intermediate port. The Mach port received over the intermediate port in
  // the child is referred to as the final Mach port.
  // Ports that cannot be brokered are replaced with MACH_PORT_NULL.
  void SendPortsToProcess(Channel::Message* message,
                          base::ProcessHandle process);

  // Given a InternalPlatformHandle of Type::MACH_NAME, extracts the Mach port,
  // and updates the contents of the InternalPlatformHandle to have Type::MACH
  // and have the actual Mach port. On failure, replaces the contents with
  // Type::MACH and MACH_PORT_NULL.
  void ExtractPort(ScopedInternalPlatformHandle* handle,
                   base::ProcessHandle process);

  // Observer interface.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  base::PortProvider* port_provider() const { return port_provider_; }

 private:
  // base::PortProvider::Observer implementation.
  void OnReceivedTaskPort(base::ProcessHandle process) override;

  base::PortProvider* const port_provider_;

  base::Lock observers_lock_;
  std::set<Observer*> observers_;

  DISALLOW_COPY_AND_ASSIGN(MachPortRelay);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_MACH_PORT_RELAY_H_
