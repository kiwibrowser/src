// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/win/sandbox_poc/pocdll/exports.h"
#include "sandbox/win/sandbox_poc/pocdll/utils.h"

// This file contains the tests used to verify the security of the registry.

// Tries to open the key hive\path and outputs the result.
// "output" is the stream used for logging.
void TryOpenKey(const HKEY hive,
                const wchar_t* hive_name,
                const wchar_t* path,
                FILE* output) {
  HKEY key;
  LONG err_code = ::RegOpenKeyEx(hive,
                                 path,
                                 0,  // Reserved, must be 0.
                                 MAXIMUM_ALLOWED,
                                 &key);
  if (ERROR_SUCCESS == err_code) {
    fprintf(output,
            "[GRANTED] Opening key \"%S\\%S\". Handle 0x%p\r\n",
            hive_name,
            path,
            key);
    ::RegCloseKey(key);
  } else {
    fprintf(output,
            "[BLOCKED] Opening key \"%S\\%S\". Error %ld\r\n",
            hive_name,
            path,
            err_code);
  }
}

void POCDLL_API TestRegistry(HANDLE log) {
  HandleToFile handle2file;
  FILE *output = handle2file.Translate(log, "w");

  TryOpenKey(HKEY_LOCAL_MACHINE, L"HKEY_LOCAL_MACHINE", NULL, output);
  TryOpenKey(HKEY_CURRENT_USER, L"HKEY_CURRENT_USER", NULL, output);
  TryOpenKey(HKEY_USERS, L"HKEY_USERS", NULL, output);
  TryOpenKey(HKEY_LOCAL_MACHINE,
             L"HKEY_LOCAL_MACHINE",
             L"Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon",
             output);
}
