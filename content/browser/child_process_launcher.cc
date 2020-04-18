// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/child_process_launcher.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/i18n/icu_util.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "build/build_config.h"
#include "content/public/browser/child_process_launcher_utils.h"
#include "content/public/common/sandboxed_process_launcher_delegate.h"
#include "services/service_manager/embedder/result_codes.h"

namespace content {

using internal::ChildProcessLauncherHelper;

ChildProcessLauncher::ChildProcessLauncher(
    std::unique_ptr<SandboxedProcessLauncherDelegate> delegate,
    std::unique_ptr<base::CommandLine> command_line,
    int child_process_id,
    Client* client,
    std::unique_ptr<mojo::edk::OutgoingBrokerClientInvitation>
        broker_client_invitation,
    const mojo::edk::ProcessErrorCallback& process_error_callback,
    bool terminate_on_shutdown)
    : client_(client),
      starting_(true),
      start_time_(base::TimeTicks::Now()),
#if defined(ADDRESS_SANITIZER) || defined(LEAK_SANITIZER) ||  \
    defined(MEMORY_SANITIZER) || defined(THREAD_SANITIZER) || \
    defined(UNDEFINED_SANITIZER) || defined(CLANG_COVERAGE)
      terminate_child_on_shutdown_(false),
#else
      terminate_child_on_shutdown_(terminate_on_shutdown),
#endif
      weak_factory_(this) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(BrowserThread::GetCurrentThreadIdentifier(&client_thread_id_));

  helper_ = new ChildProcessLauncherHelper(
      child_process_id, client_thread_id_, std::move(command_line),
      std::move(delegate), weak_factory_.GetWeakPtr(), terminate_on_shutdown,
      std::move(broker_client_invitation), process_error_callback);
  helper_->StartLaunchOnClientThread();
}

ChildProcessLauncher::~ChildProcessLauncher() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (process_.process.IsValid() && terminate_child_on_shutdown_) {
    // Client has gone away, so just kill the process.
    ChildProcessLauncherHelper::ForceNormalProcessTerminationAsync(
        std::move(process_));
  }
}

void ChildProcessLauncher::SetProcessPriority(
    const ChildProcessLauncherPriority& priority) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::Process to_pass = process_.process.Duplicate();
  GetProcessLauncherTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ChildProcessLauncherHelper::SetProcessPriorityOnLauncherThread,
          helper_, std::move(to_pass), priority));
}

void ChildProcessLauncher::Notify(
    ChildProcessLauncherHelper::Process process,
    int error_code) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  starting_ = false;
  process_ = std::move(process);

  if (process_.process.IsValid()) {
    client_->OnProcessLaunched();
  } else {
    termination_info_.status = base::TERMINATION_STATUS_LAUNCH_FAILED;

    // NOTE: May delete |this|.
    client_->OnProcessLaunchFailed(error_code);
  }
}

bool ChildProcessLauncher::IsStarting() {
  // TODO(crbug.com/469248): This fails in some tests.
  // DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return starting_;
}

const base::Process& ChildProcessLauncher::GetProcess() const {
  // TODO(crbug.com/469248): This fails in some tests.
  // DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return process_.process;
}

ChildProcessTerminationInfo ChildProcessLauncher::GetChildTerminationInfo(
    bool known_dead) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!process_.process.IsValid()) {
    // Make sure to avoid using the default termination status if the process
    // hasn't even started yet.
    if (IsStarting()) {
      termination_info_.status = base::TERMINATION_STATUS_STILL_RUNNING;
      termination_info_.uptime = base::TimeTicks::Now() - start_time_;
      DCHECK_LE(base::TimeDelta::FromSeconds(0), termination_info_.uptime);
    }

    // Process doesn't exist, so return the cached termination info.
    return termination_info_;
  }

  termination_info_ = helper_->GetTerminationInfo(process_, known_dead);
  termination_info_.uptime = base::TimeTicks::Now() - start_time_;
  DCHECK_LE(base::TimeDelta::FromSeconds(0), termination_info_.uptime);

  // POSIX: If the process crashed, then the kernel closed the socket for it and
  // so the child has already died by the time we get here. Since
  // GetTerminationInfo called waitpid with WNOHANG, it'll reap the process.
  // However, if GetTerminationInfo didn't reap the child (because it was
  // still running), we'll need to Terminate via ProcessWatcher. So we can't
  // close the handle here.
  if (termination_info_.status != base::TERMINATION_STATUS_STILL_RUNNING) {
    process_.process.Exited(termination_info_.exit_code);
    process_.process.Close();
  }

  return termination_info_;
}

bool ChildProcessLauncher::Terminate(int exit_code) {
  return IsStarting() ? false
                      : ChildProcessLauncherHelper::TerminateProcess(
                            GetProcess(), exit_code);
}

// static
bool ChildProcessLauncher::TerminateProcess(const base::Process& process,
                                            int exit_code) {
  return ChildProcessLauncherHelper::TerminateProcess(process, exit_code);
}

// static
void ChildProcessLauncher::SetRegisteredFilesForService(
    const std::string& service_name,
    catalog::RequiredFileMap required_files) {
  ChildProcessLauncherHelper::SetRegisteredFilesForService(
      service_name, std::move(required_files));
}

// static
void ChildProcessLauncher::ResetRegisteredFilesForTesting() {
  ChildProcessLauncherHelper::ResetRegisteredFilesForTesting();
}

ChildProcessLauncher::Client* ChildProcessLauncher::ReplaceClientForTest(
    Client* client) {
  Client* ret = client_;
  client_ = client;
  return ret;
}

bool ChildProcessLauncherPriority::operator==(
    const ChildProcessLauncherPriority& other) const {
  return background == other.background && frame_depth == other.frame_depth &&
         boost_for_pending_views == other.boost_for_pending_views
#if defined(OS_ANDROID)
         && importance == other.importance
#endif
      ;
}

}  // namespace content
