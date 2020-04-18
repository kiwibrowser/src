/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <signal.h>
#include <string.h>
#include <sys/mman.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/include/portability_io.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/load_file.h"
#include "native_client/src/trusted/service_runtime/nacl_all_modules.h"
#include "native_client/src/trusted/service_runtime/nacl_app.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_register.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sys_memory.h"
#include "native_client/src/trusted/service_runtime/thread_suspension_unwind.h"
#include "native_client/tests/common/register_set.h"
#include "native_client/tests/signal_handler_single_step/step_test_common.h"

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
# include "native_client/src/trusted/service_runtime/arch/arm/tramp_arm.h"
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
# include "native_client/src/trusted/service_runtime/arch/mips/tramp_mips.h"
#endif


/*
 * This test case checks that NaCl thread suspension reports the
 * correct untrusted register values for a thread at all points during
 * the context switches between trusted and untrusted code for
 * handling NaCl syscalls.
 *
 * This is similar to step_test_host.c, except that:
 *
 *  1) step_test_host.c merely checks that NaCl's Linux signal handler
 *     can be entered successfully during context switches.  It does
 *     not check register values comprehensively.
 *
 *  2) Unlike step_test_host.c, this test does not cover the initial
 *     context switch to untrusted code that occurs when starting an
 *     untrusted thread.  That is because the register values for the
 *     initial context switch are different from the
 *     more-easily-recorded register values for the system calls made
 *     during this test.
 *
 * On x86, we use single-stepping by setting the x86 trap flag.
 *
 * ARM and MIPS do not support single-stepping in hardware, so instead we set
 * breakpoints on the code ranges we are interested in.
 */

static const int kNumberOfCallsToTest = 5;
static const int kFastPathSyscallsToTest = 2;

static int g_call_count = 0;
static int g_in_untrusted_code = 0;
static int g_context_switch_count = 0;
static struct NaClAppThread *g_natp;
static struct RegsTestShm *g_test_shm;

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
static const int kSteppingSignal = SIGILL;
#else
static const int kSteppingSignal = SIGTRAP;
#endif


#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm || \
    NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips

/*
 * This represents a range of address space that has been patched to
 * overwrite normal instructions with breakpoint instructions.
 */
struct PatchInfo {
  uintptr_t start_addr;
  uintptr_t end_addr;
  void *old_data;
};

static struct PatchInfo g_patches[2];
static unsigned g_patch_count = 0;
/* Address from which the breakpoint has been temporarily removed. */
uint32_t *g_unpatched_addr = NULL;

const int kInstructionBundleSize = 16;

/* Add breakpoints covering the given range of instructions. */
static void AddBreakpoints(struct NaClApp *nap,
                           uintptr_t start, uintptr_t end) {
  size_t size = end - start;
  size_t page_mask = NACL_PAGESIZE - 1;
  uintptr_t page_addr = start & ~page_mask;
  uintptr_t mapping_size = ((end + page_mask) & ~page_mask) - page_addr;
  int rc;
  struct PatchInfo *patch;
  uint32_t *dest;

  CHECK(start % sizeof(uint32_t) == 0);
  CHECK(end % sizeof(uint32_t) == 0);

  rc = mprotect((void *) page_addr, mapping_size,
                PROT_READ | PROT_WRITE | PROT_EXEC);
  CHECK(rc == 0);

  CHECK(g_patch_count < NACL_ARRAY_SIZE(g_patches));
  patch = &g_patches[g_patch_count++];
  patch->start_addr = start;
  patch->end_addr = end;
  patch->old_data = malloc(size);
  CHECK(patch->old_data != NULL);
  memcpy(patch->old_data, (void *) start, size);
  for (dest = (uint32_t *) start; dest < (uint32_t *) end; ) {
    /* In untrusted address space, avoid overwriting any constant pools. */
    if (NaClIsUserAddr(nap, (uintptr_t) dest) &&
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
        (*dest == NACL_HALT_WORD ||
         *dest == NACL_INSTR_ARM_LITERAL_POOL_HEAD)) {
#else
        *dest == NACL_HALT_WORD) {
#endif
      do {
        dest++;
      } while ((uintptr_t) dest % kInstructionBundleSize != 0);
    } else {
      *dest++ = NACL_HALT_WORD;
    }
  }
  __builtin___clear_cache((void *) start, (void *) end);
}

