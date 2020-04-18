// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/remote_commands/user_command_arc_job.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/syslog_logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chromeos/arc/policy/arc_policy_bridge.h"
#include "chrome/browser/profiles/profile.h"
#include "components/arc/common/policy.mojom.h"
#include "components/policy/proto/device_management_backend.pb.h"

namespace policy {

UserCommandArcJob::UserCommandArcJob(Profile* profile) : profile_(profile) {}

UserCommandArcJob::~UserCommandArcJob() = default;

enterprise_management::RemoteCommand_Type UserCommandArcJob::GetType() const {
  return enterprise_management::RemoteCommand_Type_USER_ARC_COMMAND;
}

bool UserCommandArcJob::ParseCommandPayload(
    const std::string& command_payload) {
  command_payload_ = command_payload;
  return true;
}

void UserCommandArcJob::RunImpl(const CallbackWithResult& succeeded_callback,
                                const CallbackWithResult& failed_callback) {
  SYSLOG(INFO) << "Running Arc command, payload = " << command_payload_;

  auto* const arc_policy_bridge =
      arc::ArcPolicyBridge::GetForBrowserContext(profile_);

  if (!arc_policy_bridge) {
    // ARC is not enabled for this profile, fail the remote command.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(failed_callback, nullptr));
    return;
  }

  auto on_command_finished_callback = base::BindOnce(
      [](const CallbackWithResult& succeeded_callback,
         const CallbackWithResult& failed_callback,
         arc::mojom::CommandResultType result) {
        if (result == arc::mojom::CommandResultType::FAILURE ||
            result == arc::mojom::CommandResultType::IGNORED) {
          failed_callback.Run(nullptr);
          return;
        }
        succeeded_callback.Run(nullptr);
      },
      succeeded_callback, failed_callback);

  // Documentation for RemoteCommandJob::RunImpl requires that the
  // implementation executes the command asynchronously.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&arc::ArcPolicyBridge::OnCommandReceived,
                     arc_policy_bridge->GetWeakPtr(), command_payload_,
                     std::move(on_command_finished_callback)));
}

}  // namespace policy
