// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/blacklist/blacklist.h"

#include <windows.h>

#include "chrome/install_static/install_util.h"
#include "chrome_elf/nt_registry/nt_registry.h"

namespace {

void GetIpcOverrides() {
  DWORD buffer_size = ::GetEnvironmentVariableW(L"hkcu_override", nullptr, 0);
  if (buffer_size > 0) {
    std::wstring content(buffer_size, L'\0');
    buffer_size =
        ::GetEnvironmentVariableW(L"hkcu_override", &content[0], buffer_size);
    if (buffer_size)
      nt::SetTestingOverride(nt::HKCU, content);
  }

  buffer_size = ::GetEnvironmentVariableW(L"hklm_override", nullptr, 0);
  if (buffer_size > 0) {
    std::wstring content(buffer_size, L'\0');
    buffer_size =
        ::GetEnvironmentVariableW(L"hklm_override", &content[0], buffer_size);
    if (buffer_size)
      nt::SetTestingOverride(nt::HKLM, content);
  }

  return;
}

}  // namespace

extern "C" __declspec(dllexport) void InitTestDll() {
  // Make sure we've got the latest registry overrides.
  GetIpcOverrides();
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    GetIpcOverrides();
    install_static::InitializeProcessType();
    blacklist::Initialize(true);  // force always on, no beacon
  }

  return TRUE;
}
