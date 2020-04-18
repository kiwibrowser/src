/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime logging code.
 */

#include "native_client/src/include/portability.h"
#include <stdio.h>
#include "windows.h"

#include "native_client/src/shared/platform/nacl_timestamp.h"

/*
 * TODO(bsy): split formatting from getting the time.
 */
char  *NaClTimeStampString(char   *buffer,
                           size_t buffer_size) {
  SYSTEMTIME  systime;

  GetSystemTime(&systime);

  _snprintf_s(buffer, buffer_size, _TRUNCATE, "%02d:%02d:%02d.%06d",
              systime.wHour, systime.wMinute, systime.wSecond,
              systime.wMilliseconds*1000);
  return buffer;
}
