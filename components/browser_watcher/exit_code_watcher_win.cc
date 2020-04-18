// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_watcher/exit_code_watcher_win.h"

#include <utility>

#include "base/logging.h"
#include "base/process/kill.h"
#include "base/strings/stringprintf.h"
#include "base/win/registry.h"

#include <windows.h>

namespace browser_watcher {

namespace {

base::string16 GetValueName(const base::Time creation_time,
                            base::ProcessId pid) {
  // Convert the PID and creation time to a string value unique to this
  // process instance.
  return base::StringPrintf(L"%d-%lld", pid, creation_time.ToInternalValue());
}

}  // namespace

ExitCodeWatcher::ExitCodeWatcher(base::StringPiece16 registry_path)
    : registry_path_(registry_path.as_string()), exit_code_(STILL_ACTIVE) {}

ExitCodeWatcher::~ExitCodeWatcher() {
}

bool ExitCodeWatcher::Initialize(base::Process process) {
  if (!process.IsValid()) {
    LOG(ERROR) << "Invalid parent handle, can't get parent process ID.";
    return false;
  }

  DWORD process_pid = process.Pid();
  if (process_pid == 0) {
    LOG(ERROR) << "Invalid parent handle, can't get parent process ID.";
    return false;
  }

  FILETIME creation_time = {};
  FILETIME dummy = {};
  if (!::GetProcessTimes(process.Handle(), &creation_time, &dummy, &dummy,
                         &dummy)) {
    PLOG(ERROR) << "Invalid parent handle, can't get parent process times.";
    return false;
  }

  // Success, take ownership of the process.
  process_ = std::move(process);
  process_creation_time_ = base::Time::FromFileTime(creation_time);

  // Start by writing the value STILL_ACTIVE to registry, to allow detection
  // of the case where the watcher itself is somehow terminated before it can
  // write the process' actual exit code.
  return WriteProcessExitCode(STILL_ACTIVE);
}

void ExitCodeWatcher::WaitForExit() {
  if (!process_.WaitForExit(&exit_code_)) {
    LOG(ERROR) << "Failed to wait for process.";
    return;
  }

  WriteProcessExitCode(exit_code_);
}

bool ExitCodeWatcher::WriteProcessExitCode(int exit_code) {
  base::win::RegKey key(HKEY_CURRENT_USER,
                        registry_path_.c_str(),
                        KEY_WRITE);
  base::string16 value_name(
      GetValueName(process_creation_time_, process_.Pid()));

  ULONG result = key.WriteValue(value_name.c_str(), exit_code);
  if (result != ERROR_SUCCESS) {
    LOG(ERROR) << "Unable to write to registry, error " << result;
    return false;
  }

  return true;
}

}  // namespace browser_watcher