static uint32_t GetOverwrittenInstruction(uintptr_t addr) {
  struct PatchInfo *patch;
  struct PatchInfo *patches_end = &g_patches[g_patch_count];
  for (patch = g_patches; patch < patches_end; patch++) {
    if (patch->start_addr <= addr && addr < patch->end_addr) {
      return *(uint32_t *) (addr - patch->start_addr +
                            (uintptr_t) patch->old_data);
    }
  }
  SignalSafeLogStringLiteral("Address not covered by breakpoint\n");
  _exit(1);
}

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm

/*
 * This is a workaround for qemu-arm: Writing to an address that has
 * been executed doesn't seem to work unless we re-call mprotect() on
 * the address.
 */
static void ResetPagePermissions(uintptr_t addr) {
  int rc = mprotect((void *) (addr & ~(NACL_PAGESIZE - 1)), NACL_PAGESIZE,
                    PROT_READ | PROT_WRITE | PROT_EXEC);
  CHECK(rc == 0);
}

static void TemporarilyRemoveBreakpoint(uintptr_t addr) {
  uint32_t *dest;

  /* Reapply previously-removed patch. */
  if (g_unpatched_addr != NULL) {
    ResetPagePermissions((uintptr_t) g_unpatched_addr);
    CHECK(*g_unpatched_addr
          == GetOverwrittenInstruction((uintptr_t) g_unpatched_addr));
    *g_unpatched_addr = NACL_HALT_WORD;
    __builtin___clear_cache(g_unpatched_addr, &g_unpatched_addr[1]);
  }

  dest = (uint32_t *) addr;
  ResetPagePermissions(addr);
  CHECK(*dest == NACL_HALT_WORD);
  *dest = GetOverwrittenInstruction(addr);
  __builtin___clear_cache(dest, &dest[1]);
  g_unpatched_addr = dest;
}

#else

static int HasDelaySlot(uint32_t opcode) {
  /*
   * jr
   * 0000 00xx xxx0 0000 0000 0000 0000 1000
   *
   * jalr
   * 0000 00xx xxx0 0000 xxxx x000 0000 1001
   *
   * bltz, bgez, bltzl, bgezl, bltzal, bgezal, bltzall, bgezall
   * 0000 01xx xxxx 00xx xxxx xxxx xxxx xxxx
   *
   * j, jal
   * 0000 1xxx xxxx xxxx xxxx xxxx xxxx xxxx
   *
   * beq, bne, beql, bnel
   * 0x01 0xxx xxxx xxxx xxxx xxxx xxxx xxxx
   *
   * blez, bgtz, blezl, bgtzl
   * 0x01 1xxx xxx0 0000 xxxx xxxx xxxx xxxx
   *
   * bc1f, bc1fl, bc1t, bc1tl
   * 0100 0101 000x xxxx xxxx xxxx xxxx xxxx
   *
   */

  return ((opcode & 0xFC1FFFFF) == 0x00000008) ||
         ((opcode & 0xFC1F07FF) == 0x00000009) ||
         ((opcode & 0xFC0C0000) == 0x04000000) ||
         ((opcode & 0xF8000000) == 0x08000000) ||
         ((opcode & 0xB8000000) == 0x10000000) ||
         ((opcode & 0xB81F0000) == 0x18000000) ||
         ((opcode & 0xFFE00000) == 0x45000000);
}

