// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/remote_commands/remote_commands_service.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/syslog_logging.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "components/policy/core/common/cloud/cloud_policy_client.h"
#include "components/policy/core/common/remote_commands/remote_commands_factory.h"

namespace policy {

namespace em = enterprise_management;

RemoteCommandsService::RemoteCommandsService(
    std::unique_ptr<RemoteCommandsFactory> factory,
    CloudPolicyClient* client)
    : factory_(std::move(factory)), client_(client), weak_factory_(this) {
  DCHECK(client_);
  queue_.AddObserver(this);
}

RemoteCommandsService::~RemoteCommandsService() {
  queue_.RemoveObserver(this);
}

bool RemoteCommandsService::FetchRemoteCommands() {
  // TODO(hunyadym): Remove after crbug.com/582506 is fixed.
  SYSLOG(INFO) << "Fetching remote commands.";
  if (!client_->is_registered()) {
    SYSLOG(WARNING) << "Client is not registered.";
    return false;
  }

  if (command_fetch_in_progress_) {
    // TODO(hunyadym): Remove after crbug.com/582506 is fixed.
    SYSLOG(WARNING) << "Command fetch is already in progress.";
    has_enqueued_fetch_request_ = true;
    return false;
  }

  command_fetch_in_progress_ = true;
  has_enqueued_fetch_request_ = false;

  std::vector<em::RemoteCommandResult> previous_results;
  unsent_results_.swap(previous_results);

  std::unique_ptr<RemoteCommandJob::UniqueIDType> id_to_acknowledge;

  if (has_finished_command_) {
    // Acknowledges |lastest_finished_command_id_|, and removes it and every
    // command before it from |fetched_command_ids_|.
    id_to_acknowledge.reset(
        new RemoteCommandJob::UniqueIDType(lastest_finished_command_id_));
    // It's safe to remove these IDs from |fetched_command_ids_| here, since
    // it is guaranteed that there is no earlier fetch request in progress
    // anymore that could have returned these IDs.
    while (!fetched_command_ids_.empty() &&
           fetched_command_ids_.front() != lastest_finished_command_id_) {
      fetched_command_ids_.pop_front();
    }
  }

  client_->FetchRemoteCommands(
      std::move(id_to_acknowledge), previous_results,
      base::Bind(&RemoteCommandsService::OnRemoteCommandsFetched,
                 weak_factory_.GetWeakPtr()));

  return true;
}

void RemoteCommandsService::SetClockForTesting(const base::TickClock* clock) {
  queue_.SetClockForTesting(clock);
}

void RemoteCommandsService::EnqueueCommand(
    const enterprise_management::RemoteCommand& command) {
  if (!command.has_type() || !command.has_command_id()) {
    SYSLOG(ERROR) << "Invalid remote command from server.";
    return;
  }

  // If the command is already fetched, ignore it.
  if (std::find(fetched_command_ids_.begin(), fetched_command_ids_.end(),
                command.command_id()) != fetched_command_ids_.end()) {
    return;
  }

  fetched_command_ids_.push_back(command.command_id());

  std::unique_ptr<RemoteCommandJob> job =
      factory_->BuildJobForType(command.type());

  if (!job || !job->Init(queue_.GetNowTicks(), command)) {
    SYSLOG(ERROR) << "Initialization of remote command failed.";
    em::RemoteCommandResult ignored_result;
    ignored_result.set_result(
        em::RemoteCommandResult_ResultType_RESULT_IGNORED);
    ignored_result.set_command_id(command.command_id());
    unsent_results_.push_back(ignored_result);
    return;
  }

  queue_.AddJob(std::move(job));
}

void RemoteCommandsService::OnJobStarted(RemoteCommandJob* command) {
}

void RemoteCommandsService::OnJobFinished(RemoteCommandJob* command) {
  has_finished_command_ = true;
  lastest_finished_command_id_ = command->unique_id();
  // TODO(binjin): Attempt to sync |lastest_finished_command_id_| to some
  // persistent source, so that we can reload it later without relying solely on
  // the server to keep our last acknowledged command ID.
  // See http://crbug.com/466572.

  em::RemoteCommandResult result;
  result.set_command_id(command->unique_id());
  result.set_timestamp((command->execution_started_time() -
                        base::TimeTicks::UnixEpoch()).InMilliseconds());

  if (command->status() == RemoteCommandJob::SUCCEEDED ||
      command->status() == RemoteCommandJob::FAILED) {
    if (command->status() == RemoteCommandJob::SUCCEEDED)
      result.set_result(em::RemoteCommandResult_ResultType_RESULT_SUCCESS);
    else
      result.set_result(em::RemoteCommandResult_ResultType_RESULT_FAILURE);
    const std::unique_ptr<std::string> result_payload =
        command->GetResultPayload();
    if (result_payload)
      result.set_payload(*result_payload);
  } else if (command->status() == RemoteCommandJob::EXPIRED ||
             command->status() == RemoteCommandJob::INVALID) {
    result.set_result(em::RemoteCommandResult_ResultType_RESULT_IGNORED);
  } else {
    NOTREACHED();
  }

  SYSLOG(INFO) << "Remote command " << command->unique_id()
               << " finished with result " << result.result();

  unsent_results_.push_back(result);

  FetchRemoteCommands();
}

void RemoteCommandsService::OnRemoteCommandsFetched(
    DeviceManagementStatus status,
    const std::vector<enterprise_management::RemoteCommand>& commands) {
  DCHECK(command_fetch_in_progress_);
  // TODO(hunyadym): Remove after crbug.com/582506 is fixed.
  SYSLOG(INFO) << "Remote commands fetched.";
  command_fetch_in_progress_ = false;

  // TODO(binjin): Add retrying on errors. See http://crbug.com/466572.
  if (status == DM_STATUS_SUCCESS) {
    for (const auto& command : commands)
      EnqueueCommand(command);
  }

  // Start another fetch request job immediately if there are unsent command
  // results or enqueued fetch requests.
  if (!unsent_results_.empty() || has_enqueued_fetch_request_)
    FetchRemoteCommands();
}

}  // namespace policy
