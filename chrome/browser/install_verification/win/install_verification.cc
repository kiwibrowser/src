// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/install_verification/win/install_verification.h"

#include <stddef.h>
#include <windows.h>

#include <set>
#include <vector>

#include "base/files/file_path.h"
#include "base/metrics/histogram_functions.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/install_verification/win/loaded_module_verification.h"
#include "chrome/browser/install_verification/win/module_ids.h"
#include "chrome/browser/install_verification/win/module_info.h"
#include "chrome/browser/install_verification/win/module_verification_common.h"
#include "components/variations/hashing.h"

namespace {

void ReportModuleMatch(size_t module_id) {
  base::UmaHistogramSparse("InstallVerifier.ModuleMatch", module_id);
}

base::FilePath GetExeFilePathForProcess(const base::Process& process) {
  wchar_t exe_name[MAX_PATH];
  DWORD exe_name_len = arraysize(exe_name);
  // Note: requesting the Win32 path format.
  if (::QueryFullProcessImageName(process.Handle(), 0, exe_name,
                                  &exe_name_len) == 0) {
    DPLOG(ERROR) << "Failed to get executable name for process";
    return base::FilePath();
  }

  // QueryFullProcessImageName's documentation does not specify behavior when
  // the buffer is too small, but we know that GetModuleFileNameEx succeeds and
  // truncates the returned name in such a case. Given that paths of arbitrary
  // length may exist, the conservative approach is to reject names when
  // the returned length is that of the buffer.
  if (exe_name_len > 0 && exe_name_len < arraysize(exe_name))
    return base::FilePath(exe_name);

  return base::FilePath();
}

void ReportParentProcessName() {
  base::ProcessId ppid =
      base::GetParentProcessId(base::GetCurrentProcessHandle());

  base::Process process(
      base::Process::OpenWithAccess(ppid, PROCESS_QUERY_LIMITED_INFORMATION));

  uint32_t hash = 0U;

  if (process.IsValid()) {
    base::FilePath path(GetExeFilePathForProcess(process));

    if (!path.empty()) {
      std::string ascii_path(base::SysWideToUTF8(path.BaseName().value()));
      DCHECK(base::IsStringASCII(ascii_path));
      hash = variations::HashName(base::ToLowerASCII(ascii_path));
    }
  }

  base::UmaHistogramSparse("Windows.ParentProcessNameHash", hash);
}

}  // namespace

void VerifyInstallation() {
  ReportParentProcessName();
  ModuleIDs module_ids;
  LoadModuleIDs(&module_ids);
  std::set<ModuleInfo> loaded_modules;
  if (GetLoadedModules(&loaded_modules)) {
    VerifyLoadedModules(loaded_modules, module_ids, &ReportModuleMatch);
  }
}
