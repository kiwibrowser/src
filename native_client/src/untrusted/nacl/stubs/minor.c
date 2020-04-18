/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `minor' for porting support.
 */

#include <sys/types.h>

int minor(dev_t dev) {
  return 0;
}
