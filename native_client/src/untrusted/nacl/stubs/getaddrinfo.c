/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `getaddrinfo' for porting support.
 */

#include <errno.h>

struct addrinfo;

/* TODO(sbc): remove this once we have netdb.h in newlib toolchain */
#define EAI_SYSTEM (-1)

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res) {
  return EAI_SYSTEM;
}
