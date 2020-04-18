/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>

/*
 * NaCl's sysbrk() interface is deprecated, so the sbrk() function is
 * a dummy function that always returns an error.
 * See: https://code.google.com/p/nativeclient/issues/detail?id=3542
 */
void *sbrk(intptr_t increment) {
  errno = ENOSYS;
  return (void *) -1;
}
