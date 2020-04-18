/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/include/elf_constants.h"
#include "native_client/src/include/nacl/nacl_exception.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/nonsfi/linux/irt_signal_handling.h"
#include "native_client/src/nonsfi/linux/linux_sys_private.h"
#include "native_client/src/nonsfi/linux/linux_syscall_defines.h"
#include "native_client/src/nonsfi/linux/linux_syscall_structs.h"
#include "native_client/src/public/linux_syscalls/sys/syscall.h"
#include "native_client/src/public/nonsfi/irt_signal_handling.h"
#include "native_client/src/untrusted/irt/irt.h"

typedef struct compat_sigaltstack {
  uint32_t ss_sp;
  int32_t ss_flags;
  uint32_t ss_size;
} linux_stack_t;

#if defined(__i386__)

/* From linux/arch/x86/include/uapi/asm/sigcontext32.h */
struct sigcontext_ia32 {
  uint16_t gs, __gsh;
  uint16_t fs, __fsh;
  uint16_t es, __esh;
  uint16_t ds, __dsh;
  uint32_t di;
  uint32_t si;
  uint32_t bp;
  uint32_t sp;
  uint32_t bx;
  uint32_t dx;
  uint32_t cx;
  uint32_t ax;
  uint32_t trapno;
  uint32_t err;
  uint32_t ip;
  uint16_t cs, __csh;
  uint32_t flags;
  uint32_t sp_at_signal;
  uint16_t ss, __ssh;
  uint32_t fpstate;
  uint32_t oldmask;
  uint32_t cr2;
};

typedef struct sigcontext_ia32 linux_mcontext_t;

#elif defined(__arm__)

/* From linux/arch/arm/include/uapi/asm/sigcontext.h */
struct sigcontext_arm {
  uint32_t trap_no;
  uint32_t error_code;
  uint32_t oldmask;
  uint32_t arm_r0;
  uint32_t arm_r1;
  uint32_t arm_r2;
  uint32_t arm_r3;
  uint32_t arm_r4;
  uint32_t arm_r5;
  uint32_t arm_r6;
  uint32_t arm_r7;
  uint32_t arm_r8;
  uint32_t arm_r9;
  uint32_t arm_r10;
  uint32_t arm_r11;  /* fp */
  uint32_t arm_r12;  /* ip */
  uint32_t arm_sp;
  uint32_t arm_lr;
  uint32_t arm_pc;
  uint32_t arm_cpsr;
  uint32_t fault_address;
};

typedef struct sigcontext_arm linux_mcontext_t;

#else
#error "unsupported architecture"
#endif

/* From linux/arch/arm/include/asm/ucontext.h */
struct linux_ucontext_t {
  uint32_t uc_flags;
  uint32_t uc_link;
  linux_stack_t uc_stack;
  linux_mcontext_t uc_mcontext;
  linux_sigset_t uc_sigmask;
  /* More data follows which we don't care about. */
};

/*
 * Crash signals to handle.  The differences from SFI NaCl are that
 * NonSFI NaCl does not use NACL_THREAD_SUSPEND_SIGNAL (==SIGUSR1),
 */
static const int kSignals[] = {
  LINUX_SIGSTKFLT,
  LINUX_SIGINT, LINUX_SIGQUIT, LINUX_SIGILL, LINUX_SIGTRAP, LINUX_SIGBUS,
  LINUX_SIGFPE, LINUX_SIGSEGV,
  /* Handle SIGABRT in case someone sends it asynchronously using kill(). */
  LINUX_SIGABRT,
};

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static NaClExceptionHandler g_signal_handler_function_pointer = NULL;
static NaClExceptionHandler g_exception_handler_function_pointer = NULL;
static int g_signal_handler_initialized = 0;
static int g_tgid = 0;
static int g_main_tid;

struct NonSfiExceptionFrame {
  struct NaClExceptionContext context;
  struct NaClExceptionPortableContext portable;
};

