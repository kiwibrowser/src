/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/nacl_assert.h"

#ifdef _NEWLIB_VERSION
#include <stdint.h>
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);
#else
#include <arpa/inet.h>
#endif

int main(void) {
  /*
   * NaCl is always little-endian, and network order is big-endian
   * so ntohl(3) and friends are always expected to reverse the
   * byte order.
   */
  uint32_t netlong = 0x01020304;
  uint32_t hostlong = 0x04030201;
  uint32_t netshort = 0x0102;
  uint32_t hostshort = 0x0201;

  ASSERT_EQ(hostlong, ntohl(netlong));
  ASSERT_EQ(netlong, htonl(hostlong));

  ASSERT_EQ(hostshort, ntohs(netshort));
  ASSERT_EQ(netshort, htons(hostshort));

  return 0;
}
