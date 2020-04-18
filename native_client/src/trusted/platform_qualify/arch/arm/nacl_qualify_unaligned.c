/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/platform_qualify/arch/arm/nacl_arm_qualify.h"

/*
 * The misaligned load/store tests all work in a similar way:
 *  - Create a struct with a 1-byte misaligned 16/32/64-bit value. Make
 *    sure this struct is suitably aligned and packed.
 *  - For stores, memset the struct to a known value.
 *  - Load/store the struct's misaligned value with a few reg-reg and
 *    reg-imm variants.
 *  - Check that the loaded/stored value is correct.
 *  - For stores, check that the struct padding is unchanged.
 *
 * This test cares about ARM because it's the only supported NaCl
 * sandboxing platform, but it also cares about Thumb because that's how
 * Chrome (and therefore the trusted runtime) is compiled.
 */

static sigjmp_buf try_state;

static void signal_catch(int sig) {
  siglongjmp(try_state, sig);
}

#define LOAD_TEST_START(size, offset, value)                    \
  typedef NACL_CONCAT(NACL_CONCAT(uint, size), _t) Value;       \
  struct Data {                                                 \
    uint8_t misalign[offset];                                   \
    Value val;                                                  \
    uint8_t pad[sizeof(Value) - offset];                        \
  } __attribute__((aligned(sizeof(Value)), packed));            \
                                                                \
  static const struct Data data = {                             \
    { 0 }, value, { 0 /* Zero-init rest. */ }                   \
  };                                                            \
  const char *addr = (const char *) &data;                      \
  const Value val = value; /* Local aligned copy. */            \
                                                                \
  /* Used as register offsets. */                               \
  const int zero = 0;                                           \
  const int one = 1;                                            \
  const int minus_size = -sizeof(Value);                        \
                                                                \
  int success = 1;                                              \
  Value out;                                                    \
                                                                \
  NACL_COMPILE_TIME_ASSERT(                                     \
      offsetof(struct Data, val) == offset);                    \
  success &= (((uintptr_t) &data.val & (sizeof(Value) - 1)) ==  \
              offset);                                          \
                                                                \
  (void) (zero + one + minus_size) /* Sometimes unused on Thumb. */


#define STORE_TEST_START(size, offset, value)                   \
  typedef NACL_CONCAT(NACL_CONCAT(uint, size), _t) Value;       \
  struct Data {                                                 \
    uint8_t misalign[offset];                                   \
    Value val;                                                  \
    uint8_t pad[sizeof(Value) - 1];                             \
  } __attribute__((aligned(sizeof(Value)), packed));            \
                                                                \
  struct Data data;                                             \
  char *addr = (char *) &data;                                  \
  const Value val = value; /* Local aligned copy. */            \
                                                                \
  /* Used as register offsets. */                               \
  const int zero = 0;                                           \
  const int one = 1;                                            \
  const int minus_size = -sizeof(Value);                        \
                                                                \
  int success = 1;                                              \
                                                                \
  NACL_COMPILE_TIME_ASSERT(                                     \
      offsetof(struct Data, val) == offset);                    \
  success &= (((uintptr_t) &data.val & (sizeof(Value) - 1)) ==  \
              offset);                                          \
                                                                \
  (void) (zero + one + minus_size) /* Sometimes unused on Thumb. */

/*
 * Execute assembly statement, and validate the result.
 * All of them are of the form:
 *    INSTR %[Rt], [%[Rn], %[off]]
 * Where:
 *   - %[Rt] the value loaded/stored for loads/stores.
 *   - %[Rn] is Rn, the base address.
 *   - %[off] is immediate offset (#0, #+1, #-size) or Rm the register offset.
 *
 * Note that this also applies for 64-bit values: the assembler takes in
 * a single "register" even though the two 32-bit words are split into
 * two physical registers: the ISA mandates that the registers be
 * subsequent, and that the first be even.
 *
 * In the LDRD case, the ARM specification classes it UNPREDICTABLE if
 * the destination register pair overlaps with either register used in
 * the address operand.  Ordinarily, the compiler's register allocator
 * would be free to choose the same register for the destination of the
 * load as is used in the address operand (if that input register would
 * otherwise be dead after this instruction).  By making the constraint
 * "=&r" rather than plain "=r", the compiler is constrained to using
 * destination registers that can safely be clobbered while the input
 * registers are still live; that's not exactly the requirement here,
 * but it's equivalent in practice.  Note that the (looser) plain "=r"
 * constraint would be fine for all cases other than LDRD; but for this
 * code there is no reason to bother trying to help the compiler do the
 * best register allocation possible.
 */
