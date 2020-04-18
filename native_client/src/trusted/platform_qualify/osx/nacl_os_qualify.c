/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <string.h>
#include <sys/utsname.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/platform_qualify/kernel_version.h"
#include "native_client/src/trusted/platform_qualify/nacl_os_qualify.h"


#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32

/*
 * The 64-bit Mac kernel bug was fixed in kernel version 10.8.0, which
 * is in Mac OS X 10.6.8.
 */
static const char *kMinimumKernelVersion = "10.8";

static int KernelVersionIsBuggy(const char *version) {
  int res;
  if (!NaClCompareKernelVersions(version, kMinimumKernelVersion, &res)) {
    NaClLog(LOG_ERROR, "KernelVersionIsBuggy: "
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
  if (strcmp(info.machine, "x86_64") == 0 &&
      KernelVersionIsBuggy(info.release)) {
    NaClLog(LOG_ERROR,
            "NaClOsIsSupported: "
            "This system is running an old 64-bit Mac OS X kernel.  "
            "This kernel version is buggy and can crash when running "
            "Native Client's x86-32 sandbox.  "
            "The fix is to upgrade to Mac OS X 10.6.8 or later, or, as a "
            "workaround, to switch to using a 32-bit kernel, which will "
            "be capable of running Native Client.  For more information, see "
            "http://code.google.com/p/nativeclient/issues/detail?id=1712\n");
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

/*
 * Returns the Darwin Major Version. Returns 0 on error.
 */
static int DarwinMajorVersion(void) {
  struct utsname uname_info;
  if (uname(&uname_info) != 0) {
    NaClLog(LOG_ERROR, "uname error\n");
    return 0;
  }

  if (strcmp(uname_info.sysname, "Darwin") != 0) {
    NaClLog(LOG_ERROR, "unexpected uname sysname %s\n", uname_info.sysname);
    return 0;
  }

  int major_version, minor_version, dot_version;
  if (!NaClParseKernelVersion(uname_info.release, &major_version,
                              &minor_version, &dot_version)) {
    NaClLog(LOG_ERROR, "couldn't parse release %s\n", uname_info.release);
    return 0;
  }

  return major_version;
}

static int g_darwin_major_version = 0;

static void InitializeDarwinMajorVersion() {
  g_darwin_major_version = DarwinMajorVersion();
}

/*
 * Returns the OS X Major Version. Caches the result of DarwinMajorVersion().
 */
static int OSXMajorVersion(void) {
  static pthread_once_t once_control = PTHREAD_ONCE_INIT;
  pthread_once(&once_control, InitializeDarwinMajorVersion);
  return g_darwin_major_version - 4;
}

int NaClOSX10Dot10OrLater(void) {
  return OSXMajorVersion() >= 10;
}

int NaClOSX10Dot7OrLater(void) {
  return OSXMajorVersion() >= 7;
}
