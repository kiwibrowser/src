/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl logging module.
 *
 * This module is used in many parts of Native Client to generate
 * logging output.
 *
 * Example usage:
 *
 *   #include "native_client/src/trusted/platform/nacl_log.h"
 *
 *   NaClLog(3, "Hello %s\n", "world");
 *
 * The 3 is a "detail level" for this particular log message, the
 * "Hello %s\n" is the familiar printf-style format string, and all
 * subsequent arguments are the corresponding arguments.  Type
 * checking for these arguments are done when gcc-compatible compilers
 * are used.
 *
 * When running the program (assuming the main function invoked the
 * module initializer(s) appropriately [see NaClLogModuleInit*
 * below]), you can set the global verbosity level by running the
 * program with the NACLVERBOSITY environment variable, e.g.:
 *
 *   NACLVERBOSITY=3
 *
 * Pre-defined logging detail levels of LOG_INFO, LOG_WARNING,
 * LOG_ERROR, and LOG_FATAL may also be used.  When LOG_FATAL is used,
 * the application aborts after the log message is generated.
 * Messages at these levels cannot be suppressed.
 *
 *
 * The default logging output is standard error.  NB: on Windows, both
 * stdout and stderr are discarded for Windowed applications.
 *
 * All logging output may be redirected to a file using the NACLLOG
 * environment variable, the value of which should be the path to a
 * file.  NOTE: when running in Chrome, the outer sandbox is by
 * default enabled, so the log module will be unable to open the log
 * file.  In order to enable this for testing, use the --no-sandbox
 * flag to Chrome.  (This is not recommended for normal use, since it
 * eliminates a layer of defense.)
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_LOG_H__
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_LOG_H__

#include <stdarg.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"

#ifdef __native_client__
# define ATTRIBUTE_FORMAT_PRINTF(m, n) __attribute__((format(printf, m, n)))
#else
# include "native_client/src/include/portability.h"
#endif

struct Gio;

EXTERN_C_BEGIN

/*
 * PreInit functions may be used to set default module parameter
 * values before the module initializer is called.  This is needed in
 * some cases, such as by users of NaClNrdModuleInit or
 * NaClAllModuleInit, where a list of module initializer is invoked,
 * and the caller wants to crank up logging to get logging output from
 * functions invoked in the module initializers that occur after
 * NaClLogModuleInit (and prior to NaClNrdModuleInit returning).
 */
void NaClLogPreInitSetVerbosity(int verb);
void NaClLogPreInitSetGio(struct Gio *out_stream);

/*
 * NaClLogModuleInit() is idempotent, provided that all the calls to
 * it originate from a single thread.
 */
void NaClLogModuleInit(void);

void NaClLogModuleInitExtended(int        initial_verbosity,
                               struct Gio *log_gio);

/*
 * Convenience functions, in case only one needs to be overridden.
 * Also useful for setting a new default, e.g., invoking
 * NaClLogPreInitSetVerbosity with the maximum of the verbosity level
 * supplied from higher level code such as chrome's command line
 * flags, and the default value from the environment as returned by
 * NaClLogDefaultLogVerbosity().
 */
int NaClLogDefaultLogVerbosity(void);
struct Gio *NaClLogDefaultLogGio(void);

/*
 * Sets the log file to the named file.  Aborts program if the open
 * fails.  A GioFile object is associated with the file.
 *
 * The GioFile object is dynamically allocated, so caller is
 * responsible for obtaining it via NaClLogGetGio and freeing it as
 * appropriate, since otherwise a memory leak will occur.  This
 * includes closing the wrapped FILE *, if appropriate (e.g., not the
 * default of stderr).
 */
void NaClLogSetFile(char const *log_file);

void NaClLogModuleFini(void);

void NaClLogSetVerbosity(int verb);

int NaClLogGetVerbosity(void);

void NaClLogIncrVerbosity(void);

void NaClLogSetGio(struct Gio *out_stream);

struct Gio *NaClLogGetGio(void);

/*
 * Timestamps on log entries may be disabled, e.g., to make it easier to
 * write test that compare logging output.
 */

void NaClLogEnableTimestamp(void);

void NaClLogDisableTimestamp(void);

/*
 * Users of NaClLogV should add ATTRIBUTE_FORMAT_PRINTF(m,n) to their
 * function prototype, where m is the argument position of the format
 * string and n is the position of the first argument to be consumed
 * by the format specifier.
 */
void NaClLogV(int         detail_level,
              char const  *fmt,
              va_list     ap);

void NaClLog(int         detail_level,
             char const  *fmt,
             ...) ATTRIBUTE_FORMAT_PRINTF(2, 3);

#define LOG_INFO    (-1)
#define LOG_WARNING (-2)
#define LOG_ERROR   (-3)
#define LOG_FATAL   (-4)

/*
 * Low-level logging code that requires manual lock manipulation.
 * Should be used carefully!
 */
void NaClLogLock(void);
void NaClLogUnlock(void);

/*
 * Caller is responsible for grabbing the NaClLog mutex lock (via
 * NaClLogLock) before calling NaClLogV_mu or NaClLog_mu and for
 * releasing it (via NaClLogUnlock) afterward.
 *
 * Users of NaClLogV_mu should also use ATTRIBUTE_FORMAT_PRINTF as
 * above.
 *
 * Only the first call to NaClLog_mu or NaClLogV_mu after the
 * NaClLogLock will get a timestamp tag prepended.  No newline
 * detection is done, so a multi-line output must be (1) split into
 * multiple calls to NaClLog_mu or NaClLogV_mu so that newlines are
 * the last character in the format string to the NaClLog_mu or
 * NaClLogV_mu calls, and (2) are followed by NaClLogTagNext_mu() so
 * that the next output will know to generate a tag.
 */
void NaClLogV_mu(int         detail_level,
                 char const  *fmt,
                 va_list     ap);

void NaClLog_mu(int         detail_level,
                char const  *fmt,
                ...) ATTRIBUTE_FORMAT_PRINTF(2, 3);

void NaClLogTagNext_mu(void);

/*
 * The function at |fn| is invoked when NaClLog's family of logging
 * functions are invoked with LOG_FATAL.  This defaults to NaClAbort
 * (see nacl_exit.h), but may be overridden to do other things prior
 * to aborting.  NB: it is not a good idea to depend on too much
 * within |fn|, since the application state is likely
 * inconsistent/unstable.  It is probably a good idea to only use
 * low-level routines or system calls directly.
 */
void NaClLogSetAbortBehavior(void (*fn)(void));

/*
 * Run the current abort behavior hook.
 */
void NaClLogRunAbortBehavior(void);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_LOG_H__ */
