/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test ensures that the backends can deal with bit manipulation
 * intrinsics.
 */

#include <stdio.h>
#include <stdint.h>
#include "native_client/tests/toolchain/utils.h"

/*  __builtin_popcount[ll](x) */
uint32_t llvm_intrinsic_ctpop(uint32_t) __asm__("llvm.ctpop.i32");
uint64_t llvm_intrinsic_ctpopll(uint64_t) __asm__("llvm.ctpop.i64");

/* __builtin_clz[ll](x) */
uint32_t llvm_intrinsic_ctlz(uint32_t, _Bool) __asm__("llvm.ctlz.i32");
uint64_t llvm_intrinsic_ctlzll(uint64_t, _Bool) __asm__("llvm.ctlz.i64");

/* __builtin_ctz[ll](x) */
uint32_t llvm_intrinsic_cttz(uint32_t, _Bool) __asm__("llvm.cttz.i32");
uint64_t llvm_intrinsic_cttzll(uint64_t, _Bool) __asm__("llvm.cttz.i64");

/* __builtin_bswap{16,32,64}(x) */
uint16_t llvm_intrinsic_bswap16(uint16_t) __asm__("llvm.bswap.i16");
uint32_t llvm_intrinsic_bswap32(uint32_t) __asm__("llvm.bswap.i32");
uint64_t llvm_intrinsic_bswap64(uint64_t) __asm__("llvm.bswap.i64");

/* volatile prevents partial evaluation by llvm */
volatile uint16_t i16[] = {0, 0xabcd, 0xdcba, 0xffff};
volatile uint32_t i32[] = {0, 0x01234567, 0x12345670, 0xffffffff};
volatile uint64_t i64[] = {0, 0x0123456789abcdefll,
                           0x0123456789abcdef0ll, 0xffffffffffffffffll};

/*
 * LLVM had a bug generating ARM code with large array offsets. The function
 * needs a constant offset > 255 to trigger the bug.
 */
volatile uint16_t i16l[257] = {[256] = 0xabcd};
volatile uint32_t i32l[257] = {[256] = 0x01234567};
volatile uint64_t i64l[257] = {[256] = 0x0123456789abcdefll};

#define print_op(op, x)                                                 \
  printf("%30s: %u\n", #op " (llvm)", (unsigned) llvm_intrinsic_ ## op(x))

#define print_op2(op, x, y)                                             \
  printf("%30s: %u\n", #op " (llvm)", (unsigned) llvm_intrinsic_ ## op(x, y))

#define print_op_builtin(op, x)                                     \
  printf("%30s: %u\n", #op " (builtin)", (unsigned) __builtin_ ## op(x))

int main(int argc, char* argv[]) {
  int i;

  for (i = 0; i < ARRAY_SIZE_UNSAFE(i16); ++i) {
    printf("\n%30s: 0x%04hx\n", "i16 value is", i16[i]);
    printf("%30s: 0x%04hx\n", "bswap (llvm)", llvm_intrinsic_bswap16(i16[i]));
    printf("%30s: 0x%04hx\n", "bswap (builtin)", __builtin_bswap16(i16[i]));
  }

  for (i = 0; i < ARRAY_SIZE_UNSAFE(i32); ++i) {
    printf("\n%30s: 0x%08x\n", "i32 value is", i32[i]);
    print_op(ctpop, i32[i]);
    print_op_builtin(popcount, i32[i]);

    print_op2(ctlz, i32[i], 0 /* Don't return undef for 0 input! */);
    /* 0 gives undefined results for the builtin on x86 */
    if (i32[i] == 0)
      printf("%30s: %u\n", "clz (builtin-manual-check)", 32);
    else
      print_op_builtin(clz, i32[i]);

    print_op2(cttz, i32[i], 0 /* Don't return undef for 0 input! */);
    /* 0 gives undefined results for the builtin on x86 */
    if (i32[i] == 0)
      printf("%30s: %u\n", "ctz (builtin-manual-check)", 32);
    else
      print_op_builtin(ctz, i32[i]);

    printf("%30s: 0x%08x\n", "bswap (llvm)", llvm_intrinsic_bswap32(i32[i]));
    printf("%30s: 0x%08x\n", "bswap (builtin)", __builtin_bswap32(i32[i]));
  }

  for (i = 0; i < ARRAY_SIZE_UNSAFE(i64); ++i) {
    printf("\n%30s: 0x%016llx\n", "i64 value is", i64[i]);
    print_op(ctpopll, i64[i]);
    print_op_builtin(popcountll, i64[i]);

    print_op2(ctlzll, i64[i], 0 /* Don't return undef for 0 input! */);
    /* 0 gives undefined results for the builtin on x86 */
    if (i64[i] == 0)
      printf("%30s: %u\n", "clzll (builtin-manual-check)", 64);
    else
      print_op_builtin(clzll, i64[i]);

    print_op2(cttzll, i64[i], 0 /* Don't return undef for 0 input! */);
    /* 0 gives undefined results for the builtin on x86 */
    if (i64[i] == 0)
      printf("%30s: %u\n", "ctzll (builtin-manual-check)", 64);
    else
      print_op_builtin(ctzll, i64[i]);

    printf("%30s: 0x%016llx\n", "bswapll (llvm)",
           llvm_intrinsic_bswap64(i64[i]));
    printf("%30s: 0x%016llx\n", "bswapll (builtin)",
           __builtin_bswap64(i64[i]));
  }

  printf("\nLarge offset tests:\n");
  printf("%30s: 0x%04hx\n", "bswap (llvm)", llvm_intrinsic_bswap16(i16l[256]));
  printf("%30s: 0x%04hx\n", "bswap (builtin)", __builtin_bswap16(i16l[256]));
  printf("%30s: 0x%08x\n", "bswap (llvm)", llvm_intrinsic_bswap32(i32l[256]));
  printf("%30s: 0x%08x\n", "bswap (builtin)", __builtin_bswap32(i32l[256]));
  printf("%30s: 0x%016llx\n", "bswapll (llvm)",
         llvm_intrinsic_bswap64(i64l[256]));
  printf("%30s: 0x%016llx\n", "bswapll (builtin)",
         __builtin_bswap64(i64l[256]));

  return 0;
}
