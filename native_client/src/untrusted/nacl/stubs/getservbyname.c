/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `getservbyname' for porting support.
 */

#include <stddef.h>

struct servent *getservbyname(const char *name, const char *proto) {
  return NULL;
}
