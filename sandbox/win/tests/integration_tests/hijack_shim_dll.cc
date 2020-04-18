// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include "sandbox/win/tests/common/controller.h"
#include "sandbox/win/tests/integration_tests/hijack_shim_dll.h"

// Function definition from hijack dll.
bool GetPathOnDisk(wchar_t* buffer);

// This shim implicitly links in the GetPathOnDisk function from the test
// hijack DLL.  When this DLL is loaded, the loader resolves the
// import using standard search path.  (The mitigation being tested affects the
// standard search path.)  This test then calls the exported function to
// determine which DLL on disk was loaded.
//
// Arg1: "true" or "false", if the DLL path should be in system32.
int CheckHijackResult(bool expect_system) {
  wchar_t dll_path[MAX_PATH + 1] = {};

  // This should always succeed.
  if (!GetPathOnDisk(dll_path))
    return sandbox::SBOX_TEST_NOT_FOUND;

  // Make sure the path is all one case.
  for (size_t i = 0; dll_path[i]; i++)
    dll_path[i] = ::towlower(dll_path[i]);

  if (::wcsstr(dll_path, L"system32") == nullptr) {
    // Not in system32.
    return (expect_system) ? sandbox::SBOX_TEST_FAILED
                           : sandbox::SBOX_TEST_SUCCEEDED;
  }

  return (!expect_system) ? sandbox::SBOX_TEST_FAILED
                          : sandbox::SBOX_TEST_SUCCEEDED;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
  return TRUE;
}
