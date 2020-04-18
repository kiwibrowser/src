/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__i386__) || defined(__x86_64__)

typedef uint16_t fpcw_t;

#define X86_FPCW_PREC_MASK      0x300
#define X86_FPCW_PREC_DOUBLE    0x200
#define X86_FPCW_PREC_EXTENDED  0x300

static uint16_t get_fpcw(void) {
  uint16_t fpcw;
  __asm__ __volatile__("fnstcw %0": "=m" (fpcw));
  return fpcw;
}

static void set_fpcw(uint16_t fpcw) {
  __asm__  __volatile__("fldcw %0": : "m" (fpcw));
}

static fpcw_t modify_fpcw_bits(fpcw_t orig_fpcw) {
  assert(orig_fpcw & X86_FPCW_PREC_EXTENDED);
  return (orig_fpcw & ~X86_FPCW_PREC_MASK) | X86_FPCW_PREC_DOUBLE;
}

typedef uint32_t mxcsr_t;

#define X86_MXCSR_FLUSH_ZERO    0x8000

static mxcsr_t get_mxcsr(void) {
  uint32_t mxcsr;
  __asm__ __volatile__("stmxcsr %0" : "=m" (mxcsr));
  return mxcsr;
}

static void set_mxcsr(mxcsr_t mxcsr) {
  __asm__ __volatile__("ldmxcsr %0" :: "m" (mxcsr));
}

static mxcsr_t modify_mxcsr_bits(mxcsr_t orig_mxcsr) {
  assert((orig_mxcsr & X86_MXCSR_FLUSH_ZERO) == 0);
  return orig_mxcsr | X86_MXCSR_FLUSH_ZERO;
}

#elif defined(__arm__)

typedef uint32_t fpcw_t;

#define ARM_FPSCR_ROUND_MASK            0xc00000
#define ARM_FPSCR_ROUND_TONEAREST       0x000000
#define ARM_FPSCR_ROUND_UPWARD          0x800000
#define ARM_FPSCR_ROUND_DOWNWARD        0x400000
#define ARM_FPSCR_ROUND_TOWARDZERO      0xc00000

/*
 * These are defined in test_fpu_control_word_arm.S because
 * PNaCl/LLVM cannot handle the inline assembly for these instructions.
 */
fpcw_t get_fpcw(void);
void set_fpcw(fpcw_t);

static fpcw_t modify_fpcw_bits(fpcw_t orig_fpcw) {
  assert((orig_fpcw & ARM_FPSCR_ROUND_MASK) == ARM_FPSCR_ROUND_TONEAREST);
  return (orig_fpcw & ~ARM_FPSCR_ROUND_MASK) | ARM_FPSCR_ROUND_TOWARDZERO;
}

typedef int mxcsr_t;

static mxcsr_t get_mxcsr(void) {
  return 0;
}

static void set_mxcsr(mxcsr_t mxcsr __attribute__((unused))) {
}

static mxcsr_t modify_mxcsr_bits(mxcsr_t orig_mxcsr) {
  return orig_mxcsr;
}

#else
#error unsupported architecture
#endif

int main(void) {
  int result = 0;
  fpcw_t fpcw1;
  fpcw_t fpcw2;
  mxcsr_t mxcsr1;
  mxcsr_t mxcsr2;

  /*
   * Change the FPU control word to a non-default setting.
   */
  fpcw1 = get_fpcw();
  fpcw1 = modify_fpcw_bits(fpcw1);
  set_fpcw(fpcw1);

  mxcsr1 = get_mxcsr();
  mxcsr1 = modify_mxcsr_bits(mxcsr1);
  set_mxcsr(mxcsr1);

  /*
   * Now do a system call.  It's supposed to clear the rest of the FPU
   * state, but leave the control word intact.
   */
  sched_yield();

  /*
   * Now check what we got.
   */
  fpcw2 = get_fpcw();
  if (fpcw2 != fpcw1) {
    fprintf(stderr, "Set FPCW to %#x and got back %#x\n",
            (unsigned int) fpcw1, (unsigned int) fpcw2);
    result = 1;
  }

  mxcsr2 = get_mxcsr();
  if (mxcsr2 != mxcsr1) {
    fprintf(stderr, "Set MXCSR to %#x and got back %#x\n",
            (unsigned int) mxcsr1, (unsigned int) mxcsr2);
    result = 1;
  }

  return result;
}
