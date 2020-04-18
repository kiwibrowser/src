/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>
#include <sys/utsname.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/platform_qualify/kernel_version.h"
#include "native_client/src/trusted/platform_qualify/nacl_os_qualify.h"


#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32

static const char *kMinimumVersion = "2.6.27";

/*
 * Checks for this bug:
 * http://code.google.com/p/nativeclient/issues/detail?id=2032
 */
static int NaClModifyLdtHasProblems(const char *version) {
  int res;
  if (!NaClCompareKernelVersions(version, kMinimumVersion, &res)) {
    NaClLog(LOG_ERROR, "NaClModifyLdtHasProblems: "
            "Couldn't parse kernel version: %s\n", version);
    return 1;
  }
  return res < 0;
}

#endif

/*
 * Returns 1 if the operating system can run Native Client modules.
 */
int NaClOsIsSupported(void) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  struct utsname info;
  if (uname(&info) != 0) {
    NaClLog(LOG_ERROR, "NaClOsIsSupported: uname() failed\n");
    return 0;
  }
  if (NaClModifyLdtHasProblems(info.release)) {
    NaClLog(LOG_ERROR, "NaClOsIsSupported: This system is running an "
            "old Linux kernel. This kernel version is buggy and "
            "Native Client's x86-32 sandbox might be insecure on "
            "this kernel. The fix is to upgrade the kernel to v2.6.27 "
            "or later, or, as a workaround, to switch using a 64-bit "
            "Native Client sandbox. For more information, see "
            "http://code.google.com/p/nativeclient/issues/detail?id=2032\n");
    return 0;
  }
#endif

  return 1;
}


/*
 * Returns 1 if the operating system is a 64-bit version of
 * Windows.  For now, all of these versions are not supported.
 */
int NaClOsIs64BitWindows(void) {
  return 0;
}
