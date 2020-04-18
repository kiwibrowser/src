// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// Arg1: pointer to a buffer of size MAX_PATH + 1.
bool GetPathOnDisk(wchar_t* buffer) {
  if (buffer == nullptr)
    return false;

  if (::GetModuleFileNameW((HINSTANCE)&__ImageBase, buffer, MAX_PATH) == 0)
    return false;

  return true;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
  return TRUE;
}
