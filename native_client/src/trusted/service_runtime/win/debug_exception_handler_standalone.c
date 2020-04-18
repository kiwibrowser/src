/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/win/debug_exception_handler.h"

#include <string.h>

#include <windows.h>

#include "native_client/src/include/portability_io.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"


int NaClDebugExceptionHandlerStandaloneAttach(const void *info,
                                              size_t info_size) {
  NaClHandle sockets[2];
  STARTUPINFOA startup_info;
  PROCESS_INFORMATION process_information;
  char sel_ldr_path[PATH_MAX];
  /*
   * 'args' only needs to be long enough to store the format string
   * below plus 2*sizeof(struct StartupInfo).  PATH_MAX is plenty.
   */
  char args[PATH_MAX];
  char *dest;
  size_t length;
  size_t index;
  char buffer[1];
  DWORD bytes_read;

  if (NaClSocketPair(sockets) != 0) {
    NaClLog(LOG_ERROR, "NaClDebugExceptionHandlerStandaloneAttach: "
            "Failed to create socket pair\n");
    return 0;
  }
  if (!SetHandleInformation(sockets[1], HANDLE_FLAG_INHERIT,
                            HANDLE_FLAG_INHERIT)) {
    NaClLog(LOG_ERROR, "NaClDebugExceptionHandlerStandaloneAttach: "
            "SetHandleInformation() failed\n");
    return 0;
  }
  memset(&startup_info, 0, sizeof(startup_info));
  startup_info.cb = sizeof(startup_info);
  memset(&process_information, 0, sizeof(process_information));

  /*
   * Note that the child process does not use argv[0] so we do not
   * need to provide the executable filename in that argument; it is
   * tricky to get the quoting right on Windows.
   */
  length = SNPRINTF(args, sizeof(args),
                    "argv0 --debug-exception-handler %i %i ",
                    GetCurrentProcessId(), (int)(intptr_t) sockets[1]);
  CHECK(length > 0 && length < sizeof(args));
  dest = args + length;
  for (index = 0; index < info_size; index++) {
    CHECK(dest + 3 < args + sizeof(args));
    CHECK(SNPRINTF(dest, 2, "%02x", ((uint8_t *) info)[index]) == 2);
    dest += 2;
  }
  *dest = 0;

  length = GetModuleFileNameA(NULL, sel_ldr_path, sizeof(sel_ldr_path));
  if (length == 0 || length >= sizeof(sel_ldr_path) - 1) {
    NaClLog(LOG_ERROR, "NaClDebugExceptionHandlerStandaloneAttach: "
            "Failed to get the executable's filename\n");
    return 0;
  }

  if (!CreateProcessA(sel_ldr_path, args, NULL, NULL,
                      /* bInheritHandles= */ TRUE, 0,
                      NULL, NULL, &startup_info, &process_information)) {
    NaClLog(LOG_ERROR, "NaClDebugExceptionHandlerStandaloneAttach: "
            "Failed to create debug exception handler process\n");
    return 0;
  }
  CloseHandle(sockets[1]);
  CloseHandle(process_information.hThread);
  CloseHandle(process_information.hProcess);

  /*
   * We wait for a message from the child process to indicate that it
   * has successfully attached to us as a debugger.
   */
  if (!ReadFile(sockets[0], buffer, sizeof(buffer), &bytes_read, NULL)) {
    NaClLog(LOG_ERROR, "NaClDebugExceptionHandlerStandaloneAttach: "
            "Failed to read message\n");
    return 0;
  }
  if (bytes_read != 1 || buffer[0] != 'k') {
    NaClLog(LOG_ERROR, "NaClDebugExceptionHandlerStandaloneAttach: "
            "Did not receive expected reply message\n");
    return 0;
  }
  CloseHandle(sockets[0]);
  return 1;
}

static void DecodeHexString(uint8_t *dest, char *src, size_t size) {
  size_t index;
  for (index = 0; index < size; index++) {
    char *rest;
    char tmp[3];
    tmp[0] = src[index * 2];
    tmp[1] = src[index * 2 + 1];
    tmp[2] = 0;
    dest[index] = (uint8_t) strtol(tmp, &rest, 16);
    if (rest != tmp + 2) {
      NaClLog(LOG_FATAL, "DecodeHexString: Bad input: %s\n", src);
    }
  }
}

static int NaClDebugExceptionHandlerStandaloneMain(int argc, char **argv) {
  int target_pid;
  NaClHandle socket;
  char *rest1;
  char *rest2;
  void *info;
  size_t info_size;
  HANDLE process_handle;
  DWORD written;

  if (argc != 3) {
    NaClLog(LOG_FATAL, "NaClDebugExceptionHandlerStandaloneMain: "
            "Expected 3 arguments: target_pid, socket, data\n");
  }

  target_pid = strtol(argv[0], &rest1, 0);
  socket = (NaClHandle)(uintptr_t) strtol(argv[1], &rest2, 0);
  if (*rest1 != '\0' || *rest2 != '\0') {
    NaClLog(LOG_FATAL, "NaClDebugExceptionHandlerStandaloneMain: "
            "Bad string argument\n");
  }

  info_size = strlen(argv[2]);
  if (info_size % 2 != 0) {
    NaClLog(LOG_FATAL, "NaClDebugExceptionHandlerStandaloneMain: "
            "Odd string length\n");
  }
  info_size /= 2;
  info = malloc(info_size);
  if (info == NULL) {
    NaClLog(LOG_FATAL, "NaClDebugExceptionHandlerStandaloneMain: "
            "malloc() failed\n");
  }
  DecodeHexString((uint8_t *) info, argv[2], info_size);

  if (!DebugActiveProcess(target_pid)) {
    NaClLog(LOG_FATAL, "NaClDebugExceptionHandlerStandaloneMain: "
            "Failed to attach with DebugActiveProcess()\n");
  }

  process_handle = OpenProcess(PROCESS_QUERY_INFORMATION |
                               PROCESS_SUSPEND_RESUME |
                               PROCESS_TERMINATE |
                               PROCESS_VM_OPERATION |
                               PROCESS_VM_READ |
                               PROCESS_VM_WRITE |
                               PROCESS_DUP_HANDLE |
                               SYNCHRONIZE,
                               /* bInheritHandle= */ FALSE,
                               target_pid);
  if (process_handle == NULL) {
    NaClLog(LOG_FATAL, "NaClDebugExceptionHandlerStandaloneMain: "
            "Failed to get process handle with OpenProcess()\n");
  }

  /* Send message to indicate that we attached to the process successfully. */
  if (!WriteFile(socket, "k", 1, &written, NULL) || written != 1) {
    NaClLog(LOG_FATAL, "NaClDebugExceptionHandlerStandaloneMain: "
            "Failed to send reply\n");
  }

  NaClDebugExceptionHandlerRun(process_handle, info, info_size);
  return 0;
}

void NaClDebugExceptionHandlerStandaloneHandleArgs(int argc, char **argv) {
  if (argc >= 2 && strcmp(argv[1], "--debug-exception-handler") == 0) {
    exit(NaClDebugExceptionHandlerStandaloneMain(argc - 2, argv + 2));
  }
}