#define LOAD_TEST(instr, address, offset_constraint, offset) do {       \
    asm(instr " %[Rt], [%[Rn], %[off]]\n"                               \
        : [Rt] "=&r" (out)                                              \
        : [Rn] "r" (address), [off] offset_constraint (offset),         \
          "m" (*(const Value *) (address + offset)));                   \
    success &= (out == val);                                            \
  } while (0)

#define LOAD_RI_TEST(instr, address, immediate_offset)                  \
  LOAD_TEST(instr, address, "i", immediate_offset)
#define LOAD_RR_TEST(instr, address, register_offset)                   \
  LOAD_TEST(instr, address, "r", register_offset)

/*
 * The canary value isn't used as a test byte in the *_TEST_START values
 * below. If the struct's padding loses the canary then something went
 * wrong.
 */
enum { CANARY = 0x69 };

#define STORE_TEST(instr, address, offset_constraint, offset) do {      \
    size_t i, o = offsetof(struct Data, val);                           \
    for (i = 0; i < o; ++i)                                             \
      data.misalign[i] = CANARY;                                        \
    for (i = 0; i < sizeof(Value) - o; ++i)                             \
      data.pad[i] = CANARY;                                             \
    asm(instr " %[Rt], [%[Rn], %[off]]\n"                               \
        : "=m" (data)                                                   \
        : [Rt] "r" (val), [Rn] "r" (address),                           \
          [off] offset_constraint (offset));                            \
    for (i = 0; i < o; ++i)                                             \
      success &= (data.misalign[i] == CANARY);                          \
    success &= (data.val == val);                                       \
    for (i = 0; i < sizeof(Value) - o; ++i)                             \
      success &= (data.pad[i] == CANARY);                               \
  } while (0)

#define STORE_RI_TEST(instr, address, immediate_offset)                 \
  STORE_TEST(instr, address, "i", immediate_offset)
#define STORE_RR_TEST(instr, address, register_offset)                  \
  STORE_TEST(instr, address, "r", register_offset)


static int test_halfword_load(void) {
  LOAD_TEST_START(16, 1, 0xDEAD);
  LOAD_RI_TEST("ldrh",  addr + 1,               0);
  LOAD_RI_TEST("ldrh",  addr,                   1);
  LOAD_RI_TEST("ldrh",  addr + 1 + sizeof(val), -sizeof(val));
  LOAD_RR_TEST("ldrh",  addr + 1,               zero);
  LOAD_RR_TEST("ldrh",  addr,                   one);
  LOAD_RR_TEST("ldrh",  addr + 1 + sizeof(val), minus_size);
  LOAD_RI_TEST("ldrsh", addr + 1,               0);
  LOAD_RI_TEST("ldrsh", addr,                   1);
  LOAD_RI_TEST("ldrsh", addr + 1 + sizeof(val), -sizeof(val));
  LOAD_RR_TEST("ldrsh", addr + 1,               zero);
  LOAD_RR_TEST("ldrsh", addr,                   one);
  LOAD_RR_TEST("ldrsh", addr + 1 + sizeof(val), minus_size);
  return success;
}

static int test_halfword_store(void) {
  STORE_TEST_START(16, 1, 0xDEAD);
  STORE_RI_TEST("strh", addr + 1,               0);
  STORE_RI_TEST("strh", addr,                   1);
  STORE_RI_TEST("strh", addr + 1 + sizeof(val), -sizeof(val));
  STORE_RR_TEST("strh", addr + 1,               zero);
  STORE_RR_TEST("strh", addr,                   one);
  STORE_RR_TEST("strh", addr + 1 + sizeof(val), minus_size);
  return success;
}

int test_word_load(void) {
  LOAD_TEST_START(32, 1, 0xC0BEBEEF);
  LOAD_RI_TEST("ldr", addr + 1,               0);
  LOAD_RI_TEST("ldr", addr,                   1);
  LOAD_RI_TEST("ldr", addr + 1 + sizeof(val), -sizeof(val));
  LOAD_RR_TEST("ldr", addr + 1,               zero);
  LOAD_RR_TEST("ldr", addr,                   one);
  LOAD_RR_TEST("ldr", addr + 1 + sizeof(val), minus_size);
  return success;
}

int test_word_store(void) {
  STORE_TEST_START(32, 1, 0xC0BEBEEF);
  STORE_RI_TEST("str", addr + 1,               0);
  STORE_RI_TEST("str", addr,                   1);
  STORE_RI_TEST("str", addr + 1 + sizeof(val), -sizeof(val));
  STORE_RR_TEST("str", addr + 1,               zero);
  STORE_RR_TEST("str", addr,                   one);
  STORE_RR_TEST("str", addr + 1 + sizeof(val), minus_size);
  return success;
}

