/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `getnameinfo' for porting support.
 */

#include <errno.h>

struct sockaddr;

/* TODO(sbc): remove this once we have netdb.h in newlib toolchain */
#define EAI_SYSTEM (-1)

int getnameinfo(const struct sockaddr *sa, unsigned int salen,
                char *host, size_t hostlen,
                char *serv, size_t servlen, int flags) {
  errno = ENOSYS;
  return EAI_SYSTEM;
}
