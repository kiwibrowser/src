/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_OSX_CRASH_FILTER_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_OSX_CRASH_FILTER_H_ 1


#include <mach/mach.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86

/*
 * This function is intended for use by Chromium's embedding of
 * Breakpad crash reporting.  Given the Mach port for a thread in this
 * process that has crashed (and is suspended), this function returns
 * whether the thread crashed inside NaCl untrusted code.  This is
 * used for deciding whether to report the crash.
 */
int NaClMachThreadIsInUntrusted(mach_port_t thread_port);

/*
 * The internal implementation of NaClMachThreadIsInUntrusted. This function
 * may be called by Native Client's own Mach exception handler to determine
 * whether an exception occurred in untrusted code.
 *
 * Determining whether code is untrusted, for the purposes of this function,
 * considers only the executing code based on the code segment (x86-32) or
 * instruction pointer (x86-64). This is sufficient for determining whether
 * the executing code is untrusted. Callers that are concerned that trusted
 * code may have jumped to untrusted code may also want to consider
 * natp->suspend_state before deciding how to proceed.
 */
int NaClMachThreadStateIsInUntrusted(x86_thread_state_t *state,
                                     size_t nacl_thread_index);

#endif  /* NACL_ARCH(NACL_BUILD_ARCH) */

EXTERN_C_END


#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_OSX_CRASH_FILTER_H_ */
