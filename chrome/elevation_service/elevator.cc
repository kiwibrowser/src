// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/elevation_service/elevator.h"

#include "base/files/file_util.h"
#include "base/process/process.h"
#include "base/win/registry.h"
#include "base/win/scoped_handle.h"
#include "base/win/scoped_process_information.h"
#include "chrome/install_static/install_util.h"

namespace elevation_service {

namespace {

// This registry key contains the commands that the Elevator can run.
constexpr base::char16 kCommandsRegistryKey[] = L"Commands";
constexpr base::char16 kPathAndName[] = L"PathAndName";
constexpr base::char16 kArguments[] = L"Arguments";

HRESULT HRESULTFromLastError() {
  const DWORD error_code = ::GetLastError();
  return (error_code != ERROR_SUCCESS) ? HRESULT_FROM_WIN32(error_code) :
                                         E_FAIL;
}

// TODO(ganesh): we need to have the installer write into the proper key for the
// bitness (i.e., no KEY_WOW64 specifier) and make sure it deletes a stale value
// in the opposite bitness hive to cover cases where a 32-bit install is updated
// to 64-bit.
HRESULT GetCommandToLaunch(base::StringPiece16 cmd_id,
                           base::FilePath* path_and_name,
                           base::string16* args) {
  base::win::RegKey key;

  auto registry_key_path =
      install_static::GetRegistryPath() + L"\\" + kCommandsRegistryKey + L"\\";
  cmd_id.AppendToString(&registry_key_path);

  LONG result =
      key.Open(HKEY_LOCAL_MACHINE, registry_key_path.c_str(), KEY_QUERY_VALUE);
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);

  base::string16 string_path_and_name;
  result = key.ReadValue(kPathAndName, &string_path_and_name);
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);

  *path_and_name = base::FilePath(string_path_and_name);
  if (!path_and_name->IsAbsolute())
    return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

  result = key.ReadValue(kArguments, args);
  if (result != ERROR_FILE_NOT_FOUND)
    return HRESULT_FROM_WIN32(result);

  return S_OK;
}

HRESULT StartProcess(const base::FilePath& path_and_name,
                     const base::string16& args,
                     PROCESS_INFORMATION* pi) {
  DCHECK(pi);

  STARTUPINFO si = {sizeof(si)};
  si.dwFlags = STARTF_FORCEOFFFEEDBACK;

  // We have had problems in the past with subprocs preventing chrome from being
  // uninstalled because their cwd was within Google\Chrome\Application\. Hence,
  // making the starting directory the Temp directory. This should be
  // \\Windows\\Temp for the Service.
  const base::char16* starting_directory = nullptr;
  base::FilePath temp_dir;
  if (base::GetTempDir(&temp_dir))
    starting_directory = temp_dir.value().c_str();

  base::string16 command_line = L"\"" + path_and_name.value() + L"\" " + args;
  command_line.push_back(L'\0');
  bool success =
      ::CreateProcess(path_and_name.value().c_str(),  // Application path/name
                      &command_line[0],    // Command line
                      nullptr,             // Process handle not inheritable
                      nullptr,             // Thread handle not inheritable
                      false,               // Set handle inheritance to FALSE
                      0,                   // No creation flags
                      nullptr,             // Use parent's environment block
                      starting_directory,  // Use TMP
                      &si,                 // Pointer to STARTUPINFO struct
                      pi);  // Pointer to PROCESS_INFORMATION structure

  if (!success) {
    HRESULT hr = HRESULTFromLastError();
    LOG(ERROR) << "StartProcess::CreateProcess failed; hr: " << hr
               << ", command_line: " << path_and_name << ", args:" << args;
    return hr;
  }

  return S_OK;
}

HRESULT OpenCallingProcess(DWORD proc_id, base::Process* process) {
  DCHECK(process);

  HRESULT hr = ::CoImpersonateClient();
  if (FAILED(hr))
    return hr;

  *process = base::Process::OpenWithAccess(proc_id, PROCESS_DUP_HANDLE);
  hr = process->IsValid() ? S_OK : HRESULTFromLastError();

  ::CoRevertToSelf();
  return hr;
}

HRESULT LaunchCmd(const base::FilePath& path_and_name,
                  const base::string16& args,
                  const base::Process& calling_process,
                  base::win::ScopedHandle* proc_handle) {
  DCHECK(!path_and_name.empty());
  DCHECK(proc_handle);

  PROCESS_INFORMATION process_info = {};
  HRESULT hr = StartProcess(path_and_name, args, &process_info);
  if (FAILED(hr)) {
    LOG(ERROR) << "failed to launch app: " << path_and_name << ", args:" << args
               << "; hr: " << hr;
    return hr;
  }
  base::win::ScopedProcessInformation pi(process_info);

  // DuplicateHandle call will close the source handle regardless of any error
  // status returned.
  DCHECK(pi.process_handle());

  HANDLE duplicate_proc_handle = nullptr;

  constexpr DWORD desired_access =
      PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE;
  bool res = ::DuplicateHandle(
                 ::GetCurrentProcess(),     // Current process.
                 pi.TakeProcessHandle(),    // Process handle to duplicate.
                 calling_process.Handle(),  // Process receiving the handle.
                 &duplicate_proc_handle,    // Duplicated handle.
                 desired_access,  // Access requested for the new handle.
                 FALSE,           // Don't inherit the new handle.
                 DUPLICATE_CLOSE_SOURCE) != 0;  // Closes the source handle.
  if (!res) {
    hr = HRESULTFromLastError();
    LOG(ERROR) << "failed to duplicate the handle; hr: " << hr;
    return hr;
  }

  proc_handle->Set(duplicate_proc_handle);

  return S_OK;
}

}  // namespace

IFACEMETHODIMP Elevator::LaunchCommand(const WCHAR* cmd_id,
                                       DWORD caller_proc_id,
                                       ULONG_PTR* proc_handle) {
  DCHECK(cmd_id);
  DCHECK(proc_handle);

  const base::StringPiece16 id = cmd_id;
  if (id.empty() || id.find(L'\\') != base::StringPiece16::npos || !proc_handle)
    return E_INVALIDARG;

  base::Process calling_process;
  HRESULT hr = OpenCallingProcess(caller_proc_id, &calling_process);
  if (FAILED(hr)) {
    LOG(ERROR) << "failed to open caller's handle; hr: " << hr
               << ", cmd_id: " << cmd_id << ", pid: " << caller_proc_id;
    return hr;
  }

  base::FilePath path_and_name;
  base::string16 args;
  hr = GetCommandToLaunch(id, &path_and_name, &args);
  if (FAILED(hr))
    return hr;

  base::win::ScopedHandle scoped_proc_handle;
  hr = LaunchCmd(path_and_name, args, calling_process, &scoped_proc_handle);
  if (FAILED(hr))
    return hr;

  *proc_handle = reinterpret_cast<ULONG_PTR>(scoped_proc_handle.Take());
  return hr;
}

Elevator::~Elevator() = default;

}  // namespace elevation_service
