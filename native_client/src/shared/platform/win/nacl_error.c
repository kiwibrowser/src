/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <windows.h>

#include "native_client/src/shared/platform/nacl_error.h"

int NaClGetLastErrorString(char* buffer, size_t length) {
  DWORD error = GetLastError();
  return FormatMessageA(
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      error,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      buffer,
      (DWORD) ((64 * 1024 < length) ? 64 * 1024 : length),
      NULL) ? 0 : -1;
}
