/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `gai_strerror' for porting support.
 */

#include <errno.h>

const char *gai_strerror(int errcode) {
  return NULL;
}
