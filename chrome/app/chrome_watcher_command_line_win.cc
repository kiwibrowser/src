// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/chrome_watcher_command_line_win.h"

#include <stdint.h>

#include <string>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/win/win_util.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/common/content_switches.h"

namespace {

const char kMainThreadIdSwitch[] = "main-thread-id";
const char kOnIninitializedEventHandleSwitch[] = "on-initialized-event-handle";
const char kParentHandleSwitch[] = "parent-handle";

void AppendHandleSwitch(const std::string& switch_name,
                        HANDLE handle,
                        base::CommandLine* command_line) {
  command_line->AppendSwitchASCII(
      switch_name, base::UintToString(base::win::HandleToUint32(handle)));
}

uint32_t ReadUintSwitch(const base::CommandLine& command_line,
                            const std::string& switch_name) {
  std::string switch_string = command_line.GetSwitchValueASCII(switch_name);
  unsigned int switch_uint = 0;
  if (switch_string.empty() ||
      !base::StringToUint(switch_string, &switch_uint)) {
    DLOG(ERROR) << "Missing or invalid " << switch_name << " argument.";
    return 0;
  }
  return switch_uint;
}

HANDLE ReadHandleFromSwitch(const base::CommandLine& command_line,
                            const std::string& switch_name) {
  return reinterpret_cast<HANDLE>(ReadUintSwitch(command_line, switch_name));
}

}  // namespace

ChromeWatcherCommandLineGenerator::ChromeWatcherCommandLineGenerator(
    const base::FilePath& chrome_exe) : chrome_exe_(chrome_exe) {
}

ChromeWatcherCommandLineGenerator::~ChromeWatcherCommandLineGenerator() {
}

bool ChromeWatcherCommandLineGenerator::SetOnInitializedEventHandle(
    HANDLE on_initialized_event_handle) {
  return SetHandle(on_initialized_event_handle, &on_initialized_event_handle_);
}

bool ChromeWatcherCommandLineGenerator::SetParentProcessHandle(
    HANDLE parent_process_handle) {
  return SetHandle(parent_process_handle, &parent_process_handle_);
}

// Generates a command-line representing this configuration.
base::CommandLine ChromeWatcherCommandLineGenerator::GenerateCommandLine() {
  // TODO(chrisha): Get rid of the following function and move the
  // implementation here.
  return GenerateChromeWatcherCommandLine(
      chrome_exe_, parent_process_handle_.Get(), ::GetCurrentThreadId(),
      on_initialized_event_handle_.Get());
}

void ChromeWatcherCommandLineGenerator::GetInheritedHandles(
    std::vector<HANDLE>* inherited_handles) const {
  if (on_initialized_event_handle_.IsValid())
    inherited_handles->push_back(on_initialized_event_handle_.Get());
  if (parent_process_handle_.IsValid())
    inherited_handles->push_back(parent_process_handle_.Get());
}

bool ChromeWatcherCommandLineGenerator::SetHandle(
    HANDLE handle, base::win::ScopedHandle* scoped_handle) {
  // Create a duplicate handle that is inheritable.
  HANDLE proc = ::GetCurrentProcess();
  HANDLE new_handle = 0;
  if (!::DuplicateHandle(proc, handle, proc, &new_handle, 0, TRUE,
                         DUPLICATE_SAME_ACCESS)) {
    return false;
  }

  scoped_handle->Set(new_handle);
  return true;
}

ChromeWatcherCommandLine::ChromeWatcherCommandLine(
    HANDLE on_initialized_event_handle,
    HANDLE parent_process_handle,
    DWORD main_thread_id)
    : on_initialized_event_handle_(on_initialized_event_handle),
      parent_process_handle_(parent_process_handle),
      main_thread_id_(main_thread_id) {
}

ChromeWatcherCommandLine::~ChromeWatcherCommandLine() {
  // If any handles were not taken then die violently.
  CHECK(!on_initialized_event_handle_.IsValid() &&
        !parent_process_handle_.IsValid())
      << "Handles left untaken.";
}

