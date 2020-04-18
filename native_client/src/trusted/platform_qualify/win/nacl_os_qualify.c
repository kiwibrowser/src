/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/platform_qualify/nacl_os_qualify.h"

/*
 * Returns 1 if the operating system can run Native Client modules.
 */
int NaClOsIsSupported(void) {
  SYSTEM_INFO system_info;
  OSVERSIONINFO version_info;

  memset(&version_info, 0, sizeof(version_info));
  version_info.dwOSVersionInfoSize = sizeof(version_info);
  if (!GetVersionEx(&version_info)) {
    /* If the API doesn't work we have to assume the worst. */
    return 0;
  }
  if (5 > version_info.dwMajorVersion ||
      ((5 == version_info.dwMajorVersion) &&
       (0 == version_info.dwMinorVersion))) {
    /* We do not support versions prior to Windows XP. */
    return 0;
  }
  /*
   * GetNativeSystemInfo is only available on Windows XP and after.
   */
  GetNativeSystemInfo(&system_info);
  if (PROCESSOR_ARCHITECTURE_AMD64 != system_info.wProcessorArchitecture &&
      PROCESSOR_ARCHITECTURE_INTEL != system_info.wProcessorArchitecture) {
    /*
     * The installed operating system processor architecture is either
     * Itanium or unknown.
     */
    return 0;
  }
#ifndef _WIN64
  if (NaClOsIs64BitWindows()) {
    /*
     * Must run 64-bit nacl on 64-bit Windows. 32-bit modules won't work
     * due to lack of support for segment register modification.
     */
    return 0;
  }
#endif
  return 1;
}


/*
 * Returns 1 if the operating system is a 64-bit version of
 * Windows.
 */
int NaClOsIs64BitWindows(void) {
  SYSTEM_INFO system_info;

  GetNativeSystemInfo(&system_info);
  if (PROCESSOR_ARCHITECTURE_AMD64 == system_info.wProcessorArchitecture) {
    /*
     * The installed operating system processor architecture is x86-64.
     * This assumes the caller already knows it's a supported architecture.
     */
    return 1;
  }
  return 0;
}
