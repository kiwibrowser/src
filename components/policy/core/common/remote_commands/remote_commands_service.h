// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_REMOTE_COMMANDS_REMOTE_COMMANDS_SERVICE_H_
#define COMPONENTS_POLICY_CORE_COMMON_REMOTE_COMMANDS_REMOTE_COMMANDS_SERVICE_H_

#include <memory>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "components/policy/core/common/remote_commands/remote_command_job.h"
#include "components/policy/core/common/remote_commands/remote_commands_queue.h"
#include "components/policy/policy_export.h"
#include "components/policy/proto/device_management_backend.pb.h"

namespace base {
class TickClock;
}  // namespace base

namespace policy {

class CloudPolicyClient;
class RemoteCommandsFactory;

// Service class which will connect to a CloudPolicyClient in order to fetch
// remote commands from DMServer and send results for executed commands
// back to the server.
class POLICY_EXPORT RemoteCommandsService
    : public RemoteCommandsQueue::Observer {
 public:
  RemoteCommandsService(std::unique_ptr<RemoteCommandsFactory> factory,
                        CloudPolicyClient* client);
  ~RemoteCommandsService() override;

  // Attempts to fetch remote commands, mainly supposed to be called by
  // invalidation service. Note that there will be at most one ongoing fetch
  // request and all other fetch request will be enqueued if another fetch
  // request is in-progress. And in such a case, another request will be made
  // immediately after the current ongoing request finishes.
  // Returns true if the new request was started immediately. Returns false if
  // another request was in progress already and the new request got enqueued.
  bool FetchRemoteCommands();

  // Returns whether a command fetch request is in progress or not.
  bool IsCommandFetchInProgressForTesting() const {
    return command_fetch_in_progress_;
  }

  // Set an alternative clock for testing.
  void SetClockForTesting(const base::TickClock* clock);

 private:
  // Helper function to enqueue a command which we get from server.
  void EnqueueCommand(const enterprise_management::RemoteCommand& command);

  // RemoteCommandsQueue::Observer:
  void OnJobStarted(RemoteCommandJob* command) override;
  void OnJobFinished(RemoteCommandJob* command) override;

  // Callback to handle commands we get from the server.
  void OnRemoteCommandsFetched(
      DeviceManagementStatus status,
      const std::vector<enterprise_management::RemoteCommand>& commands);

  // Whether there is a command fetch on going or not.
  bool command_fetch_in_progress_ = false;

  // Whether there is an enqueued fetch request, which indicates there were
  // additional FetchRemoteCommands() calls while a fetch request was ongoing.
  bool has_enqueued_fetch_request_ = false;

  // Command results that have not been sent back to the server yet.
  std::vector<enterprise_management::RemoteCommandResult> unsent_results_;

  // Whether at least one command has finished executing.
  bool has_finished_command_ = false;

  // ID of the latest command which has finished execution if
  // |has_finished_command_| is true. We will acknowledge this ID to the
  // server so that we can re-fetch commands that have not been executed yet
  // after a crash or browser restart.
  RemoteCommandJob::UniqueIDType lastest_finished_command_id_;

  // Collects the IDs of all fetched commands. We need this since the command ID
  // is opaque.
  // IDs will be stored in the order that they are fetched from the server,
  // and acknowledging a command will discard its ID from
  // |fetched_command_ids_|, as well as the IDs of every command before it.
  base::circular_deque<RemoteCommandJob::UniqueIDType> fetched_command_ids_;

  RemoteCommandsQueue queue_;
  std::unique_ptr<RemoteCommandsFactory> factory_;
  CloudPolicyClient* const client_;

  base::WeakPtrFactory<RemoteCommandsService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RemoteCommandsService);
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_REMOTE_COMMANDS_REMOTE_COMMANDS_SERVICE_H_
