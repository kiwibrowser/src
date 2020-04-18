/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `getservbyport' for porting support.
 */

#include <stddef.h>

struct servent *getservbyport(int port, const char *proto) {
  return NULL;
}
