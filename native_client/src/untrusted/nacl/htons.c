/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>

uint16_t htons(uint16_t hostshort) {
  return ((hostshort & 0xff) << 8) | (hostshort >> 8);
}
