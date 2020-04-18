/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Simple Fault Injection library.  (No, SFI is already taken!)
 *
 * For testing, inject faults under the control of environment
 * variables.  At call sites where faults may occur, e.g., memory
 * allocation, wrap the call to the function so that rather than
 * calling the function, a dummy bad return value can be provided
 * instead.
 *
 * When fault injection occurs is controlled by the
 * NACL_FAULT_INJECTION environment variable.  Its value is a colon or
 * comma separated list of <site_name>=<fault_control_expression>
 * pairs.  The <site_name> portion is just the call site string name
 * specified in the NACL_FI, NACL_FI_VAL, or NACL_FI_SYSCALL macros
 * below.  The <fault_control_expression> is an expression that
 * controls when to inject the fault and what value(s) to use (in the
 * case of NACL_FI, the value is only under the control of the macro
 * and not the environment variable).  Often it is desirable that the
 * fault does not occur on the first time that the call site is
 * reached.  This is where the <fault_control_expression> comes in.
 * It grammar in BNF is
 *
 * <fault_control_expression> ::= T <pass_or_fail_seq> | G <pass_or_fail_seq>
 * <pass_or_fail_seq> ::= <pass_or_fail_seq> <pass_or_fail> | <pass_or_fail>
 *                        | <pass_or_fail_seq> + <pass_or_fail>
 * <pass_or_fail> ::= P<count> | F[<value>][/<count>]]
 * <count> := <digits>|@
 * <value> := <digits>
 *
 * If a leading literal 'T' occurs, the fault control expression is
 * thread specific -- every thread is started with its own private
 * counters for how many times it has reached the call site, etc;
 * otherwise there must be a leading literal 'G', and the fault
 * control counter for the call site is global, and accessed after
 * acquiring a lock.
 *
 * <pass_or_fail> specifies at what "time" a call to that call site
 * fails.  The <count> value after the literal 'P' specifies how many
 * times the fault injection passes the call through to the real
 * function (note that the real function can also return an error or
 * faulting value), and the optional <value> and optional <count>
 * after the 'F' literal specifies the injected fault value, and how
 * many times the fault occurs (if the call-site is reached again)
 * [defaulting to 1].  If <value> is omitted, it defaults to 0;
 * <count> may be '@', in which case "infinite" or ~(size_t) 0 is
 * used.
 *
 * Once the <fault_control_sequence> is exhausted, the call site will
 * revert to passing through (i.e., the real function will always be
 * called).  The fault injection library does not implicitly add F/@
 * to the <fault_control_sequence>.
 *
 * <pass_or_fail_seq> has an option '+' separator between the
 * <pass_or_fail> entries so that <digits> can include hexadecimal,
 * and since 'F' is a valid hexadecimal digit, we would otherwise
 * consume the failure prefix.
 *
 * TODO: <fault_control_sequence> should really be extended to be more
 * like regular expressions, so that e.g. (P1F/2){3}F-1/2P2 could be
 * used to specify
 *
 * P F(0) F(0) P F(0) F(0) P F(0) F(0) F(-1) F(-1) P P
 *
 * in which case instead of '@' for infinity we should use '*'.  NB:
 * unlike regular expressions which are used for matching a regular
 * set, we want generating expressions which generates exactly one
 * (possibly infinite length) sequence of P or F(fault_value) symbols.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_FAULT_INJECTION_FAULT_INJECTION_H_
#define NATIVE_CLIENT_SRC_TRUSTED_FAULT_INJECTION_FAULT_INJECTION_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

/*
 * Module initializer.  Requires log module to have been initialized.
 *
 * Must be called before going multithreaded.  Idempotent: repeated
 * calls ignored.
 */
void NaClFaultInjectionModuleInit(void);

/*
 * Private functions, to be used only by the macros below.  Not stable
 * ABI; may change.
 */
int NaClFaultInjectionFaultP(char const *site_name);
uintptr_t NaClFaultInjectionValue(void);

/*
 * Function to free per-thread counters.
 *
 * We use pthread_cleanup_push on systems with pthread, but the API
 * cannot rely on this, since Windows has no equivalent cleanup
 * function, so sans deeper integration with threading libraries
 * (e.g. platform), we cannot do the cleanup automatically.
 */
void NaClFaultInjectionPreThreadExitCleanup(void);

/*
 * Private test functions.
 */
