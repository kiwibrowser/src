// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/remote_commands/testing_remote_commands_server.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace em = enterprise_management;

namespace policy {

struct TestingRemoteCommandsServer::RemoteCommandWithCallback {
  RemoteCommandWithCallback(const em::RemoteCommand& command_proto,
                            base::TimeTicks issued_time,
                            const ResultReportedCallback& reported_callback)
      : command_proto(command_proto),
        issued_time(issued_time),
        reported_callback(reported_callback) {}
  ~RemoteCommandWithCallback() {}

  em::RemoteCommand command_proto;
  base::TimeTicks issued_time;
  ResultReportedCallback reported_callback;
};

TestingRemoteCommandsServer::TestingRemoteCommandsServer()
    : clock_(base::DefaultTickClock::GetInstance()),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {
  weak_ptr_to_this_ = weak_factory_.GetWeakPtr();
}

TestingRemoteCommandsServer::~TestingRemoteCommandsServer() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Commands are removed from the queue when a result is reported. Only
  // commands for which no result was expected should remain in the queue.
  for (const auto& command_with_callback : commands_)
    EXPECT_TRUE(command_with_callback.reported_callback.is_null());
}

void TestingRemoteCommandsServer::IssueCommand(
    em::RemoteCommand_Type type,
    const std::string& payload,
    const ResultReportedCallback& reported_callback,
    bool skip_next_fetch) {
  DCHECK(thread_checker_.CalledOnValidThread());

  base::AutoLock auto_lock(lock_);

  em::RemoteCommand command;
  command.set_type(type);
  command.set_command_id(++last_generated_unique_id_);
  if (!payload.empty())
    command.set_payload(payload);

  const RemoteCommandWithCallback command_with_callback(
      command, clock_->NowTicks(), reported_callback);
  if (skip_next_fetch)
    commands_issued_after_next_fetch_.push_back(command_with_callback);
  else
    commands_.push_back(command_with_callback);
}

TestingRemoteCommandsServer::RemoteCommands
TestingRemoteCommandsServer::FetchCommands(
    std::unique_ptr<RemoteCommandJob::UniqueIDType> last_command_id,
    const RemoteCommandResults& previous_job_results) {
  base::AutoLock auto_lock(lock_);

  for (const auto& job_result : previous_job_results) {
    EXPECT_TRUE(job_result.has_command_id());
    EXPECT_TRUE(job_result.has_result());

    bool found_command = false;
    ResultReportedCallback reported_callback;

    if (last_command_id) {
      // This relies on us generating commands with increasing IDs.
      EXPECT_GE(*last_command_id, job_result.command_id());
    }

    for (auto it = commands_.begin(); it != commands_.end(); ++it) {
      if (it->command_proto.command_id() == job_result.command_id()) {
        reported_callback = it->reported_callback;
        commands_.erase(it);
        found_command = true;
        break;
      }
    }

    // Verify that the command result is for an existing command actually
    // expecting a result.
    EXPECT_TRUE(found_command);
    EXPECT_FALSE(reported_callback.is_null());

    if (reported_callback.is_null()) {
      // Post task to the original thread which will report the result.
      task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&TestingRemoteCommandsServer::ReportJobResult,
                     weak_ptr_to_this_, reported_callback, job_result));
    }
  }

  RemoteCommands fetched_commands;
  for (const auto& command_with_callback : commands_) {
    if (!last_command_id ||
        command_with_callback.command_proto.command_id() > *last_command_id) {
      fetched_commands.push_back(command_with_callback.command_proto);
      // Simulate the age of commands calculation on the server side.
      fetched_commands.back().set_age_of_command(
          (clock_->NowTicks() - command_with_callback.issued_time)
              .InMilliseconds());
    }
  }

  // Push delayed commands into the main queue.
  commands_.insert(commands_.end(), commands_issued_after_next_fetch_.begin(),
                   commands_issued_after_next_fetch_.end());
  commands_issued_after_next_fetch_.clear();

  return fetched_commands;
}

void TestingRemoteCommandsServer::SetClock(const base::TickClock* clock) {
  DCHECK(thread_checker_.CalledOnValidThread());
  clock_ = clock;
}

size_t TestingRemoteCommandsServer::NumberOfCommandsPendingResult() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return commands_.size();
}

void TestingRemoteCommandsServer::ReportJobResult(
    const ResultReportedCallback& reported_callback,
    const em::RemoteCommandResult& job_result) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  reported_callback.Run(job_result);
}

}  // namespace policy
