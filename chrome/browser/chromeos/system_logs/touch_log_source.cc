// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/system_logs/touch_log_source.h"

#include <stddef.h>

#include "ash/public/cpp/config.h"
#include "ash/touch/touch_hud_debug.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part_chromeos.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "content/public/browser/browser_thread.h"
#include "services/ui/public/cpp/input_devices/input_device_controller_client.h"

using content::BrowserThread;

namespace {

const char kHUDLogDataKey[] = "hud_log";

// The prefix "hack-33025" was historically chosen in http://crbug.com/139715.
// We continue to go with it in order to be compatible with the existing touch
// log processing toolchain.
const char kDeviceStatusLogDataKey[] = "hack-33025-touchpad";
const char kTouchpadEventLogDataKey[] = "hack-33025-touchpad_activity";
const char kTouchscreenEventLogDataKey[] = "hack-33025-touchscreen_activity";

// Directory for temp touch event logs.
const char kTouchEventLogDir[] = "/home/chronos/user/log";

// Prefixes of touch event logs.
const char kTouchpadGestureLogPrefix[] = "touchpad_activity_";
const char kTouchscreenLogPrefix[] = "evdev_input_events_";
const char kTouchpadEvdevLogPrefix[] = "cmt_input_events_";

// Binary paths.
const char kShellCommand[] = "/bin/sh";
const char kTarCommand[] = "/bin/tar cf -";
const char kUuencodeCommand[] = "/usr/bin/uuencode";

const int kMaxDeviceTouchEventLogs = 7;

// Clean up intermediate log files dumped during feedback creation.
void CleanupEventLog(const std::vector<base::FilePath>& log_paths) {
  for (const base::FilePath& path : log_paths)
    base::DeleteFile(path, false);
}

// Check for all known log paths and find the ones whose filenames match a
// prefix. Concatenate their filenames into one string. |max_log_count| is
// the maximum number of logs that we will collect.
//
// This is used to distinguish touchpad/mice logs from touchscreen logs.
std::string GetEventLogListOfOnePrefix(
    const std::vector<base::FilePath>& log_paths,
    const std::string& prefix,
    const int max_log_count) {
  int collected = 0;
  std::string log_list;
  for (size_t i = 0; i < log_paths.size(); ++i) {
    const std::string basename = log_paths[i].BaseName().value();
    if (base::StartsWith(basename, prefix, base::CompareCase::SENSITIVE)) {
      log_list.append(" " + log_paths[i].value());

      // Limit the max number of collected logs to shorten the log collection
      // process.
      if (++collected >= max_log_count)
        break;
    }
  }

  return log_list;
}

// Pack the collected event logs in a way that is compatible with the log
// analysis tools.
void PackEventLog(system_logs::SystemLogsResponse* response,
                  const std::vector<base::FilePath>& log_paths) {
  // Combine logs with a command line call that tars them up and uuencode the
  // result in one string.
  std::vector<std::pair<std::string, base::CommandLine>> commands;
  base::CommandLine command = base::CommandLine(base::FilePath(kShellCommand));
  command.AppendArg("-c");

  // Make a list that contains touchpad (and mouse) event logs only.
  const std::string touchpad_log_list =
      GetEventLogListOfOnePrefix(log_paths, kTouchpadGestureLogPrefix,
                                 kMaxDeviceTouchEventLogs) +
      GetEventLogListOfOnePrefix(log_paths, kTouchpadEvdevLogPrefix,
                                 kMaxDeviceTouchEventLogs);
  command.AppendArg(std::string(kTarCommand) + touchpad_log_list +
                    " 2>/dev/null | " + kUuencodeCommand +
                    " -m touchpad_activity_log.tar");

  commands.push_back(std::make_pair(kTouchpadEventLogDataKey, command));

  base::CommandLine ts_command =
      base::CommandLine(base::FilePath(kShellCommand));
  ts_command.AppendArg("-c");

  // Make a list that contains touchscreen event logs only.
  const std::string touchscreen_log_list = GetEventLogListOfOnePrefix(
      log_paths, kTouchscreenLogPrefix, kMaxDeviceTouchEventLogs);
  ts_command.AppendArg(std::string(kTarCommand) + touchscreen_log_list +
                       " 2>/dev/null | " + kUuencodeCommand +
                       " -m touchscreen_activity_log.tar");

  commands.push_back(std::make_pair(kTouchscreenEventLogDataKey, ts_command));

  // For now only touchpad (and mouse) logs are actually collected.
  for (size_t i = 0; i < commands.size(); ++i) {
    std::string output;
    base::GetAppOutput(commands[i].second, &output);
    (*response)[commands[i].first] = output;
  }

  // Cleanup these temporary log files.
  base::PostTaskWithTraits(FROM_HERE,
                           {base::MayBlock(), base::TaskPriority::BACKGROUND},
                           base::Bind(CleanupEventLog, log_paths));
}

// Callback for handing the outcome of GetTouchEventLog().
//
// This is the end of the whole touch log collection process.
void OnEventLogCollected(
    std::unique_ptr<system_logs::SystemLogsResponse> response,
    system_logs::SysLogsSourceCallback callback,
    const std::vector<base::FilePath>& log_paths) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  system_logs::SystemLogsResponse* response_ptr = response.get();
  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&PackEventLog, response_ptr, log_paths),
      base::BindOnce(std::move(callback), std::move(response)));
}

// Callback for handing the outcome of GetTouchDeviceStatus().
//
// Appends the collected log to the SystemLogsResponse map. Also goes on to
// collect touch event logs.
void OnStatusLogCollected(
    std::unique_ptr<system_logs::SystemLogsResponse> response,
    system_logs::SysLogsSourceCallback callback,
    const std::string& log) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  (*response)[kDeviceStatusLogDataKey] = log;

  // Collect touch event logs.
  const base::FilePath kBaseLogPath(kTouchEventLogDir);
  ui::InputDeviceControllerClient* input_device_controller_client =
      g_browser_process->platform_part()->GetInputDeviceControllerClient();
  input_device_controller_client->GetTouchEventLog(
      kBaseLogPath, base::BindOnce(&OnEventLogCollected, std::move(response),
                                   std::move(callback)));
}

// Collect touch HUD debug logs. This needs to be done on the UI thread.
void CollectTouchHudDebugLog(system_logs::SystemLogsResponse* response) {
  // TODO(mash): Collect this data from window server over mojo.
  if (chromeos::GetAshConfig() == ash::Config::MASH) {
    NOTIMPLEMENTED();
    return;
  }
  std::unique_ptr<base::DictionaryValue> dictionary =
      ash::TouchHudDebug::GetAllAsDictionary();
  if (!dictionary->empty()) {
    std::string touch_log;
    JSONStringValueSerializer json(&touch_log);
    json.set_pretty_print(true);
    if (json.Serialize(*dictionary) && !touch_log.empty())
      (*response)[kHUDLogDataKey] = touch_log;
  }
}

}  // namespace

namespace system_logs {

void TouchLogSource::Fetch(SysLogsSourceCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!callback.is_null());

  auto response = std::make_unique<SystemLogsResponse>();
  CollectTouchHudDebugLog(response.get());

  // Collect touch device status logs.
  ui::InputDeviceControllerClient* input_device_controller_client =
      g_browser_process->platform_part()->GetInputDeviceControllerClient();
  input_device_controller_client->GetTouchDeviceStatus(base::BindOnce(
      &OnStatusLogCollected, std::move(response), std::move(callback)));
}

}  // namespace system_logs
