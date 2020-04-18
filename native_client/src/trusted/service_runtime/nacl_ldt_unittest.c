/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Test code for NaCl local descriptor table (LDT) managment
 */
#include <stdio.h>
#include "native_client/src/trusted/service_runtime/arch/x86/nacl_ldt_x86.h"

int main(void) {
  uint16_t a, b, c, d, e;

  /* Initialize LDT services. */
  NaClLdtInit();

  /* Data, not read only */
  a = NaClLdtAllocatePageSelector(NACL_LDT_DESCRIPTOR_DATA, 0, 0, 0x000ff);
  printf("a = %0x\n", a);
  NaClLdtPrintSelector(a);

  /* Data, read only */
  b = NaClLdtAllocatePageSelector(NACL_LDT_DESCRIPTOR_DATA, 1, 0, 0x000ff);
  printf("b = %0x\n", b);
  NaClLdtPrintSelector(b);

  /* Data, read only */
  c = NaClLdtAllocatePageSelector(NACL_LDT_DESCRIPTOR_DATA, 1, 0, 0x000ff);
  printf("c = %0x\n", c);
  NaClLdtPrintSelector(c);

  /* Delete b */
  NaClLdtDeleteSelector(b);
  printf("b (after deletion) = %0x\n", b);
  NaClLdtPrintSelector(b);

  /* Since there is only one thread, d should grab slot previously holding b */
  d = NaClLdtAllocatePageSelector(NACL_LDT_DESCRIPTOR_DATA, 1, 0, 0x000ff);
  printf("d = %0x\n", d);
  NaClLdtPrintSelector(d);

  /* Code selector */
  e = NaClLdtAllocatePageSelector(NACL_LDT_DESCRIPTOR_CODE, 1, 0, 0x000ff);
  printf("e (code) = %0x\n", e);
  NaClLdtPrintSelector(e);

  /* Shut down LDT services. */
  NaClLdtFini();
  return 0;
}
