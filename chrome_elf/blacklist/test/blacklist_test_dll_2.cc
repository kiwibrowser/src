// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

const wchar_t kDll2Beacon[] = L"{F70A0100-2889-4629-9B44-610FE5C73231}";

extern "C" {
// Have a dummy export so that the module gets an export table entry.
void DummyExport() {}
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    ::SetEnvironmentVariable(kDll2Beacon, L"1");
  }
  return TRUE;
}
