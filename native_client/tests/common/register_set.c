/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/common/register_set.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_assert.h"

#if defined(__native_client__)
# include "native_client/src/untrusted/nacl/nacl_thread.h"
#endif


struct RegInfo {
  const char *reg_name;
  int reg_offset;
  int reg_size;
  int user_register_state_reg_offset;
  int user_register_state_reg_size;
};

#define DEFINE_REG(regname) \
    { \
      #regname, \
      offsetof(struct NaClSignalContext, regname), \
      sizeof(((struct NaClSignalContext *) NULL)->regname), \
      offsetof(NaClUserRegisterState, regname), \
      sizeof(((NaClUserRegisterState *) NULL)->regname) \
    }

const struct RegInfo kRegs[] = {
  /*
   * We do not look at x86 segment registers because they are not
   * saved by REGS_SAVER_FUNC.
   */
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  DEFINE_REG(eax),
  DEFINE_REG(ecx),
  DEFINE_REG(edx),
  DEFINE_REG(ebx),
  DEFINE_REG(stack_ptr),
  DEFINE_REG(ebp),
  DEFINE_REG(esi),
  DEFINE_REG(edi),
  DEFINE_REG(prog_ctr),
  DEFINE_REG(flags)
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  DEFINE_REG(rax),
  DEFINE_REG(rbx),
  DEFINE_REG(rcx),
  DEFINE_REG(rdx),
  DEFINE_REG(rsi),
  DEFINE_REG(rdi),
  DEFINE_REG(rbp),
  DEFINE_REG(stack_ptr),
  DEFINE_REG(r8),
  DEFINE_REG(r9),
  DEFINE_REG(r10),
  DEFINE_REG(r11),
  DEFINE_REG(r12),
  DEFINE_REG(r13),
  DEFINE_REG(r14),
  DEFINE_REG(r15),
  DEFINE_REG(prog_ctr),
  DEFINE_REG(flags)
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  DEFINE_REG(r0),
  DEFINE_REG(r1),
  DEFINE_REG(r2),
  DEFINE_REG(r3),
  DEFINE_REG(r4),
  DEFINE_REG(r5),
  DEFINE_REG(r6),
  DEFINE_REG(r7),
  DEFINE_REG(r8),
  DEFINE_REG(r9),
  DEFINE_REG(r10),
  DEFINE_REG(r11),
  DEFINE_REG(r12),
  DEFINE_REG(stack_ptr),
  DEFINE_REG(lr),
  DEFINE_REG(prog_ctr),
  DEFINE_REG(cpsr)
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  DEFINE_REG(zero),
  DEFINE_REG(at),
  DEFINE_REG(v0),
  DEFINE_REG(v1),
  DEFINE_REG(a0),
  DEFINE_REG(a1),
  DEFINE_REG(a2),
  DEFINE_REG(a3),
  DEFINE_REG(t0),
  DEFINE_REG(t1),
  DEFINE_REG(t2),
  DEFINE_REG(t3),
  DEFINE_REG(t4),
  DEFINE_REG(t5),
  DEFINE_REG(t6),
  DEFINE_REG(t7),
  DEFINE_REG(s0),
  DEFINE_REG(s1),
  DEFINE_REG(s2),
  DEFINE_REG(s3),
  DEFINE_REG(s4),
  DEFINE_REG(s5),
  DEFINE_REG(s6),
  DEFINE_REG(s7),
  DEFINE_REG(t8),
  DEFINE_REG(t9),
  DEFINE_REG(k0),
  DEFINE_REG(k1),
  DEFINE_REG(global_ptr),
  DEFINE_REG(stack_ptr),
  DEFINE_REG(frame_ptr),
  DEFINE_REG(return_addr),
  DEFINE_REG(prog_ctr)
#else
# error Unsupported architecture
#endif
};

#undef DEFINE_REG

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
/* Flags readable and writable by untrusted code. */
const uint8_t kX86FlagBits[5] = { 0, 2, 6, 7, 11 };
/*
 * kX86KnownFlagsMask contains kX86FlagBits plus the trap flag (which
 * is not readable or writable by untrusted code) so that trusted-code
 * tests will check that the trap flag is still set.
 */
static const uint32_t kX86KnownFlagsMask =
    (1<<0) | (1<<2) | (1<<6) | (1<<7) | (1<<11) |
    (1<<8); /* Trap flag */
#endif

void RegsFillTestValues(struct NaClSignalContext *regs, int seed) {
  unsigned int index;
  for (index = 0; index < sizeof(*regs); index++) {
    ((char *) regs)[index] = index + seed + 1;
  }
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  /* Set x86 flags to a value that we know will work. */
  regs->flags = RESET_X86_FLAGS_VALUE;
#endif
}

#if defined(__native_client__)
void RegsApplySandboxConstraints(struct NaClSignalContext *regs) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  uint64_t r15;
  __asm__("mov %%r15, %0" : "=r"(r15));
  regs->r15 = r15;
  regs->prog_ctr = r15 + (uint32_t) regs->prog_ctr;
  regs->stack_ptr = r15 + (uint32_t) regs->stack_ptr;
  regs->rbp = r15 + (uint32_t) regs->rbp;
#else
  UNREFERENCED_PARAMETER(regs);
#endif
}
#endif

static uint64_t RegsGetRegValue(const struct NaClSignalContext *regs,
                                unsigned int regnum) {
  uintptr_t ptr = (uintptr_t) regs + kRegs[regnum].reg_offset;
  assert(regnum < NACL_ARRAY_SIZE(kRegs));
  if (kRegs[regnum].reg_size == 4) {
    return *(uint32_t *) ptr;
  } else if (kRegs[regnum].reg_size == 8) {
    return *(uint64_t *) ptr;
  } else {
    fprintf(stderr, "Unknown register size: %i\n", kRegs[regnum].reg_size);
    _exit(1);
  }
}

