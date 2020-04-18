// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/child_process_launcher_helper.h"

#include "base/command_line.h"
#include "base/process/launch.h"
#include "content/browser/child_process_launcher.h"
#include "content/common/sandbox_policy_fuchsia.h"
#include "content/public/browser/child_process_launcher_utils.h"
#include "content/public/common/sandboxed_process_launcher_delegate.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "services/service_manager/embedder/result_codes.h"

namespace content {
namespace internal {

void ChildProcessLauncherHelper::SetProcessPriorityOnLauncherThread(
    base::Process process,
    const ChildProcessLauncherPriority& priority) {
  DCHECK(CurrentlyOnProcessLauncherTaskRunner());
  // TODO(fuchsia): Implement this. (crbug.com/707031)
  NOTIMPLEMENTED();
}

ChildProcessTerminationInfo ChildProcessLauncherHelper::GetTerminationInfo(
    const ChildProcessLauncherHelper::Process& process,
    bool known_dead) {
  ChildProcessTerminationInfo info;
  info.status =
      base::GetTerminationStatus(process.process.Handle(), &info.exit_code);
  return info;
}

// static
bool ChildProcessLauncherHelper::TerminateProcess(const base::Process& process,
                                                  int exit_code) {
  return process.Terminate(exit_code, false);
}

// static
void ChildProcessLauncherHelper::SetRegisteredFilesForService(
    const std::string& service_name,
    catalog::RequiredFileMap required_files) {
  // TODO(fuchsia): Implement this. (crbug.com/707031)
  NOTIMPLEMENTED();
}

// static
void ChildProcessLauncherHelper::ResetRegisteredFilesForTesting() {
  // TODO(fuchsia): Implement this. (crbug.com/707031)
  NOTIMPLEMENTED();
}

void ChildProcessLauncherHelper::BeforeLaunchOnClientThread() {
  DCHECK_CURRENTLY_ON(client_thread_id_);
}

mojo::edk::ScopedInternalPlatformHandle
ChildProcessLauncherHelper::PrepareMojoPipeHandlesOnClientThread() {
  DCHECK_CURRENTLY_ON(client_thread_id_);

  // By doing nothing here, StartLaunchOnClientThread() will construct a channel
  // pair instead.
  return mojo::edk::ScopedInternalPlatformHandle();
}

std::unique_ptr<FileMappedForLaunch>
ChildProcessLauncherHelper::GetFilesToMap() {
  DCHECK(CurrentlyOnProcessLauncherTaskRunner());
  return std::unique_ptr<FileMappedForLaunch>();
}

bool ChildProcessLauncherHelper::BeforeLaunchOnLauncherThread(
    const PosixFileDescriptorInfo& files_to_register,
    base::LaunchOptions* options) {
  DCHECK(CurrentlyOnProcessLauncherTaskRunner());

  mojo::edk::PlatformChannelPair::PrepareToPassHandleToChildProcess(
      mojo_client_handle(), command_line(), &options->handles_to_transfer);

  UpdateLaunchOptionsForSandbox(delegate_->GetSandboxType(), options);

  return true;
}

ChildProcessLauncherHelper::Process
ChildProcessLauncherHelper::LaunchProcessOnLauncherThread(
    const base::LaunchOptions& options,
    std::unique_ptr<FileMappedForLaunch> files_to_register,
    bool* is_synchronous_launch,
    int* launch_result) {
  DCHECK(CurrentlyOnProcessLauncherTaskRunner());
  DCHECK(mojo_client_handle().is_valid());

  // TODO(750938): Implement sandboxed/isolated subprocess launching.
  Process child_process;
  child_process.process = base::LaunchProcess(*command_line(), options);
  return child_process;
}

void ChildProcessLauncherHelper::AfterLaunchOnLauncherThread(
    const ChildProcessLauncherHelper::Process& process,
    const base::LaunchOptions& options) {
  DCHECK(CurrentlyOnProcessLauncherTaskRunner());

  if (process.process.IsValid()) {
    // |mojo_client_handle_| has already been transferred to the child process
    // by this point. Remove it from the scoped container so that we don't
    // erroneously delete it.
    ignore_result(mojo_client_handle_.release());
  }
}

// static
void ChildProcessLauncherHelper::ForceNormalProcessTerminationSync(
    ChildProcessLauncherHelper::Process process) {
  DCHECK(CurrentlyOnProcessLauncherTaskRunner());
  process.process.Terminate(service_manager::RESULT_CODE_NORMAL_EXIT, true);
}

}  // namespace internal
}  // namespace content
