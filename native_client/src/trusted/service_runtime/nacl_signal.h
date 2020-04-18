/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SIGNAL_H__
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SIGNAL_H__ 1

/*
 * The nacl_signal module provides a platform independent mechanism for
 * trapping signals encountered while running a Native Client executable.
 * Signal handlers can be installed which will receive a POSIX signal number
 * and a platform dependent signal object.  Accessors are provided to convert
 * to and from architecture dependent CPU state structures.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl/nacl_exception.h"

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  #if NACL_BUILD_SUBARCH == 32
    #include "native_client/src/trusted/service_runtime/arch/x86_32/nacl_signal_32.h"
  #elif NACL_BUILD_SUBARCH == 64
    #include "native_client/src/trusted/service_runtime/arch/x86_64/nacl_signal_64.h"
  #else
    #error "Woe to the service runtime.  Is it running on a 128-bit machine?!?"
  #endif
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  #include "native_client/src/trusted/service_runtime/arch/arm/nacl_signal_arm.h"
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  #include "native_client/src/trusted/service_runtime/arch/mips/nacl_signal_mips.h"
#else
  #error Unknown platform!
#endif


EXTERN_C_BEGIN

struct NaClApp;
struct NaClAppThread;
struct NaClExceptionFrame;

/*
 * TODO(halyavin): These signal numbers are part of an external ABI
 * exposed through NaCl's GDB debug stub.  We need to find a directory
 * to place such headers.
 */
#define NACL_ABI_SIGILL 4
#define NACL_ABI_SIGTRAP 5
#define NACL_ABI_SIGKILL 9
#define NACL_ABI_SIGSEGV 11

#define NACL_ABI_WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define NACL_ABI_WTERMSIG(status) ((status) & 0x7f)
#define NACL_ABI_WIFEXITED(status) (NACL_ABI_WTERMSIG(status) == 0)
#define NACL_ABI_WIFSIGNALED(status) ((((status) + 1) & 0x7f) > 1)
#define NACL_ABI_W_EXITCODE(ret, sig) ((((ret) & 0xff) << 8) + ((sig) & 0x7f))

#if NACL_WINDOWS
enum PosixSignals {
  SIGINT  = 2,
  SIGQUIT = 3,
  SIGILL  = 4,
  SIGTRACE= 5,
  SIGABRT = 6,
  SIGBUS  = 7,
  SIGFPE  = 8,
  SIGKILL = 9,
  SIGSEGV = 11,
  SIGSTKFLT = 16,
};
#endif

#if NACL_LINUX
# define NACL_THREAD_SUSPEND_SIGNAL SIGUSR1
#endif


/*
 * Prototype for a signal handler.  The handler will receive the POSIX
 * signal number and an opaque platform dependent signal object.
 */
typedef void (*NaClSignalHandler)(int sig_num,
                                  const struct NaClSignalContext *regs,
                                  int is_untrusted);


/*
 * This allows setting a larger signal stack size than the default.
 * This is for use by tests which may want to call functions such as
 * fprintf() to print debugging info in the event of a failure,
 * because fprintf() requires a larger stack.
 */
void NaClSignalStackSetSize(uint32_t size);

/*
 * Allocates a stack suitable for passing to
 * NaClSignalStackRegister(), for use as a stack for signal handlers.
 * This can be called in any thread.
 * Stores the result in *result; returns 1 on success, 0 on failure.
 */
int NaClSignalStackAllocate(void **result);

/*
 * Deallocates a stack allocated by NaClSignalStackAllocate().
 * This can be called in any thread.
 */
void NaClSignalStackFree(void *stack);

/*
 * Registers a signal stack for use in the current thread.
 */
void NaClSignalStackRegister(void *stack);

/*
 * Undoes the effect of NaClSignalStackRegister().
 */
void NaClSignalStackUnregister(void);

void NaClSignalTestCrashOnStartup(void);

/*
 * Register process-wide signal handlers.
 */
void NaClSignalHandlerInit(void);

/*
 * Undoes the effect of NaClSignalHandlerInit().
 */
void NaClSignalHandlerFini(void);

/*
 * Provides a signal safe method to write to stderr.
 */
ssize_t NaClSignalErrorMessage(const char *str);

/*
 * Replace the signal handler that is run after NaCl restores %gs on
 * x86-32.  This is only used by test code.
 */
void NaClSignalHandlerSet(NaClSignalHandler func);

/*
 * Fill a signal context structure from the raw platform dependent
 * signal information.
 */
void NaClSignalContextFromHandler(struct NaClSignalContext *sig_ctx,
                                  const void *raw_ctx);

/*
 * Update the raw platform dependent signal information from the
 * signal context structure.
 */
void NaClSignalContextToHandler(void *raw_ctx,
                                const struct NaClSignalContext *sig_ctx);


int NaClSignalContextIsUntrusted(struct NaClAppThread *natp,
                                 const struct NaClSignalContext *sig_ctx);

int NaClSignalCheckSandboxInvariants(const struct NaClSignalContext *regs,
                                     struct NaClAppThread *natp);

/*
 * A basic handler which will exit with -signal_number when
 * a signal is encountered in the untrusted code, otherwise
 * the signal is passed to the next handler.
 */
void NaClSignalHandleUntrusted(int signal_number,
                               const struct NaClSignalContext *regs,
                               int is_untrusted);


void NaClSignalSetUpExceptionFrame(volatile struct NaClExceptionFrame *frame,
                                   const struct NaClSignalContext *regs,
                                   uint32_t context_user_addr);

#if NACL_OSX

# include <mach/thread_status.h>

void NaClSignalContextFromMacThreadState(struct NaClSignalContext *dest,
                                         const x86_thread_state_t *src);
void NaClSignalContextToMacThreadState(x86_thread_state_t *dest,
                                       const struct NaClSignalContext *src);

#endif


EXTERN_C_END

#endif  /* NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SIGNAL_H__ */
