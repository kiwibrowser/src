/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_HOST_DESC_TYPES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_HOST_DESC_TYPES_H_

#include <windows.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/shared/platform/nacl_sync.h"

EXTERN_C_BEGIN

struct NaClHostDesc {
  int d;  /* POSIX layer, not Windows HANDLEs */

  /*
   * Stores open's flag argument bits.
   *
   * This is needed for correctly specifying flProtect argument in
   * CreateFileMapping, and for permissions in MapViewOfFileEx.
   *
   * friend int NaClDescIoDescExternalizeSize(...);
   * friend int NaClDescIoDescExternalize(...);
   * friend int NaClDescIoInternalize(...);
   *
   * Translation: NaClDescIoDesc's externalization/internalization
   * interface functions are friend functions that look inside the
   * NaClHostDesc implementation.  Do not make changes here without
   * also looking there.
   */
  int flags;

  /*
   * For regular files -- seekable -- we need to protect the implicit
   * file position pointer against temporary changes due to the
   * pread/pwrite implementation.  Contrary to documentation, ReadFile
   * on a synchronous file handle with a non-NULL LPOVERLAPPED
   * argument updates the implicit, shared file position instead of
   * the file position in the OVERLAPPED structure, so messy locking
   * and seeking is required to fix things up.  However, we may
   * inherit console descriptors for standard output and standard
   * error, and in such cases, attempts to do byte range locking
   * and/or seeking will fail.  When we import descriptors in
   * NaClHostDescCtorIntern, we set protect_filepos only for regular
   * files and directories.
   */
  int protect_filepos;

  /*
   * Mutex acquisition of mu ensures accesses to flProtect below see
   * consistent values.
   */
  struct NaClFastMutex mu;

  /* flMappingProtect fallback used; zero when unknown or not needed */
  DWORD flMappingProtect;
};

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_HOST_DESC_TYPES_H_ */
