/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Directory descriptor abstraction.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_HOST_DIR_TYPES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_HOST_DIR_TYPES_H_

#include <windows.h>

#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

struct NaClHostDir {
  struct NaClMutex  mu;
  /*
   * Windows HANDLEs are used by FindFirst/FindNext/FindClose
   */
  HANDLE            handle;
  /*
   * The data found from the previous call is in this slot.
   */
  WIN32_FIND_DATAW  find_data;
  /*
   * Monotonic count returned in dirents.
   */
  int               off;
  /*
   * Set when no more files can be found, i.e., FindNextFile has
   * already returned ERROR_NO_MORE_FILES, so find_data is no longer
   * valid.
   */
  int               done;
  /*
   * Pattern representing the directory we are listing.
   * This is cached in the descriptor so that it can be restarted
   * (i.e. rewinddir)
   */
  wchar_t           pattern[NACL_CONFIG_PATH_MAX + 1];
};

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_NACL_HOST_DIR_TYPES_H_ */
