/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

static int getpid_not_implemented(int *pid) {
  return ENOSYS;
}

pid_t getpid(void) {
  /*
   * Note that getpid() is not normally documented as returning an
   * error or setting errno.  For example,
   * http://pubs.opengroup.org/onlinepubs/007904975/functions/getpid.html
   * says "The getpid() function shall always be successful and no
   * return value is reserved to indicate an error".  However, that is
   * not reasonable for systems that don't support getpid(), such as
   * NaCl in the web browser.
   */
  if (__libnacl_irt_dev_getpid.getpid == NULL) {
    if (__nacl_irt_query(NACL_IRT_DEV_GETPID_v0_1, &__libnacl_irt_dev_getpid,
                         sizeof(__libnacl_irt_dev_getpid)) !=
        sizeof(__libnacl_irt_dev_getpid)) {
      __libnacl_irt_dev_getpid.getpid = getpid_not_implemented;
    }
  }
  int pid;
  int error = __libnacl_irt_dev_getpid.getpid(&pid);
  if (error != 0) {
    errno = error;
    return -1;
  }
  return pid;
}