static void machine_context_to_register(const linux_mcontext_t *mctx,
                                        NaClUserRegisterState *dest) {
#if defined(__i386__)
#define COPY_REG(A) dest->e##A = mctx->A
  COPY_REG(ax);
  COPY_REG(cx);
  COPY_REG(dx);
  COPY_REG(bx);
  COPY_REG(bp);
  COPY_REG(si);
  COPY_REG(di);
#undef COPY_REG
  dest->stack_ptr = mctx->sp;
  dest->prog_ctr = mctx->ip;
  dest->flags = mctx->flags;
#elif defined(__arm__)
#define COPY_REG(A) dest->A = mctx->arm_##A
  COPY_REG(r0);
  COPY_REG(r1);
  COPY_REG(r2);
  COPY_REG(r3);
  COPY_REG(r4);
  COPY_REG(r5);
  COPY_REG(r6);
  COPY_REG(r7);
  COPY_REG(r8);
  COPY_REG(r9);
  COPY_REG(r10);
  COPY_REG(r11);
  COPY_REG(r12);
#undef COPY_REG
  dest->stack_ptr = mctx->arm_sp;
  dest->lr = mctx->arm_lr;
  dest->prog_ctr = mctx->arm_pc;
  dest->cpsr = mctx->arm_cpsr;
#else
# error Unsupported architecture
#endif
}

static void nonsfi_exception_frame_from_signal_context(
    struct NonSfiExceptionFrame *frame,
    const void *raw_ctx) {
  const struct linux_ucontext_t *uctx = (struct linux_ucontext_t *) raw_ctx;
  const linux_mcontext_t *mctx = &uctx->uc_mcontext;
  frame->context.size = (((uintptr_t) (&frame->portable + 1))
                         - (uintptr_t) &frame->context);
  frame->context.portable_context_offset = ((uintptr_t) &frame->portable
                                            - (uintptr_t) &frame->context);
  frame->context.portable_context_size = sizeof(frame->portable);
  frame->context.regs_size = sizeof(frame->context.regs);

  memset(frame->context.reserved, 0, sizeof(frame->context.reserved));
  machine_context_to_register(mctx, &frame->context.regs);
  frame->portable.prog_ctr = frame->context.regs.prog_ctr;
  frame->portable.stack_ptr = frame->context.regs.stack_ptr;

#if defined(__i386__)
  frame->context.arch = EM_386;
  frame->portable.frame_ptr = frame->context.regs.ebp;
#elif defined(__arm__)
  frame->context.arch = EM_ARM;
  /* R11 is frame pointer in ARM mode, R8 is frame pointer in thumb mode. */
  frame->portable.frame_ptr = frame->context.regs.r11;
#else
# error Unsupported architecture
#endif
}

