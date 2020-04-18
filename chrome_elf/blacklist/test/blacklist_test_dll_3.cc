// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

const wchar_t kDll3Beacon[] = L"{9E056AEC-169E-400c-B2D0-5A07E3ACE2EB}";

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    ::SetEnvironmentVariable(kDll3Beacon, L"1");
  }
  return TRUE;
}
