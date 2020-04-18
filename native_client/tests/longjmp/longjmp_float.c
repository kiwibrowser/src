/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>

static volatile double should_be_two = 2.0;

int main(int argc, char* argv[]) {
  jmp_buf buf;
  /* LLVM assembler doesn't parse vfpv3, but we've set the default */
#if !defined(__llvm__)
  __asm__(".fpu vfpv3");
#endif

  assert(should_be_two == 2.0);
  /* Copy |should_be_two| into regs. */
  __asm__("vmov.f64 d8, %P[value]\n\t"
          "vmov.f64 d9, %P[value]\n\t"
          "vmov.f64 d10, %P[value]\n\t"
          "vmov.f64 d11, %P[value]\n\t"
          "vmov.f64 d12, %P[value]\n\t"
          "vmov.f64 d13, %P[value]\n\t"
          "vmov.f64 d14, %P[value]\n\t"
          "vmov.f64 d15, %P[value]"
      :
      : [value] "w" (should_be_two)
      : "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15" );
  if (!setjmp(buf) ) {
    /* Clobber the regs */
    __asm__("vmov.f64 d8, #1.0\n\t"
      "vmov.f64 d9, #1.0\n\t"
      "vmov.f64 d10, #1.0\n\t"
      "vmov.f64 d11, #1.0\n\t"
      "vmov.f64 d12, #1.0\n\t"
      "vmov.f64 d13, #1.0\n\t"
      "vmov.f64 d14, #1.0\n\t"
      "vmov.f64 d15, #1.0"
      : :
      : "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15" );
    longjmp(buf, 1);
    abort();
  } else {
    double restored_val;
    __asm__("vmov.f64 %P0, d8\n\t" : "=w" (restored_val) : );
    assert (restored_val == 2.0);
    __asm__("vmov.f64 %P0, d9\n\t" : "=w" (restored_val) : );
    assert (restored_val == 2.0);
    __asm__("vmov.f64 %P0, d10\n\t" : "=w" (restored_val) : );
    assert (restored_val == 2.0);
    __asm__("vmov.f64 %P0, d11\n\t" : "=w" (restored_val) : );
    assert (restored_val == 2.0);
    __asm__("vmov.f64 %P0, d12\n\t" : "=w" (restored_val) : );
    assert (restored_val == 2.0);
    __asm__("vmov.f64 %P0, d13\n\t" : "=w" (restored_val) : );
    assert (restored_val == 2.0);
    __asm__("vmov.f64 %P0, d14\n\t" : "=w" (restored_val) : );
    assert (restored_val == 2.0);
    __asm__("vmov.f64 %P0, d15\n\t" : "=w" (restored_val) : );
    assert (restored_val == 2.0);
    return 0;
  }
  abort();
}
