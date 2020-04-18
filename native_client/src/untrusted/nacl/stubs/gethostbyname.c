/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `gethostbyname' for porting support.
 */

#include <errno.h>

struct hostent;

struct hostent *gethostbyname(const char *name) {
  return NULL;
}
