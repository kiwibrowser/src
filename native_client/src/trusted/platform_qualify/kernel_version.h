/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_KERNEL_VERSION_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_KERNEL_VERSION_H_

#include <stdlib.h>
#include <string.h>
#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Returns 1 on success and fills num1.num2.num3 appropriately.
 */
static INLINE int NaClParseKernelVersion(const char *version, int *num1,
                                         int *num2, int *num3) {
  char *next;
  *num1 = strtol(version, &next, 10);
  if (next == NULL || *next != '.') {
    return 0;
  }
  *num2 = strtol(next + 1, &next, 10);
  if (next == NULL) {
    return 0;
  }
  if (*next != '.') {
    *num3 = 0;
    return 1;
  }
  *num3 = strtol(next + 1, &next, 10);
  if (next == NULL) {
    return 0;
  }
  /* Ignore the rest of the version string. */
  return 1;
}

/*
 * Returns 1 on success and fills res with -1, 0 or 1,
 * if version1 <, == or > version2 accordingly.
 */
static INLINE int NaClCompareKernelVersions(const char *version1,
                                            const char *version2,
                                            int *res) {
  int i;
  int num1[3];
  int num2[3];
  if (!NaClParseKernelVersion(version1, &num1[0], &num1[1], &num1[2])) {
    return 0;
  }
  if (!NaClParseKernelVersion(version2, &num2[0], &num2[1], &num2[2])) {
    return 0;
  }
  for (i = 0; i < 3; i++) {
    if (num1[i] < num2[i]) {
      *res = -1;
      return 1;
    }
    if (num1[i] > num2[i]) {
      *res = 1;
      return 1;
    }
  }
  *res = 0;
  return 1;
}

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_KERNEL_VERSION_H_ */
