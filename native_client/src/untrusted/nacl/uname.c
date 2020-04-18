/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/include/sys/utsname.h"

int uname(struct utsname *buf) {
  memset(buf, 0, sizeof(struct utsname));
  strcpy(buf->sysname, "NaCl");
  gethostname(buf->nodename, _UTSNAME_LENGTH);
  /*
   * TODO(sbc): It might be nice if we had some way (sysconf or IRT interface)
   * to query the runtime for version information but that doesn't exist
   * today.
   */
  strcpy(buf->version, "unknown");
  strcpy(buf->release, "unknown");
#if defined(__x86_64__)
  strcpy(buf->machine, "x86_64");
#elif defined(__i386__)
  strcpy(buf->machine, "i686");
#elif defined(__arm__)
  strcpy(buf->machine, "arm");
#elif defined(__mips__)
  strcpy(buf->machine, "mips");
#elif defined(__pnacl__)
  strcpy(buf->machine, "pnacl");
#else
#error "Unknown architecture"
#endif
  return 0;
}