/*
 * ldrd/strd only support unaligned access at word boundary by design.
 * Executing ldrd/strd on unaligned addresses at byte boundary will result in
 * alignment fault. On ARCH_ARM kernel, for legacy reasons, the kernel contains
 * a fixup code in the alignment fault handler to emulate unaligned ldrd/strd
 * instructions [1]. The ARCH_ARM64 kernel does not emulate these unaligned
 * ldrd/strd instructions.
 *
 * [1]: http://lxr.free-electrons.com/source/arch/arm/mm/alignment.c#L385
 */
int test_doubleword_load(void) {
  LOAD_TEST_START(64, 4, GG_UINT64_C(0xC0FEBEEFDEADBABE));
  LOAD_RI_TEST("ldrd", addr + 4,               0);
  /* Thumb forces the immediate to be a multiple of 4. */
  LOAD_RI_TEST("ldrd", addr,                   4);
  LOAD_RI_TEST("ldrd", addr + 4 + sizeof(val), -sizeof(val));
# if !defined(__thumb__)
  /* Reg-reg ldrd doesn't exist in Thumb. */
  LOAD_RR_TEST("ldrd", addr + 4,               zero);
  LOAD_RR_TEST("ldrd", addr,                   one * 4);
  LOAD_RR_TEST("ldrd", addr + 4 + sizeof(val), minus_size);
# endif
  return success;
}

int test_doubleword_store(void) {
  STORE_TEST_START(64, 4, GG_UINT64_C(0xC0FEBEEFDEADBABE));
  STORE_RI_TEST("strd", addr + 4,               0);
  /* Thumb forces the immediate to be a multiple of 4. */
  STORE_RI_TEST("strd", addr,                   4);
  STORE_RI_TEST("strd", addr + 4 + sizeof(val), -sizeof(val));
# if !defined(__thumb__)
  /* Reg-reg strd doesn't exist in Thumb. */
  STORE_RR_TEST("strd", addr + 4,               zero);
  STORE_RR_TEST("strd", addr,                   one * 4);
  STORE_RR_TEST("strd", addr + 4 + sizeof(val), minus_size);
#endif
  return success;
}

/*
 * Returns 1 if unaligned load/stores work properly.
 */
int NaClQualifyUnaligned(void) {
  struct sigaction old_sigaction;
  struct sigaction try_sigaction;
  int success = 1;

  try_sigaction.sa_handler = signal_catch;
  sigemptyset(&try_sigaction.sa_mask);
  try_sigaction.sa_flags = 0;

  if (0 != sigaction(SIGBUS, &try_sigaction, &old_sigaction)) {
    NaClLog(LOG_FATAL, "Failed to install handler for SIGBUS.\n");
    return 0;
  }

  if (0 == sigsetjmp(try_state, 1)) {
    /*
     * Note that the following don't test:
     *   - PUSH/POP encodings A2 (load/store single 32-bit value to/from
     *     stack). We can reasonably expect these to work, as well as
     *     assume that they won't actually be emitted by the toolchain
     *     to unaligned stack locations.
     *   - VST{1,2,3,4}, VLD{1,2,3,4} (Advanced SIMD). These
     *     instructions only came into existence for ARMv7 and have
     *     always supported unaligned load/store.
     *
     * Otherwise, all user-mode instructions detailed in the ARM ARM
     * section A3.2.1 "Unaligned data access" are tested. We expect
     * SCTLR.A to observably be 0, which is the default for Linux (and
     * is a system-wide setting).
     *
     * Note that SCTLR.A can be 1 (cause alignment faults when executing
     * unaligned instructions) as long as the kernel emulates the
     * unaligned instruction when trapping (which is very slow). On ARM
     * Linux this is configured through /proc/cpu/alignment.
     *
     * The ldrd/strd tests also execute a byte-aligned 64-bit load/store
     * when the ARM ISA only allows 32-bit aligned ldrd/strd to
     * succeed. The test expects the kernel to emulate the
     * instruction. If NaCl were to run in an environment which didn't
     * emulate this instruction then user code would crash
     * unexpectedly. We could work around non-emulating kernels by
     * adding a NaCl fault handler that does the emulation.
     */
    success &= test_halfword_load();
    success &= test_halfword_store();
    success &= test_word_load();
    success &= test_word_store();
    success &= test_doubleword_load();
    success &= test_doubleword_store();
  } else {
    /* One of the load/store faulted. */
    success = 0;
  }

  if (0 != sigaction(SIGBUS, &old_sigaction, NULL)) {
    NaClLog(LOG_FATAL, "Failed to restore handler for SIGBUS.\n");
    return 0;
  }

  return success;
}