std::unique_ptr<ChromeWatcherCommandLine>
ChromeWatcherCommandLine::InterpretCommandLine(
    const base::CommandLine& command_line) {
  base::win::ScopedHandle on_initialized_event_handle;
  base::win::ScopedHandle parent_process_handle;
  DWORD main_thread_id = 0;

  // TODO(chrisha): Get rid of the following function and move the
  // implementation here.
  if (!InterpretChromeWatcherCommandLine(
      command_line, &parent_process_handle, &main_thread_id,
      &on_initialized_event_handle))
    return std::unique_ptr<ChromeWatcherCommandLine>();

  return std::unique_ptr<ChromeWatcherCommandLine>(new ChromeWatcherCommandLine(
      on_initialized_event_handle.Take(), parent_process_handle.Take(),
      main_thread_id));
}

base::win::ScopedHandle
ChromeWatcherCommandLine::TakeOnInitializedEventHandle() {
  return std::move(on_initialized_event_handle_);
}

base::win::ScopedHandle ChromeWatcherCommandLine::TakeParentProcessHandle() {
  return std::move(parent_process_handle_);
}

base::CommandLine GenerateChromeWatcherCommandLine(
    const base::FilePath& chrome_exe,
    HANDLE parent_process,
    DWORD main_thread_id,
    HANDLE on_initialized_event) {
  base::CommandLine command_line(chrome_exe);
  command_line.AppendSwitchASCII(switches::kProcessType,
                                 switches::kWatcherProcess);
  command_line.AppendSwitchASCII(kMainThreadIdSwitch,
                                 base::UintToString(main_thread_id));
  AppendHandleSwitch(kOnIninitializedEventHandleSwitch, on_initialized_event,
                     &command_line);
  AppendHandleSwitch(kParentHandleSwitch, parent_process, &command_line);

  command_line.AppendArg(switches::kPrefetchArgumentWatcher);

  // Copy over logging switches.
  static const char* const kSwitchNames[] = {switches::kEnableLogging,
                                             switches::kV, switches::kVModule};
  base::CommandLine current_command_line =
      *base::CommandLine::ForCurrentProcess();
  command_line.CopySwitchesFrom(current_command_line, kSwitchNames,
                                arraysize(kSwitchNames));

  return command_line;
}

bool InterpretChromeWatcherCommandLine(
    const base::CommandLine& command_line,
    base::win::ScopedHandle* parent_process,
    DWORD* main_thread_id,
    base::win::ScopedHandle* on_initialized_event) {
  DCHECK(on_initialized_event);
  DCHECK(parent_process);

  // For consistency, always close any existing HANDLEs here.
  on_initialized_event->Close();
  parent_process->Close();

  HANDLE parent_handle =
      ReadHandleFromSwitch(command_line, kParentHandleSwitch);
  HANDLE on_initialized_event_handle =
      ReadHandleFromSwitch(command_line, kOnIninitializedEventHandleSwitch);
  *main_thread_id = ReadUintSwitch(command_line, kMainThreadIdSwitch);

  if (parent_handle) {
    // Initial test of the handle, a zero PID indicates invalid handle, or not
    // a process handle. In this case, parsing fails and we avoid closing the
    // handle.
    DWORD process_pid = ::GetProcessId(parent_handle);
    if (process_pid == 0) {
      DLOG(ERROR) << "Invalid " << kParentHandleSwitch
                  << " argument. Can't get parent PID.";
    } else {
      parent_process->Set(parent_handle);
    }
  }

  if (on_initialized_event_handle) {
    DWORD result = ::WaitForSingleObject(on_initialized_event_handle, 0);
    if (result == WAIT_FAILED) {
      DPLOG(ERROR)
          << "Unexpected error while testing the initialization event.";
    } else if (result != WAIT_TIMEOUT) {
      DLOG(ERROR) << "Unexpected result while testing the initialization event "
                     "with WaitForSingleObject: " << result;
    } else {
      on_initialized_event->Set(on_initialized_event_handle);
    }
  }

  if (!*main_thread_id || !on_initialized_event->IsValid() ||
      !parent_process->IsValid()) {
    // If one was valid and not the other, free the valid one.
    on_initialized_event->Close();
    parent_process->Close();
    *main_thread_id = 0;
    return false;
  }

  return true;
}
