// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/win/sandbox_poc/pocdll/exports.h"
#include "sandbox/win/sandbox_poc/pocdll/utils.h"

// This file contains the tests used to verify the security of the file system.

// Tries to open a file and outputs the result.
// "path" can contain environment variables.
// "output" is the stream for the logging.
void TryOpenFile(const wchar_t *path, FILE *output) {
  wchar_t path_expanded[MAX_PATH] = {0};
  DWORD size = ::ExpandEnvironmentStrings(path, path_expanded, MAX_PATH - 1);
  if (!size) {
    fprintf(output, "[ERROR] Cannot expand \"%S\". Error %ld.\r\n", path,
            ::GetLastError());
  }

  HANDLE file;
  file = ::CreateFile(path_expanded,
                      GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      NULL,  // No security attributes
                      OPEN_EXISTING,
                      FILE_FLAG_BACKUP_SEMANTICS,
                      NULL);  // No template

  if (file && INVALID_HANDLE_VALUE != file) {
    fprintf(output, "[GRANTED] Opening file \"%S\". Handle 0x%p\r\n", path,
            file);
    ::CloseHandle(file);
  } else {
    fprintf(output, "[BLOCKED] Opening file \"%S\". Error %ld.\r\n", path,
            ::GetLastError());
  }
}

void POCDLL_API TestFileSystem(HANDLE log) {
  HandleToFile handle2file;
  FILE *output = handle2file.Translate(log, "w");

  TryOpenFile(L"%SystemDrive%", output);
  TryOpenFile(L"%SystemRoot%", output);
  TryOpenFile(L"%ProgramFiles%", output);
  TryOpenFile(L"%SystemRoot%\\System32", output);
  TryOpenFile(L"%SystemRoot%\\explorer.exe", output);
  TryOpenFile(L"%SystemRoot%\\Cursors\\arrow_i.cur", output);
  TryOpenFile(L"%AllUsersProfile%", output);
  TryOpenFile(L"%UserProfile%", output);
  TryOpenFile(L"%Temp%", output);
  TryOpenFile(L"%AppData%", output);
}
