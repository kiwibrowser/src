// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/network_sandbox_win.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"

// NOTE: changes to this code need to be reviewed by the security team.

namespace network {

namespace {

bool EnumHardDrives(std::vector<std::wstring>* drive_paths) {
  drive_paths->clear();
  wchar_t volume_name[MAX_PATH];

  // INVALID_HANDLE_VALUE alert!
  HANDLE volume = ::FindFirstVolumeW(volume_name, ARRAYSIZE(volume_name));
  if (volume == INVALID_HANDLE_VALUE) {
    // This should never happen.  Unexpected failure, no volumes found.
    NOTREACHED();
    return false;
  }

  wchar_t device_name[MAX_PATH];
  do {
    // Example volume name returned:
    // "\\?\Volume{4c1b02c1-d990-11dc-99ae-806e6f6e6963}\"
    size_t length = ::wcsnlen(volume_name, ARRAYSIZE(volume_name));
    if (length < 5 || volume_name[0] != L'\\' || volume_name[1] != L'\\' ||
        volume_name[2] != L'?' || volume_name[3] != L'\\' ||
        volume_name[length - 1] != L'\\') {
      // Bad path returned.  Very unexpected.
      continue;
    }

    // QueryDosDevice doesn't take a trailing backslash.
    volume_name[length - 1] = L'\0';
    DWORD device_name_length =
        ::QueryDosDeviceW(&volume_name[4], device_name, ARRAYSIZE(device_name));
    volume_name[length - 1] = L'\\';
    if (!device_name_length)
      continue;

    // Example device name returned: "\Device\HarddiskVolume2"
    // Ignore any volumes that aren't hard disks (e.g. CdRom, etc).
    if (::wcsncmp(device_name, L"\\Device\\Harddisk",
                  ::wcslen(L"\\Device\\Harddisk"))) {
      continue;
    }

    // Now get the path(s) for the given hard disk volume.
    bool success = false;
    DWORD char_count = MAX_PATH;
    std::vector<wchar_t> path_names_buffer;
    while (!success) {
      path_names_buffer.resize(char_count * sizeof(wchar_t));
      success = ::GetVolumePathNamesForVolumeNameW(
          volume_name, path_names_buffer.data(), char_count, &char_count);
      if (!success && ::GetLastError() != ERROR_MORE_DATA)
        break;
    }
    if (!success)
      continue;

    // |buffer| is filled with a list of strings, separated by null
    // characters.  Double null ends list.
    // Note: usually the very first string will be the drive letter sought,
    //       and is often the only string in the list.  Some volumes have
    //       no drive letter assigned at all - not interested here.
    size_t volume_path_index = 0;
    size_t path_length = ::wcslen(&path_names_buffer[volume_path_index]);
    while (path_length > 0) {
      // Only collect drive letters, not mounted folder paths.
      // Do dumb check instead of calls to GetFileAttributes().
      if (path_length == 3 &&
          path_names_buffer[volume_path_index + 1] == L':' &&
          path_names_buffer[volume_path_index + 2] == L'\\') {
        drive_paths->emplace_back(&path_names_buffer[volume_path_index]);
      }
      volume_path_index += path_length + 1;
      path_length = ::wcslen(&path_names_buffer[volume_path_index]);
    }
  } while (::FindNextVolumeW(volume, volume_name, ARRAYSIZE(volume_name)));

  ::FindVolumeClose(volume);

  return true;
}

}  // namespace

//------------------------------------------------------------------------------
// Public network service sandbox configuration extension functions.
//------------------------------------------------------------------------------

/*
  Default policy:

  lockdown_level_(sandbox::USER_LOCKDOWN),
  initial_level_(sandbox::USER_RESTRICTED_SAME_ACCESS),

  job_level_(sandbox::JOB_LOCKDOWN),

  integrity_level_(sandbox::INTEGRITY_LEVEL_LOW),
  delayed_integrity_level_(sandbox::INTEGRITY_LEVEL_UNTRUSTED),
*/

/*
  Network cache: %UserDataDir%\\Default\\*
  (E.g. c:\Temp\UserDataDir\Default\Cache to
   c:\Temp\UserDataDir\Default\old_Cache_000)

  DNS config registry access:
  ---------------------------
  HKEY_LOCAL_MACHINE
  kTcpipPath = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";
  kTcpip6Path = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters";
  kDnscachePath = L"SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters";
  kPolicyPath = L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient";
  kPrimaryDnsSuffixPath = L"SOFTWARE\\Policies\\Microsoft\\System\\DNSClient";
  kNRPTPath = L"SOFTWARE\\Policies\\Microsoft\\Windows
  NT\\DNSClient\\DnsPolicyConfig";

  Priviliged API GetAdaptorsAddresses()
*/
bool NetworkPreSpawnTarget(sandbox::TargetPolicy* policy) {
  // Token Level (affects system network APIs)
  policy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                        sandbox::USER_LIMITED);

  // File
  std::vector<std::wstring> drives;
  if (!EnumHardDrives(&drives))
    return false;

  for (auto path : drives) {
    path.append(L"*");
    if (policy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                        sandbox::TargetPolicy::FILES_ALLOW_ANY,
                        path.c_str()) != sandbox::SBOX_ALL_OK) {
      return false;
    }
  }

  // Registry
  if (policy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY,
                      sandbox::TargetPolicy::REG_ALLOW_READONLY,
                      L"HKEY_LOCAL_MACHINE\\*") != sandbox::SBOX_ALL_OK ||
      policy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY,
                      sandbox::TargetPolicy::REG_ALLOW_READONLY,
                      L"HKEY_CURRENT_USER\\*") != sandbox::SBOX_ALL_OK) {
    return false;
  }

  return true;
}

}  // namespace network
