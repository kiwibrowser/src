/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Windows error code to errno translation.
 */

#include <errno.h>
#include <WinError.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/win/xlate_system_error.h"

#include "native_client/src/trusted/service_runtime/include/sys/errno.h"

/*
 * we attempt to map error codes that are relevant for the following
 * system calls:
 *
 * - CreateFile
 * - CreateFileMapping
 * - MapViewOfFileEx
 * - UnmapViewOfFile
 * - ReadFile
 * - WriteFile
 * - CloseHandle
 *
 * because the msdn pages do not specify what error codes might be
 * returned by any of the system calls, the preimage set is likely to
 * be incomplete.
 */

int NaClXlateSystemError(int sys_error_code) {
#define E(c, err) \
    case c: do { \
      NaClLog(2, "NaClXlateSystemError: windows error code %d (%s) -> %d\n", \
              c, #c, err); \
      return err; } while (0)

  switch (sys_error_code) {
    E(ERROR_SUCCESS, 0);
    E(ERROR_FILE_NOT_FOUND, NACL_ABI_ENOENT);
    E(ERROR_PATH_NOT_FOUND, NACL_ABI_ENOENT);
    E(ERROR_TOO_MANY_OPEN_FILES, NACL_ABI_EMFILE);
      /*
       * TODO(bsy) distinguishability?
       *
       * note EPERM might be reasonable; that is typically used for a
       * minor request parmeter asking for something that is not
       * allowed, e.g. mmap with PROT_EXEC but filesystem is mounted
       * no-exec, rather than simple access denial.
       */
    E(ERROR_ACCESS_DENIED, NACL_ABI_EACCES);
    E(ERROR_INVALID_HANDLE, NACL_ABI_EBADF);
    E(ERROR_NOT_ENOUGH_MEMORY, NACL_ABI_ENOMEM);
    E(ERROR_OUTOFMEMORY, NACL_ABI_ENOMEM);
    E(ERROR_INVALID_DRIVE, NACL_ABI_EACCES);
    E(ERROR_NOT_SAME_DEVICE, NACL_ABI_EXDEV);
    E(ERROR_NO_MORE_FILES, NACL_ABI_ENFILE);
    E(ERROR_WRITE_PROTECT, NACL_ABI_EBADF);
    E(ERROR_OPEN_FAILED, NACL_ABI_EIO);
    E(ERROR_INVALID_USER_BUFFER, NACL_ABI_EFAULT);
    E(ERROR_NOT_ENOUGH_QUOTA, NACL_ABI_EDQUOT);
    E(ERROR_INVALID_PARAMETER, NACL_ABI_EINVAL);
    E(ERROR_INVALID_ADDRESS, NACL_ABI_EFAULT);
    E(ERROR_PIPE_LISTENING, NACL_ABI_EAGAIN);
    E(ERROR_BROKEN_PIPE, NACL_ABI_EPIPE);
    E(ERROR_NO_DATA, NACL_ABI_EPIPE);
    default:
      NaClLog(LOG_ERROR,
              ("NaClXlateSystemError: UNEXPECTED ERROR %d (0x%x),"
               " returning EINVAL\n"),
              sys_error_code, sys_error_code);
      return NACL_ABI_EINVAL;  /* as a default? */
  }
}