static void TemporarilyRemoveBreakpoint(uintptr_t addr) {
  uint32_t *dest;
  /* MIPS version has to account for instruction in branch delay slot. */
  uint32_t *delay_slot_addr;
  int has_delay_slot;

  if (g_unpatched_addr != NULL) {
    has_delay_slot = HasDelaySlot(*g_unpatched_addr);
    CHECK(*g_unpatched_addr
          == GetOverwrittenInstruction((uintptr_t) g_unpatched_addr));
    *g_unpatched_addr = NACL_HALT_WORD;

    if (has_delay_slot) {
      delay_slot_addr = g_unpatched_addr + 1;
      CHECK(*delay_slot_addr
             == GetOverwrittenInstruction((uintptr_t) delay_slot_addr));
      *delay_slot_addr = NACL_HALT_WORD;
    }
    __builtin___clear_cache(g_unpatched_addr, &g_unpatched_addr[2]);
  }

  dest = (uint32_t *) addr;
  CHECK(*dest == NACL_HALT_WORD);
  *dest = GetOverwrittenInstruction(addr);

  if (HasDelaySlot(*dest)) {
    delay_slot_addr = dest + 1;
    CHECK(*delay_slot_addr == NACL_HALT_WORD);
    *delay_slot_addr = GetOverwrittenInstruction((uintptr_t) delay_slot_addr);
   }

  __builtin___clear_cache(dest, &dest[2]);
  g_unpatched_addr = dest;
}

#endif

static void SetUpBreakpoints(struct NaClApp *nap) {
  AddBreakpoints(nap, (uintptr_t) &NaClSyscallSeg,
                 (uintptr_t) &NaClSyscallSegEnd);
  /* Set up breakpoints on the syscall trampolines and on untrusted code. */
  AddBreakpoints(nap, NACL_TRAMPOLINE_START, nap->static_text_end);
}

#endif


static int32_t TestSyscall(struct NaClAppThread *natp) {
  if (g_call_count == 0) {
    g_natp = natp;
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm || \
    NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
    SetUpBreakpoints(natp->nap);
#else
    SetTrapFlag();
#endif
  }

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  /* Check that the trap flag has not been unset by anything unexpected. */
  CHECK(GetTrapFlag());
#endif

  if (++g_call_count == kNumberOfCallsToTest) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
    /*
     * Unset the trap flag, otherwise, on x86-32, stepping into a
     * system call instruction generates a SIGTRAP that we cannot
     * handle.
     */
    UnsetTrapFlag();
#endif
    NaClReportExitStatus(natp->nap, 0);
    NaClAppThreadTeardown(natp);
  }
  return 0;
}

NACL_DEFINE_SYSCALL_0(TestSyscall)