void NaClFaultInjectionModuleInternalInit(void);
void NaClFaultInjectionModuleInternalFini(void);


/*
 * NB: macros below may evaluate the site_name (call site name)
 * argument multiply, so beware.  In typical uses this will be a C
 * NUL-terminated string immediate, so it shouldn't be a problem.
 */

/*
 * char *buffer = NACL_FI("io_buffer", malloc(buffer_size), NULL);
 *
 * NACL_FAULT_INJECTION=io_buffer=GP2F ./path/to/sel_ldr -args -- some.nexe
 *
 * The malloc function is called normally twice, then on the third
 * call, rather than actually invoking malloc, NULL is returned to
 * simulate an out-of-memory condition.  If the program continues to
 * run and that call site is reached again, malloc will be invoked
 * normally.
 */

#define NACL_FI(site_name, funcall, error_value) \
  (NaClFaultInjectionFaultP(site_name) ? (error_value) : (funcall))
/*
 * Note that the |error_value| parameter can be more complex, e.g., it
 * can be an expression that invokes a factory function to obtain a
 * special error value.  Similarly, |funcall| can be an arbitrary
 * expression, e.g., the guard to a conditional that leads to a
 * LOG_FATAL.
 */

/*
 * if (NACL_FI_ERROR_COND("test", !NaClSecureServiceCtor(...))) {
 *   error_handling_code();
 * }
 *
 * This is a common case -- the error check guard to error handling
 * code is wrapped with NACL_FI_ERROR_COND to pretend that the guard
 * fired and force the error handling code to execute.
 *
 * NB: does NOT evaluate |expr| when the fault injection machinery
 * triggers.
 */
#define NACL_FI_ERROR_COND(site_name, expr)     \
  (NACL_FI(site_name, expr, 1))

#define NACL_FI_FATAL(site_name)                                      \
  do {                                                                \
    if (NACL_FI(site_name, 0, 1)) {                                   \
      NaClLog(LOG_FATAL, "NaCl Fault Injection: at %s\n", site_name); \
    }                                                                 \
  } while (0)

/*
 * NaClErrorCode = NACL_FI_VAL("load_module", NaClErrorCode,
 *                             NaClAppLoadFile(load_src, nap));
 *
 * NACL_FAULT_INJECTION=load_module=GF2 ./path/to/sel_ldr -args -- some.nexe
 * # error 2 is LOAD_UNSUPPORTED_OS_PLATFORM
 *
 * Since the type is a macro argument, we could inject in any integral
 * values that is the size of uintptr_t or smaller (e.g., pointers are
 * possible), but we cannot construct objects etc.
 *
 * WARNING: if you use NACL_FI_VAL with functions such as malloc, then
 * the fault injection harness, if enabled, could be used to subvert
 * security by, e.g., making an apparent call to malloc return a
 * pointer to user-controlled data regions rather than new memory.
 */

#define NACL_FI_VAL(site_name, type, funcall) \
  (NaClFaultInjectionFaultP(site_name)        \
   ? (type) NaClFaultInjectionValue()         \
   : (funcall))

/*
 * int syscall_ret = NACL_FI_SYSCALL("PlatformLinuxRead3",
 *                                   read(d, ...))
 *
 * NACL_FAULT_INJECTION=PlatformLinuxRead3=GF9 ./path/to/sel_ldr -args \
 *  -- some.nexe
 * # errno 9 on linux is EBADF.
 *
 * This case causes the call to return -1 and sets errno to 9.
 */
#define NACL_FI_SYSCALL(site_name, funcall)          \
  (NaClFaultInjectionFaultP(site_name)               \
   ? ((errno = (int) NaClFaultInjectionValue()), -1) \
   : (funcall))

/*
 * int syscall_ret = NACL_FI_TYPED_SYSCALL("PlatformLinuxMmap5",
 *                                         void *, mmap(d, ...))
 *
 * NACL_FAULT_INJECTION=PlatformLinuxMmap5=GF9 ./path/to/sel_ldr -args \
 *  -- some.nexe
 * # errno 9 on linux is EBADF.
 *
 * This case causes the call to return -1 and sets errno to 9.
 */
#define NACL_FI_TYPED_SYSCALL(site_name, type, funcall)     \
  (NaClFaultInjectionFaultP(site_name)                      \
   ? ((errno = (int) NaClFaultInjectionValue()), (type) -1) \
   : (funcall))

EXTERN_C_END

#endif
