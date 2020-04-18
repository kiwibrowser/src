/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl local descriptor table (LDT) management - common for all platforms
 */

#include "native_client/src/trusted/service_runtime/arch/x86/nacl_ldt_x86.h"

/* TODO(gregoryd): These need to come from a header file. */
extern int NaClLdtInitPlatformSpecific(void);
extern int NaClLdtFiniPlatformSpecific(void);


int NaClLdtInit(void) {
  return NaClLdtInitPlatformSpecific();
}


void NaClLdtFini(void) {
  NaClLdtFiniPlatformSpecific();
}
