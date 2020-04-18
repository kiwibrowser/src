// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BROWSER_CHILD_PROCESS_HOST_IMPL_H_
#define CONTENT_BROWSER_BROWSER_CHILD_PROCESS_HOST_IMPL_H_

#include <stdint.h>

#include <list>
#include <memory>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/shared_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/process/process.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event_watcher.h"
#include "build/build_config.h"
#include "content/browser/child_process_launcher.h"
#include "content/public/browser/browser_child_process_host.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/common/child_process_host_delegate.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"

#if defined(OS_WIN)
#include "base/win/object_watcher.h"
#endif

namespace base {
class CommandLine;
}

namespace content {

class BrowserChildProcessHostIterator;
class BrowserChildProcessObserver;
class BrowserMessageFilter;
class ChildConnection;

// Plugins/workers and other child processes that live on the IO thread use this
// class. RenderProcessHostImpl is the main exception that doesn't use this
/// class because it lives on the UI thread.
class CONTENT_EXPORT BrowserChildProcessHostImpl
    : public BrowserChildProcessHost,
      public ChildProcessHostDelegate,
#if defined(OS_WIN)
      public base::win::ObjectWatcher::Delegate,
#endif
      public ChildProcessLauncher::Client {
 public:
  BrowserChildProcessHostImpl(content::ProcessType process_type,
                              BrowserChildProcessHostDelegate* delegate,
                              const std::string& service_name);
  ~BrowserChildProcessHostImpl() override;

  // Terminates all child processes and deletes each BrowserChildProcessHost
  // instance.
  static void TerminateAll();

  // Copies kEnableFeatures and kDisableFeatures to the command line. Generates
  // them from the FeatureList override state, to take into account overrides
  // from FieldTrials.
  static void CopyFeatureAndFieldTrialFlags(base::CommandLine* cmd_line);

  // Appends kTraceStartup and kTraceRecordMode flags to the command line, if
  // needed.
  static void CopyTraceStartupFlags(base::CommandLine* cmd_line);

  // BrowserChildProcessHost implementation:
  bool Send(IPC::Message* message) override;
  void Launch(std::unique_ptr<SandboxedProcessLauncherDelegate> delegate,
              std::unique_ptr<base::CommandLine> cmd_line,
              bool terminate_on_shutdown) override;
  const ChildProcessData& GetData() const override;
  ChildProcessHost* GetHost() const override;
  ChildProcessTerminationInfo GetTerminationInfo(bool known_dead) override;
  std::unique_ptr<base::SharedPersistentMemoryAllocator> TakeMetricsAllocator()
      override;
  void SetName(const base::string16& name) override;
  void SetHandle(base::ProcessHandle handle) override;
  service_manager::mojom::ServiceRequest TakeInProcessServiceRequest() override;

  // ChildProcessHostDelegate implementation:
  void OnChannelInitialized(IPC::Channel* channel) override;
  void OnChildDisconnected() override;
  const base::Process& GetProcess() const override;
  void BindInterface(const std::string& interface_name,
                     mojo::ScopedMessagePipeHandle interface_pipe) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnChannelConnected(int32_t peer_pid) override;
  void OnChannelError() override;
  void OnBadMessageReceived(const IPC::Message& message) override;

  // Terminates the process and logs a stack trace after a bad message was
  // received from the child process.
  void TerminateOnBadMessageReceived(const std::string& error);

  // Removes this host from the host list. Calls ChildProcessHost::ForceShutdown
  void ForceShutdown();

  // Adds an IPC message filter.
  void AddFilter(BrowserMessageFilter* filter);

  static void HistogramBadMessageTerminated(ProcessType process_type);

  BrowserChildProcessHostDelegate* delegate() const { return delegate_; }

  ChildConnection* child_connection() const {
    return child_connection_.get();
  }

  mojo::edk::OutgoingBrokerClientInvitation*
  GetInProcessBrokerClientInvitation() {
    return broker_client_invitation_.get();
  }

  IPC::Channel* child_channel() const { return channel_; }

  typedef std::list<BrowserChildProcessHostImpl*> BrowserChildProcessList;
 private:
  friend class BrowserChildProcessHostIterator;
  friend class BrowserChildProcessObserver;

  static BrowserChildProcessList* GetIterator();

  static void AddObserver(BrowserChildProcessObserver* observer);
  static void RemoveObserver(BrowserChildProcessObserver* observer);

  // Creates the |metrics_allocator_|.
  void CreateMetricsAllocator();

  // Passes the |metrics_allocator_|, if any, to the managed process. This
  // requires the process to have been launched and the IPC channel to be
  // available.
  void ShareMetricsAllocatorToProcess();

  // ChildProcessLauncher::Client implementation.
  void OnProcessLaunched() override;
  void OnProcessLaunchFailed(int error_code) override;

  // Returns true if the process has successfully launched. Must only be called
  // on the IO thread.
  bool IsProcessLaunched() const;

  static void OnMojoError(
      base::WeakPtr<BrowserChildProcessHostImpl> process,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      const std::string& error);

#if defined(OS_WIN)
  // ObjectWatcher::Delegate implementation.
  void OnObjectSignaled(HANDLE object) override;
#endif

  ChildProcessData data_;
  BrowserChildProcessHostDelegate* delegate_;
  std::unique_ptr<ChildProcessHost> child_process_host_;

  std::unique_ptr<mojo::edk::OutgoingBrokerClientInvitation>
      broker_client_invitation_;
  std::unique_ptr<ChildConnection> child_connection_;

  std::unique_ptr<ChildProcessLauncher> child_process_;

#if defined(OS_WIN)
  // Watches to see if the child process exits before the IPC channel has
  // been connected. Thereafter, its exit is determined by an error on the
  // IPC channel.
  base::win::ObjectWatcher early_exit_watcher_;
#endif

  // The memory allocator, if any, in which the process will write its metrics.
  std::unique_ptr<base::SharedPersistentMemoryAllocator> metrics_allocator_;

  IPC::Channel* channel_ = nullptr;
  bool is_channel_connected_;
  bool notify_child_disconnected_;

  base::WeakPtrFactory<BrowserChildProcessHostImpl> weak_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_BROWSER_CHILD_PROCESS_HOST_IMPL_H_
