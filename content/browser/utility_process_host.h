// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_UTILITY_PROCESS_HOST_H_
#define CONTENT_BROWSER_UTILITY_PROCESS_HOST_H_

#include <memory>
#include <string>

#include "base/environment.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/process/launch.h"
#include "build/build_config.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_child_process_host_delegate.h"
#include "ipc/ipc_sender.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/identity.h"
#include "services/service_manager/sandbox/sandbox_type.h"

namespace base {
class SequencedTaskRunner;
class Thread;
}  // namespace base

namespace content {
class BrowserChildProcessHostImpl;
class InProcessChildThreadParams;
class UtilityProcessHostClient;
struct ChildProcessData;

typedef base::Thread* (*UtilityMainThreadFactoryFunction)(
    const InProcessChildThreadParams&);

// This class acts as the browser-side host to a utility child process.  A
// utility process is a short-lived process that is created to run a specific
// task.  This class lives solely on the IO thread.
// If you need a single method call in the process, use StartFooBar(p).
// If you need multiple batches of work to be done in the process, use
// StartBatchMode(), then multiple calls to StartFooBar(p), then finish with
// EndBatchMode().
// If you need to bind Mojo interfaces, use Start() to start the child
// process and then call BindInterface().
//
// Note: If your class keeps a ptr to an object of this type, grab a weak ptr to
// avoid a use after free since this object is deleted synchronously but the
// client notification is asynchronous.  See http://crbug.com/108871.
class CONTENT_EXPORT UtilityProcessHost
    : public IPC::Sender,
      public BrowserChildProcessHostDelegate {
 public:
  static void RegisterUtilityMainThreadFactory(
      UtilityMainThreadFactoryFunction create);

  // |client| is optional. If supplied it will be notified of incoming messages
  // from the utility process.
  // |client_task_runner| is required if |client| is supplied and is the task
  // runner upon which |client| will be invoked.
  UtilityProcessHost(
      const scoped_refptr<UtilityProcessHostClient>& client,
      const scoped_refptr<base::SequencedTaskRunner>& client_task_runner);
  ~UtilityProcessHost() override;

  base::WeakPtr<UtilityProcessHost> AsWeakPtr();

  // Makes the process run with a specific sandbox type, or unsandboxed if
  // SANDBOX_TYPE_NO_SANDBOX is specified.
  void SetSandboxType(service_manager::SandboxType sandbox_type);

  service_manager::SandboxType sandbox_type() const { return sandbox_type_; }

  // Returns information about the utility child process.
  const ChildProcessData& GetData();
#if defined(OS_POSIX)
  void SetEnv(const base::EnvironmentMap& env);
#endif

  // Starts the utility process.
  bool Start();

  // Binds an interface exposed by the utility process.
  void BindInterface(const std::string& interface_name,
                     mojo::ScopedMessagePipeHandle interface_pipe);

  // Sets the name of the process to appear in the task manager.
  void SetName(const base::string16& name);

  void set_child_flags(int flags) { child_flags_ = flags; }

  // Used when the utility process is going to host a service. |identity| is
  // the identity of the service being launched.
  void SetServiceIdentity(const service_manager::Identity& identity);

  // Sets a single callback that will be invoked exactly once after process
  // launch. If the process has already launched, the callback will not be
  // called.
  void SetLaunchCallback(base::OnceCallback<void(base::ProcessId)> callback);

 private:
  // Starts the child process if needed, returns true on success.
  bool StartProcess();

  // IPCSender:
  bool Send(IPC::Message* message) override;

  // BrowserChildProcessHostDelegate:
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnProcessLaunched() override;
  void OnProcessLaunchFailed(int error_code) override;
  void OnProcessCrashed(int exit_code) override;

  // Cleans up |this| as a result of a failed Start().
  void NotifyAndDelete(int error_code);

  // Notifies the client that the launch failed and deletes |host|.
  static void NotifyLaunchFailedAndDelete(
      base::WeakPtr<UtilityProcessHost> host,
      int error_code);

  // Pointer to our client interface used for progress notifications.
  scoped_refptr<UtilityProcessHostClient> client_;

  // Task runner used for posting progess notifications to |client_|.
  scoped_refptr<base::SequencedTaskRunner> client_task_runner_;

  // Launch the child process with switches that will setup this sandbox type.
  service_manager::SandboxType sandbox_type_;

  // ChildProcessHost flags to use when starting the child process.
  int child_flags_;

  // Map of environment variables to values.
  base::EnvironmentMap env_;

  // True if StartProcess() has been called.
  bool started_;

  // The process name used to identify the process in task manager.
  base::string16 name_;

  // Child process host implementation.
  std::unique_ptr<BrowserChildProcessHostImpl> process_;

  // Used in single-process mode instead of |process_|.
  std::unique_ptr<base::Thread> in_process_thread_;

  // If this has a value it indicates the process is going to host a mojo
  // service.
  base::Optional<service_manager::Identity> service_identity_;

  // Indicates whether the process has been successfully launched yet.
  bool launched_ = false;

  // A callback to invoke on successful process launch.
  base::OnceCallback<void(base::ProcessId)> launch_callback_;

  // Used to vend weak pointers, and should always be declared last.
  base::WeakPtrFactory<UtilityProcessHost> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(UtilityProcessHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_UTILITY_PROCESS_HOST_H_