static void TrapSignalHandler(int signal,
                              const struct NaClSignalContext *context_ptr,
                              int is_untrusted) {
  uint32_t prog_ctr;
  char buf[100];
  int len;
  struct NaClSignalContext *expected_regs = &g_test_shm->expected_regs;
  struct NaClSignalContext context = *context_ptr;

  if (signal != kSteppingSignal) {
    SignalSafeLogStringLiteral("Error: Received unexpected signal\n");
    _exit(1);
  }

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm || \
    NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  /* Remove breakpoint to allow the instruction to be executed. */
  TemporarilyRemoveBreakpoint(context_ptr->prog_ctr);
#endif

  /* Get the prog_ctr value relative to untrusted address space. */
  prog_ctr = (uint32_t) context.prog_ctr;
  /*
   * The trampoline code is not untrusted code because it is fixed by
   * the TCB.  We don't want thread suspension to report prog_ctr as
   * being inside the trampoline code because we are not providing any
   * DWARF unwind info for the trampoline code.  We want the CALL
   * instruction that jumps to a syscall trampoline to appear to be
   * atomic from the point of view of thread suspension: prog_ctr
   * should be reported as either at the CALL or after the CALL.
   *
   * TODO(mseaborn): Move this range check into the non-test part of
   * the thread suspension code.
   */
  if (prog_ctr >= NACL_TRAMPOLINE_START && prog_ctr < NACL_TRAMPOLINE_END) {
    is_untrusted = 0;
  }
  if (g_in_untrusted_code != is_untrusted) {
    g_context_switch_count++;
    g_in_untrusted_code = is_untrusted;
  }

  if (!*(uint32_t *) NaClUserToSys(g_natp->nap,
                                   (uintptr_t) g_test_shm->regs_should_match))
    return;

  len = snprintf(buf, sizeof(buf), "prog_ctr=0x%"NACL_PRIxNACL_REG": ",
                 context.prog_ctr);
  SignalSafeWrite(buf, len);

  if (is_untrusted) {
    SignalSafeLogStringLiteral("Untrusted context\n");
    RegsUnsetNonCalleeSavedRegisters(&context);
    /*
     * Don't compare prog_ctr if we are executing untrusted code.
     * Untrusted code executes a small loop for calling the syscall,
     * so there are multiple values that prog_ctr can have here.
     */
    context.prog_ctr = expected_regs->prog_ctr;
    RegsAssertEqual(&context, expected_regs);
  } else if ((g_natp->suspend_state & NACL_APP_THREAD_TRUSTED) != 0) {
    SignalSafeLogStringLiteral("Trusted (syscall) context\n");
    NaClThreadContextToSignalContext(&g_natp->user, &context);
    RegsAssertEqual(&context, expected_regs);
  } else {
    enum NaClUnwindCase unwind_case = 0;
    const char *str;

    SignalSafeLogStringLiteral("Inside a context switch: ");

    NaClGetRegistersForContextSwitch(g_natp, &context, &unwind_case);

    str = NaClUnwindCaseToString(unwind_case);
    CHECK(str != NULL);
    SignalSafeWrite(str, strlen(str));
    SignalSafeLogStringLiteral("\n");

    RegsAssertEqual(&context, expected_regs);
  }
}

int main(int argc, char **argv) {
  struct NaClApp app;
  uint32_t mmap_addr;
  char arg_string[32];
  char *args[] = {"prog_name", arg_string};

  NaClHandleBootstrapArgs(&argc, &argv);

  NaClAllModulesInit();

  if (argc != 2) {
    NaClLog(LOG_FATAL, "Expected 1 argument: executable filename\n");
  }

  CHECK(NaClAppCtor(&app));
  NACL_REGISTER_SYSCALL(&app, TestSyscall, NACL_sys_test_syscall_1);
  CHECK(NaClAppLoadFileFromFilename(&app, argv[1]) == LOAD_OK);
  NaClAppInitialDescriptorHookup(&app);

  NaClSignalHandlerInit();
  /*
   * Make the signal stack large enough that fprintf() works when
   * RegsAssertEqual() fails.
   */
  NaClSignalStackSetSize(0x100000);
  NaClSignalHandlerSet(TrapSignalHandler);

  /*
   * Allocate some space in untrusted address space.  We pass the
   * address to the guest program so that it can write a register
   * snapshot for us to compare against.
   */
  mmap_addr = NaClSysMmapIntern(
      &app, NULL, sizeof(*g_test_shm),
      NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
      NACL_ABI_MAP_PRIVATE | NACL_ABI_MAP_ANONYMOUS, -1, 0);
  g_test_shm = (struct RegsTestShm *) NaClUserToSys(&app, mmap_addr);
  SNPRINTF(arg_string, sizeof(arg_string), "0x%x", (unsigned int) mmap_addr);

  CHECK(NaClCreateMainThread(&app, 2, args, NULL));
  CHECK(NaClWaitForMainThreadToExit(&app) == 0);

  CHECK(!g_in_untrusted_code);
  ASSERT_EQ(g_context_switch_count,
            (kNumberOfCallsToTest + kFastPathSyscallsToTest - 1) * 2);
  fprintf(stderr, "Finished OK\n");

  /*
   * Avoid calling exit() because it runs process-global destructors
   * which might break code that is running in our unjoined threads.
   */
  NaClExit(0);
  return 0;
}
