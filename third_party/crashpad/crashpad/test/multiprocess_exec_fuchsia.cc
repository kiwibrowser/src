// Copyright 2018 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "test/multiprocess_exec.h"

#include <launchpad/launchpad.h>
#include <zircon/process.h>
#include <zircon/syscalls.h>

#include "base/files/scoped_file.h"
#include "base/fuchsia/fuchsia_logging.h"
#include "base/fuchsia/scoped_zx_handle.h"
#include "gtest/gtest.h"

namespace crashpad {
namespace test {

namespace internal {

struct MultiprocessInfo {
  MultiprocessInfo() {}
  base::ScopedFD stdin_write;
  base::ScopedFD stdout_read;
  base::ScopedZxHandle child;
};

}  // namespace internal

Multiprocess::Multiprocess()
    : info_(nullptr),
      code_(EXIT_SUCCESS),
      reason_(kTerminationNormal) {
}

void Multiprocess::Run() {
  // Set up and spawn the child process.
  ASSERT_NO_FATAL_FAILURE(PreFork());
  RunChild();

  // And then run the parent actions in this process.
  RunParent();

  // Wait until the child exits.
  zx_signals_t signals;
  ASSERT_EQ(
      zx_object_wait_one(
          info_->child.get(), ZX_TASK_TERMINATED, ZX_TIME_INFINITE, &signals),
      ZX_OK);
  ASSERT_EQ(signals, ZX_TASK_TERMINATED);

  // Get the child's exit code.
  zx_info_process_t proc_info;
  zx_status_t status = zx_object_get_info(info_->child.get(),
                                          ZX_INFO_PROCESS,
                                          &proc_info,
                                          sizeof(proc_info),
                                          nullptr,
                                          nullptr);
  if (status != ZX_OK) {
    ZX_LOG(ERROR, status) << "zx_object_get_info";
    ADD_FAILURE() << "Unable to get exit code of child";
  } else {
    if (code_ != proc_info.return_code) {
      ADD_FAILURE() << "Child exited with code " << proc_info.return_code
                    << ", expected exit with code " << code_;
    }
  }
}

void Multiprocess::SetExpectedChildTermination(TerminationReason reason,
                                               int code) {
  EXPECT_EQ(info_, nullptr)
      << "SetExpectedChildTermination() must be called before Run()";
  reason_ = reason;
  code_ = code;
}

void Multiprocess::SetExpectedChildTerminationBuiltinTrap() {
  SetExpectedChildTermination(kTerminationNormal, -1);
}

Multiprocess::~Multiprocess() {
  delete info_;
}

FileHandle Multiprocess::ReadPipeHandle() const {
  return info_->stdout_read.get();
}

FileHandle Multiprocess::WritePipeHandle() const {
  return info_->stdin_write.get();
}

void Multiprocess::CloseReadPipe() {
  info_->stdout_read.reset();
}

void Multiprocess::CloseWritePipe() {
  info_->stdin_write.reset();
}

void Multiprocess::RunParent() {
  MultiprocessParent();

  info_->stdout_read.reset();
  info_->stdin_write.reset();
}

void Multiprocess::RunChild() {
  MultiprocessChild();
}

MultiprocessExec::MultiprocessExec()
    : Multiprocess(), command_(), arguments_(), argv_() {
}

void MultiprocessExec::SetChildCommand(
    const base::FilePath& command,
    const std::vector<std::string>* arguments) {
  command_ = command;
  if (arguments) {
    arguments_ = *arguments;
  } else {
    arguments_.clear();
  }
}

MultiprocessExec::~MultiprocessExec() {}

void MultiprocessExec::PreFork() {
  ASSERT_FALSE(command_.empty());

  ASSERT_TRUE(argv_.empty());

  argv_.push_back(command_.value().c_str());
  for (const std::string& argument : arguments_) {
    argv_.push_back(argument.c_str());
  }

  ASSERT_EQ(info(), nullptr);
  set_info(new internal::MultiprocessInfo());
}

void MultiprocessExec::MultiprocessChild() {
  launchpad_t* lp;
  launchpad_create(zx_job_default(), command_.value().c_str(), &lp);
  launchpad_load_from_file(lp, command_.value().c_str());
  launchpad_set_args(lp, argv_.size(), &argv_[0]);

  // Pass the filesystem namespace, parent environment, and default job to the
  // child, but don't include any other file handles, preferring to set them
  // up explicitly below.
  launchpad_clone(
      lp, LP_CLONE_FDIO_NAMESPACE | LP_CLONE_ENVIRON | LP_CLONE_DEFAULT_JOB);

  int stdin_parent_side;
  launchpad_add_pipe(lp, &stdin_parent_side, STDIN_FILENO);
  info()->stdin_write.reset(stdin_parent_side);

  int stdout_parent_side;
  launchpad_add_pipe(lp, &stdout_parent_side, STDOUT_FILENO);
  info()->stdout_read.reset(stdout_parent_side);

  launchpad_clone_fd(lp, STDERR_FILENO, STDERR_FILENO);

  const char* error_message;
  zx_handle_t child;
  zx_status_t status = launchpad_go(lp, &child, &error_message);
  ZX_CHECK(status == ZX_OK, status) << "launchpad_go: " << error_message;
  info()->child.reset(child);
}

ProcessType MultiprocessExec::ChildProcess() {
  return info()->child.get();
}

}  // namespace test
}  // namespace crashpad
