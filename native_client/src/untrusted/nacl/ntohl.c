/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>

uint32_t ntohl(uint32_t netlong) {
  return __builtin_bswap32(netlong);
}
