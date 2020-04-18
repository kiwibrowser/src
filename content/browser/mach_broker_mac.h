// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MACH_BROKER_MAC_H_
#define CONTENT_BROWSER_MACH_BROKER_MAC_H_

#include <map>
#include <string>

#include "base/mac/mach_port_broker.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/process/port_provider_mac.h"
#include "base/process/process_handle.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/browser_child_process_observer.h"
#include "content/public/browser/render_process_host_observer.h"

namespace content {

// A global |MachBroker| singleton is used by content embedders to provide
// access to mach task ports for content child processes.
class CONTENT_EXPORT MachBroker : public base::PortProvider,
                                  public BrowserChildProcessObserver,
                                  public RenderProcessHostObserver,
                                  public base::PortProvider::Observer {
 public:
  // For use in child processes. This will send the task port of the current
  // process over Mach IPC to the port registered by name (via this class) in
  // the parent process. Returns true if the message was sent successfully
  // and false if otherwise.
  static bool ChildSendTaskPortToParent();

  // Returns the global MachBroker.
  static MachBroker* GetInstance();

  // The lock that protects this MachBroker object.  Clients MUST acquire and
  // release this lock around calls to EnsureRunning() and PlaceholderForPid().
  base::Lock& GetLock();

  // Performs any necessary setup that cannot happen in the constructor.
  // Callers MUST acquire the lock given by GetLock() before calling this
  // method (and release the lock afterwards).
  void EnsureRunning();

  // Adds a placeholder to the map for the given pid with MACH_PORT_NULL.
  // Callers MUST acquire the lock given by GetLock() before calling this method
  // (and release the lock afterwards).
  void AddPlaceholderForPid(base::ProcessHandle pid, int child_process_id);

  // Implement |base::PortProvider|.
  mach_port_t TaskForPid(base::ProcessHandle process) const override;

  // Implement |BrowserChildProcessObserver|.
  void BrowserChildProcessHostDisconnected(
      const ChildProcessData& data) override;
  void BrowserChildProcessCrashed(
      const ChildProcessData& data,
      const ChildProcessTerminationInfo& info) override;

  // Implement |RenderProcessHostObserver|.
  void RenderProcessExited(RenderProcessHost* host,
                           const ChildProcessTerminationInfo& info) override;
  void RenderProcessHostDestroyed(RenderProcessHost* host) override;

  // Returns the Mach port name to use when sending or receiving messages.
  // Does the Right Thing in the browser and in child processes.
  static std::string GetMachPortName();

 private:
  friend class MachBrokerTest;
  friend struct base::DefaultSingletonTraits<MachBroker>;

  MachBroker();
  ~MachBroker() override;

  // Implement |base::PortProvider::Observer|.
  void OnReceivedTaskPort(base::ProcessHandle process) override;

  // Removes all mappings belonging to |child_process_id| from the broker.
  void InvalidateChildProcessId(int child_process_id);

  // Callback used to register notifications on the UI thread.
  void RegisterNotifications();

  // Whether or not the class has been initialized.
  bool initialized_;

  // Stores the Child process unique id (RenderProcessHost ID) for every
  // process. Protected by base::MachPortBroker::GetLock().
  typedef std::map<int, base::ProcessHandle> ChildProcessIdMap;
  ChildProcessIdMap child_process_id_map_;

  // Underlying port broker that receives and manages mach ports.
  base::MachPortBroker broker_;

  DISALLOW_COPY_AND_ASSIGN(MachBroker);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MACH_BROKER_MAC_H_