/* A replacement of sigreturn. It does not restore the signal mask. */
static void __attribute__((noreturn))
nonsfi_restore_context(const linux_mcontext_t *mctx) {

#if defined(__i386__)

#define OFFSET(name) \
  [name] "i" (offsetof(linux_mcontext_t, name))
#define RESTORE_SEGMENT(name) \
  "mov %c[" #name "](%%eax), %%" #name "\n"
#define RESTORE(name) \
  "movl %c[" #name "](%%eax), %%e" #name "\n"

  __asm__ __volatile__(
      /* TODO(lhchavez): Restore floating-point environment */

      /* Restore all segment registers */
      RESTORE_SEGMENT(gs)
      RESTORE_SEGMENT(fs)
      RESTORE_SEGMENT(es)
      RESTORE_SEGMENT(ds)

      /*
       * Restore most of the other registers.
       */
      RESTORE(di)
      RESTORE(si)
      RESTORE(bp)
      RESTORE(bx)

      /*
       * Prepare the last registers. eip *must* be one slot above the original
       * stack, since that is the only way eip and esp can be simultaneously
       * restored. Here, we are using ecx as the pseudo stack pointer, and edx
       * as a scratch register. Once the stack is laid out the way we want it to
       * be, restore edx and eax last.
       */
      "mov %c[sp](%%eax), %%ecx\n"
      "mov %c[ip](%%eax), %%edx\n"
      "mov %%edx, -4(%%ecx)\n"
      "mov %c[flags](%%eax), %%edx\n"
      "mov %%edx, -8(%%ecx)\n"
      "mov %c[cx](%%eax), %%edx\n"
      "mov %%edx, -12(%%ecx)\n"
      RESTORE(dx)
      RESTORE(ax)
      "lea -12(%%ecx), %%esp\n"

      /*
       * Finally pop ecx off the stack, restore the processor flags, and return
       * to simultaneously restore esp and eip.
       */
      "pop %%ecx\n"
      "popf\n"
      "ret\n"
      :
      : "a" (mctx),
        OFFSET(gs),
        OFFSET(fs),
        OFFSET(es),
        OFFSET(ds),
        OFFSET(di),
        OFFSET(si),
        OFFSET(bp),
        OFFSET(sp),
        OFFSET(bx),
        OFFSET(dx),
        OFFSET(cx),
        OFFSET(ax),
        OFFSET(ip),
        OFFSET(flags),
        OFFSET(fpstate)
  );

#undef OFFSET
#undef RESTORE
#undef RESTORE_SEGMENT

#elif defined(__arm__)

#define OFFSET(name) \
  [name] "I" (offsetof(linux_mcontext_t, arm_ ## name) - \
              offsetof(linux_mcontext_t, arm_r0))

  register uint32_t a14 __asm__("r14") = (uint32_t) &mctx->arm_r0;

  __asm__ __volatile__(
      /* Restore flags */
      "ldr r0, [r14, %[cpsr]]\n"
      "msr APSR_nzcvqg, r0\n"

      /* TODO(lhchavez): Restore floating-point environment */

      /*
       * Restore general-purpose registers.
       * This code does not use the simpler 'ldmia r14, {r0-pc}' since using
       * ldmia with either sp or with both lr and pc is deprecated.
       */
      "ldmia r14, {r0-r10}\n"

      /*
       * Copy r11, r12, lr, and pc just before the original sp.
       * r12 will work as a temporary sp. r11 will be the scratch register, and
       * will be restored just before moving sp.
       */
      "ldr r12, [r14, %[sp]]\n"

      "ldr r11, [r14, %[pc]]\n"
      "stmdb r12!, {r11}\n"
      "ldr r11, [r14, %[lr]]\n"
      "stmdb r12!, {r11}\n"
      "ldr r11, [r14, %[r12]]\n"
      "stmdb r12!, {r11}\n"
      "ldr r11, [r14, %[r11]]\n"
      "mov sp, r12\n"

      /*
       * Restore r12, lr, and pc. sp will point to the correct location once
       * we're done.
       */
      "pop {r12, lr}\n"
      "pop {pc}\n"
      :
      : "r" (a14),
        OFFSET(cpsr),
        OFFSET(r11),
        OFFSET(r12),
        OFFSET(sp),
        OFFSET(lr),
        OFFSET(pc)
  );

#undef OFFSET

#else
# error Unsupported architecture
#endif

  /* Should never reach this. */
  __builtin_trap();
}

static __attribute__((noreturn))
void restore_context(void *raw_ctx) {
  const struct linux_ucontext_t *uctx = (struct linux_ucontext_t *) raw_ctx;
  const linux_mcontext_t *mctx = &uctx->uc_mcontext;
  nonsfi_restore_context(mctx);
}

/* Signal handlers, responsible for calling the registered handlers. */
static void exception_catch(int sig, linux_siginfo_t *info, void *uc) {
  if (g_exception_handler_function_pointer) {
    struct NonSfiExceptionFrame exception_frame;
    nonsfi_exception_frame_from_signal_context(&exception_frame, uc);
    g_exception_handler_function_pointer(&exception_frame.context);
  }
  _exit(-sig);
}

static void signal_catch(int sig, linux_siginfo_t *info, void *uc) {
  if (g_signal_handler_function_pointer) {
    struct NonSfiExceptionFrame exception_frame;
    nonsfi_exception_frame_from_signal_context(&exception_frame, uc);
    g_signal_handler_function_pointer(&exception_frame.context);
  }
  restore_context(uc);
}

static void nonsfi_install_exception_handler_locked() {
  struct linux_sigaction sa;
  unsigned int a;

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = exception_catch;
  sa.sa_flags = LINUX_SA_SIGINFO | LINUX_SA_ONSTACK;

  /*
   * Reuse the sigemptyset/sigaddset for the first 32 bits of the
   * sigmask. Works on little endian systems only.
   */
  sigset_t *mask = (sigset_t*)&sa.sa_mask;
  sigemptyset(mask);

  /* Mask all signals we catch to prevent re-entry. */
  for (a = 0; a < NACL_ARRAY_SIZE(kSignals); a++) {
    sigaddset(mask, kSignals[a]);
  }

  /* Install all handlers. */
  for (a = 0; a < NACL_ARRAY_SIZE(kSignals); a++) {
    if (linux_sigaction(kSignals[a], &sa, NULL) != 0)
      abort();
  }
}

static void nonsfi_install_signal_handler_locked() {
  struct linux_sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = signal_catch;

  /*
   * User signal handler can be recursively interrupted to avoid having
   * to allow sigreturn/sigprocmask.
   */
  sa.sa_flags = LINUX_SA_SIGINFO | LINUX_SA_NODEFER | LINUX_SA_RESTART;
  sigset_t *mask = (sigset_t*)&sa.sa_mask;
  sigemptyset(mask);

  /*
   * Install a single handler.  Multiple signals can be multiplexed in
   * userspace.
   */
  if (linux_sigaction(LINUX_SIGUSR1, &sa, NULL) != 0)
    abort();
}

static void nonsfi_initialize_signal_handler_locked() {
  if (g_signal_handler_initialized)
    return;
  pid_t tgid = getpid();
  if (tgid == -1)
    abort();
  pid_t main_tid = syscall(__NR_gettid);
  if (main_tid == -1)
    abort();
  nonsfi_install_exception_handler_locked();
  nonsfi_install_signal_handler_locked();
  g_tgid = tgid;
  g_main_tid = main_tid;
  g_signal_handler_initialized = 1;
}

/*
 * Initialize signal handlers before entering sandbox.
 */
void nonsfi_initialize_signal_handler() {
  if (pthread_mutex_lock(&g_mutex) != 0)
    abort();
  nonsfi_initialize_signal_handler_locked();
  if (pthread_mutex_unlock(&g_mutex) != 0)
    abort();
}

int nacl_exception_get_and_set_handler(NaClExceptionHandler handler,
                                       NaClExceptionHandler *old_handler) {
  if (pthread_mutex_lock(&g_mutex) != 0)
    abort();
  nonsfi_initialize_signal_handler_locked();
  if (old_handler)
    *old_handler = g_exception_handler_function_pointer;
  g_exception_handler_function_pointer = handler;
  if (pthread_mutex_unlock(&g_mutex) != 0)
    abort();
  return 0;
}

int nacl_exception_set_handler(NaClExceptionHandler handler) {
  return nacl_exception_get_and_set_handler(handler, NULL);
}

int nacl_exception_clear_flag(void) {
  /*
   * Unblock signals, useful for unit testing and continuing to
   * process after fatal signal.
   */

  /* Allocate the 8 bytes of signal mask. */
  linux_sigset_t mask;

  /*
   * sigemptyset will only clear first 4 bytes of sigset_t, and
   * compat_sigset_t has 8 bytes, clear with memset.
   */
  memset(&mask, 0, sizeof(mask));

  /*
   * Hack to be able to reuse sigset_t utilities from newlib for the
   * first lower 4 bytes of the signal, works because we are all
   * little endians.
   */
  sigset_t *maskptr = (sigset_t *) &mask;
  sigemptyset(maskptr);
  for (size_t a = 0; a < NACL_ARRAY_SIZE(kSignals); a++) {
    if (sigaddset(maskptr, kSignals[a]) != 0)
      abort();
  }
  if (linux_sigprocmask(LINUX_SIG_UNBLOCK, &mask, NULL) != 0)
    abort();

  return 0;
}

int nacl_exception_set_stack(void *p, size_t s) {
  /* Not implemented yet. */
  return ENOSYS;
}

int nacl_async_signal_set_handler(NaClIrtAsyncSignalHandler handler) {
  if (pthread_mutex_lock(&g_mutex) != 0)
    abort();
  nonsfi_initialize_signal_handler_locked();
  g_signal_handler_function_pointer = handler;
  if (pthread_mutex_unlock(&g_mutex) != 0)
    abort();
  return 0;
}

int nacl_async_signal_send_async_signal(nacl_irt_tid_t tid) {
  if (!g_signal_handler_initialized)
    return ESRCH;
  if (tid == 0)
    tid = g_main_tid;
  if (linux_tgkill(g_tgid, tid, LINUX_SIGUSR1) == -1)
    return errno;
  return 0;
}
