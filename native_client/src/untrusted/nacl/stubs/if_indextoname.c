/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `if_indextoname' for porting support.
 */

#include <errno.h>

char *if_indextoname(unsigned ifindex, char *ifname) {
  errno = ENOSYS;
  return NULL;
}
