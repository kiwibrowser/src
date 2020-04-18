// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/sandbox_init.h"

#include <string>

#include "base/base_switches.h"
#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "base/win/scoped_process_information.h"
#include "content/common/content_switches_internal.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/sandbox_init.h"
#include "content/public/common/sandboxed_process_launcher_delegate.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_types.h"
#include "services/service_manager/sandbox/sandbox.h"
#include "services/service_manager/sandbox/win/sandbox_win.h"

namespace content {

bool InitializeSandbox(service_manager::SandboxType sandbox_type,
                       sandbox::SandboxInterfaceInfo* sandbox_info) {
  return service_manager::Sandbox::Initialize(sandbox_type, sandbox_info);
}

sandbox::ResultCode StartSandboxedProcess(
    SandboxedProcessLauncherDelegate* delegate,
    base::CommandLine* child_command_line,
    const base::HandlesToInheritVector& handles_to_inherit,
    base::Process* process) {
  std::string type_str =
      child_command_line->GetSwitchValueASCII(switches::kProcessType);
  TRACE_EVENT1("startup", "StartProcessWithAccess", "type", type_str);

  // Updates the command line arguments with debug-related flags. If debug
  // flags have been used with this process, they will be filtered and added
  // to child_command_line as needed.
  const base::CommandLine* current_command_line =
      base::CommandLine::ForCurrentProcess();
  if (current_command_line->HasSwitch(switches::kWaitForDebuggerChildren)) {
    std::string value = current_command_line->GetSwitchValueASCII(
        switches::kWaitForDebuggerChildren);
    child_command_line->AppendSwitchASCII(switches::kWaitForDebuggerChildren,
                                          value);
    if (value.empty() || value == type_str)
      child_command_line->AppendSwitch(switches::kWaitForDebugger);
  }

  return service_manager::SandboxWin::StartSandboxedProcess(
      child_command_line, type_str, handles_to_inherit, delegate, process);
}

}  // namespace content
