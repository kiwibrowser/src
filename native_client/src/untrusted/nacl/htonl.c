/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>

uint32_t htonl(uint32_t hostlong) {
  return __builtin_bswap32(hostlong);
}
