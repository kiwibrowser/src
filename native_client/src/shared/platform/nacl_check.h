/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime, check macros.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_CHECK_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_CHECK_H_

#include "native_client/src/shared/platform/nacl_log.h"

EXTERN_C_BEGIN

/*
 * We cannot use variadic macros since not all preprocessors provide
 * them.  Instead, we require uses of the CHECK and DCHECK macro write
 * code in the following manner:
 *
 *   CHECK(a == b);
 *
 * or
 *
 *   VCHECK(a == b, ("a = %d, b = %d, c = %s\n", a, b, some_cstr));
 *
 * depending on whether a printf-like, more detailed message in
 * addition to the invariance failure should be printed.
 *
 * NB: BEWARE of printf arguments the evaluation of which have
 * side-effects.  Any such will cause the program to behave
 * differently depending on whether debug mode is enabled or not.
 */
#define CHECK(bool_expr) do {                                        \
    if (!(bool_expr)) {                                              \
      NaClLog(LOG_FATAL, "Fatal error in file %s, line %d: !(%s)\n", \
              __FILE__, __LINE__, #bool_expr);                       \
    }                                                                \
  } while (0)

#define DCHECK(bool_expr) do {                                       \
    if (nacl_check_debug_mode && !(bool_expr)) {                     \
      NaClLog(LOG_FATAL, "Fatal error in file %s, line %d: !(%s)\n", \
              __FILE__, __LINE__, #bool_expr);                       \
    }                                                                \
  } while (0)

#define VCHECK(bool_expr, fn_arg) do {                               \
    if   (!(bool_expr)) {                                            \
      NaClLog(LOG_ERROR, "Fatal error in file %s, line %d: !(%s)\n", \
              __FILE__, __LINE__, #bool_expr);                       \
      NaClCheckIntern fn_arg;                                        \
    }                                                                \
  } while (0)

#define DVCHECK(bool_expr, fn_arg) do {                              \
    if (nacl_check_debug_mode && !(bool_expr)) {                     \
      NaClLog(LOG_ERROR, "Fatal error in file %s, line %d: !(%s)\n", \
              __FILE__, __LINE__, #bool_expr);                       \
      NaClCheckIntern fn_arg;                                        \
    }                                                                \
  } while (0)

/*
 * By default, nacl_check_debug mode is 0 in opt builds and 1 in dbg
 * builds, so DCHECKs are only performed for dbg builds, though it's
 * possible to change this (viz, as directed by a command line
 * argument) by invoking NaClCheckSetDebugMode.  CHECKs are always
 * performed.
 */
extern void NaClCheckSetDebugMode(int mode);

/*
 * This is a private variable, needed for the macro.  Do not reference
 * directly.
 */
extern int nacl_check_debug_mode;

/*
 * This is a private function, used by the macros above.  Do not
 * reference directly.
 */
extern void NaClCheckIntern(const char *fmt, ...);

EXTERN_C_END

#endif