static void RegsDump(const struct NaClSignalContext *regs) {
  unsigned int regnum;
  for (regnum = 0; regnum < NACL_ARRAY_SIZE(kRegs); regnum++) {
    long long value = RegsGetRegValue(regs, regnum);
    fprintf(stderr, "  %s=0x%llx (%lld)\n",
            kRegs[regnum].reg_name, value, value);
  }
}

static void RegsNormalizeFlags(struct NaClSignalContext *regs) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  regs->flags &= kX86KnownFlagsMask;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  regs->cpsr &= REGS_ARM_USER_CPSR_FLAGS_MASK;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  /* No flags field on MIPS. */
  UNREFERENCED_PARAMETER(regs);
#endif
}

void RegsAssertEqual(const struct NaClSignalContext *actual,
                     const struct NaClSignalContext *expected) {
  int match = 1;
  unsigned int regnum;

  /* Make a copy so that we can ignore some of the x86/ARM flags. */
  struct NaClSignalContext copy_actual = *actual;
  struct NaClSignalContext copy_expected = *expected;
  RegsNormalizeFlags(&copy_actual);
  RegsNormalizeFlags(&copy_expected);

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  /*
   * We skip comparison of r9 because it is not supposed to be
   * settable or readable by untrusted code.  However, for debugging
   * purposes we still include r9 in register dumps printed by
   * RegsDump(), so r9 remains listed in kRegs[].
   */
  copy_expected.r9 = copy_actual.r9;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  /*
   * The registers below are all read-only, so we skip their comparison. We
   * expect t6 and t7 to hold control flow masks. For t8, which holds TLS
   * index, we skip comparison. Zero register holds zero always. All of the
   * above named registers are not settable by untrusted code. We also skip
   * comparison for k0 and k1 registers because those are registers reserved for
   * use by interrupt/trap handler and therefore volatile.
   * However, for debugging purposes we still include those in register dumps
   * printed by RegsDump(), so they remain listed in kRegs[].
   */
  copy_expected.zero = 0;
  copy_expected.t6 = NACL_CONTROL_FLOW_MASK;
  copy_expected.t7 = NACL_DATA_FLOW_MASK;
  copy_expected.t8 = copy_actual.t8;
  copy_expected.k0 = copy_actual.k0;
  copy_expected.k1 = copy_actual.k1;
#endif

  for (regnum = 0; regnum < NACL_ARRAY_SIZE(kRegs); regnum++) {
    if (RegsGetRegValue(&copy_actual, regnum) !=
        RegsGetRegValue(&copy_expected, regnum)) {
      fprintf(stderr, "Mismatch in register %s\n", kRegs[regnum].reg_name);
      match = 0;
    }
  }
  if (!match) {
    fprintf(stderr, "Expected register state:\n");
    RegsDump(expected);
    fprintf(stderr, "Actual register state:\n");
    RegsDump(actual);
    _exit(1);
  }
}

void RegsCopyFromUserRegisterState(struct NaClSignalContext *dest,
                                   const NaClUserRegisterState *src) {
  unsigned int regnum;

  /* Fill out trusted registers with dummy values. */
  memset(dest, 0xff, sizeof(*dest));

  for (regnum = 0; regnum < NACL_ARRAY_SIZE(kRegs); regnum++) {
    ASSERT_EQ(kRegs[regnum].reg_size,
              kRegs[regnum].user_register_state_reg_size);
    memcpy((char *) dest + kRegs[regnum].reg_offset,
           (char *) src + kRegs[regnum].user_register_state_reg_offset,
           kRegs[regnum].reg_size);
  }
}

void RegsUnsetNonCalleeSavedRegisters(struct NaClSignalContext *regs) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  regs->eax = 0;
  regs->ecx = 0;
  regs->edx = 0;
  regs->flags = 0;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  regs->rax = 0;
  regs->rcx = 0;
  regs->rdx = 0;
  regs->rsi = 0;
  regs->rdi = 0;
  regs->r8 = 0;
  regs->r9 = 0;
  regs->r10 = 0;
  regs->r11 = 0;
  regs->flags = 0;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  regs->r0 = 0;
  regs->r1 = 0;
  regs->r2 = 0;
  regs->r3 = 0;
  regs->r12 = 0;
  regs->lr = 0;
  regs->cpsr = 0;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  regs->zero = 0;
  regs->at = 0;
  regs->v0 = 0;
  regs->v1 = 0;
  regs->a0 = 0;
  regs->a1 = 0;
  regs->a2 = 0;
  regs->a3 = 0;
  regs->t0 = 0;
  regs->t1 = 0;
  regs->t2 = 0;
  regs->t3 = 0;
  regs->t4 = 0;
  regs->t5 = 0;
  regs->t9 = 0;
  regs->k0 = 0;
  regs->k1 = 0;
  regs->global_ptr  = 0;
  regs->return_addr = 0;
#else
# error Unsupported architecture
#endif
}

#if defined(__native_client__)
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32

uintptr_t RegsGetArg1(const struct NaClSignalContext *regs) {
  return ((uint32_t *) regs->stack_ptr)[1];
}

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64

uintptr_t RegsGetArg1(const struct NaClSignalContext *regs) {
  return regs->rdi;
}

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm

uintptr_t RegsGetArg1(const struct NaClSignalContext *regs) {
  return regs->r0;
}

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips

uintptr_t RegsGetArg1(const struct NaClSignalContext *regs) {
  return regs->a0;
}

#else
# error Unsupported architecture
#endif
#endif
