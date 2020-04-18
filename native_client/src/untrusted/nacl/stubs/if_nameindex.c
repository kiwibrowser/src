/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `if_nameindex' for porting support.
 */

#include <errno.h>

struct if_nameindex;

struct if_nameindex *if_nameindex(void) {
  errno = ENOSYS;
  return NULL;
}
