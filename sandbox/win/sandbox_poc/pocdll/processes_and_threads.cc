// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>
#include <Tlhelp32.h>
#include "sandbox/win/sandbox_poc/pocdll/exports.h"
#include "sandbox/win/sandbox_poc/pocdll/utils.h"

// This file contains the tests used to verify the security of threads and
// processes.

void POCDLL_API TestProcesses(HANDLE log) {
  HandleToFile handle2file;
  FILE *output = handle2file.Translate(log, "w");

  HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
  if (INVALID_HANDLE_VALUE == snapshot) {
    fprintf(output, "[BLOCKED] Cannot list all processes on the system. "
                    "Error %ld\r\n", ::GetLastError());
    return;
  }

  PROCESSENTRY32 process_entry = {0};
  process_entry.dwSize = sizeof(PROCESSENTRY32);

  BOOL result = ::Process32First(snapshot, &process_entry);

  while (result) {
    HANDLE process = ::OpenProcess(PROCESS_VM_READ,
                                   FALSE,  // Do not inherit handle.
                                   process_entry.th32ProcessID);
    if (NULL == process) {
      fprintf(output, "[BLOCKED] Found process %S:%ld but cannot open it. "
                      "Error %ld\r\n",
                      process_entry.szExeFile,
                      process_entry.th32ProcessID,
                      ::GetLastError());
    } else {
      fprintf(output, "[GRANTED] Found process %S:%ld and open succeeded.\r\n",
                      process_entry.szExeFile, process_entry.th32ProcessID);
      ::CloseHandle(process);
    }

    result = ::Process32Next(snapshot, &process_entry);
  }

  DWORD err_code = ::GetLastError();
  if (ERROR_NO_MORE_FILES != err_code) {
    fprintf(output, "[ERROR] Error %ld while looking at the processes on "
                    "the system\r\n", err_code);
  }

  ::CloseHandle(snapshot);
}

void POCDLL_API TestThreads(HANDLE log) {
  HandleToFile handle2file;
  FILE *output = handle2file.Translate(log, "w");

  HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, NULL);
  if (INVALID_HANDLE_VALUE == snapshot) {
    fprintf(output, "[BLOCKED] Cannot list all threads on the system. "
                    "Error %ld\r\n", ::GetLastError());
    return;
  }

  THREADENTRY32 thread_entry = {0};
  thread_entry.dwSize = sizeof(THREADENTRY32);

  BOOL result = ::Thread32First(snapshot, &thread_entry);
  int nb_success = 0;
  int nb_failure = 0;

  while (result) {
    HANDLE thread = ::OpenThread(THREAD_QUERY_INFORMATION,
                                 FALSE,  // Do not inherit handles.
                                 thread_entry.th32ThreadID);
    if (NULL == thread) {
      nb_failure++;
    } else {
      nb_success++;
      fprintf(output, "[GRANTED] Found thread %ld:%ld and able to open it.\r\n",
                      thread_entry.th32OwnerProcessID,
                      thread_entry.th32ThreadID);
      ::CloseHandle(thread);
    }

    result = Thread32Next(snapshot, &thread_entry);
  }

  DWORD err_code = ::GetLastError();
  if (ERROR_NO_MORE_FILES != err_code) {
    fprintf(output, "[ERROR] Error %ld while looking at the processes on "
                    "the system\r\n", err_code);
  }

  fprintf(output, "[INFO] Found %d threads. Able to open %d of them\r\n",
          nb_success + nb_failure, nb_success);

  ::CloseHandle(snapshot);
}
