/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file should be at the top of the #include group, followed by
 * standard system #include files, then by native client specific
 * includes.
 *
 * TODO(gregoryd): explain why.  (Something to do with windows include
 * files, to be reconstructed.)
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_H_
#define NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_H_ 1

#include <stdlib.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#ifdef __native_client__
#include <bits/wordsize.h>
#else
#include "native_client/src/trusted/service_runtime/include/bits/wordsize.h"
#endif

#include "native_client/src/include/nacl_compiler_annotations.h"

#if NACL_WINDOWS
/* disable warnings for deprecated functions like getenv, etc. */
#pragma warning(disable : 4996)
# include <malloc.h>
/* TODO: eliminate port_win.h */
# include "native_client/src/include/win/port_win.h"
#else
# include <sys/types.h>
# include <stdint.h>
# include <unistd.h>
# include <sys/time.h>
#endif  /*NACL_WINDOWS*/

/*
 * Per C99 7.8.14, define __STDC_CONSTANT_MACROS before including <stdint.h>
 * to get the INTn_C and UINTn_C macros for integer constants.  It's difficult
 * to guarantee any specific ordering of header includes, so it's difficult to
 * guarantee that the INTn_C macros can be defined by including <stdint.h> at
 * any specific point.  Provide GG_INTn_C macros instead.
 */

#define GG_INT8_C(x)    (x)
#define GG_INT16_C(x)   (x)
#define GG_INT32_C(x)   (x)
#define GG_INT64_C(x)   GG_LONGLONG(x)

#define GG_UINT8_C(x)   (x ## U)
#define GG_UINT16_C(x)  (x ## U)
#define GG_UINT32_C(x)  (x ## U)
#define GG_UINT64_C(x)  GG_ULONGLONG(x)

#if NACL_WINDOWS
#define GG_LONGLONG(x) x##I64
#define GG_ULONGLONG(x) x##UI64
#else
#define GG_LONGLONG(x) x##LL
#define GG_ULONGLONG(x) x##ULL
#endif


/**
 * Processor architecture detection. This code was derived from
 * Chromium's build/build_config.h.
 * For more info on what's defined, see:
 * http://msdn.microsoft.com/en-us/library/b0084kay.aspx
 * http://www.agner.org/optimize/calling_conventions.pdf
 * r with gcc, run: "echo | gcc -E -dM -"
 */
#if defined(_M_X64) || defined(__x86_64__)
#define NACL_HOST_WORDSIZE 64
#elif defined(_M_IX86) || defined(__i386__)
#define NACL_HOST_WORDSIZE 32
#elif defined(__ARMEL__)
#define NACL_HOST_WORDSIZE 32
#elif defined(__mips__)
#define NACL_HOST_WORDSIZE 32
#elif defined(__pnacl__)
#define NACL_HOST_WORDSIZE 32
#else
#error Unrecognized host architecture
#endif

#ifndef SIZE_T_MAX
# define SIZE_T_MAX ((size_t) -1)
#endif

/* use uint64_t as largest integral type, assume 8 bit bytes */
#ifndef OFF_T_MIN
# define OFF_T_MIN ((off_t) (((uint64_t) 1) << (8 * sizeof(off_t) - 1)))
#endif
#ifndef OFF_T_MAX
# define OFF_T_MAX ((off_t) ~(((uint64_t) 1) << (8 * sizeof(off_t) - 1)))
#endif


/*
 * printf macros for size_t, in the style of inttypes.h.  this is
 * needed since the windows compiler does not understand %zd
 * etc. 64-bit windows uses 32-bit long and does not include long
 * long
 */
#if NACL_WINDOWS
# if defined(_WIN64)
#  define  NACL___PRIdS_PREFIX "I64"
#  define  NACL___PRIuS_PREFIX "I64"
# else
#  define  NACL___PRIdS_PREFIX
#  define  NACL___PRIuS_PREFIX
# endif
#elif NACL_OSX
# define  NACL___PRIdS_PREFIX "l" /* -pedantic C++ programs w/ xcode */
# define  NACL___PRIuS_PREFIX "l" /* -pedantic C++ programs w/ xcode */
#elif NACL_ANDROID
# define  NACL___PRIdS_PREFIX "z"
# define  NACL___PRIuS_PREFIX "z"
#elif defined(__native_client__)
# define NACL___PRIdS_PREFIX "z"
# define NACL___PRIuS_PREFIX "z"
#elif __WORDSIZE == 64
# define NACL___PRIdS_PREFIX "l"
# define NACL___PRIuS_PREFIX "l"
#else
# define NACL___PRIdS_PREFIX
# define NACL___PRIuS_PREFIX
#endif

#if !defined(NACL_PRIdS)
#define NACL_PRIdS NACL___PRIdS_PREFIX "d"
#endif
#if !defined(NACL_PRIiS)
#define NACL_PRIiS NACL___PRIdS_PREFIX "i"
#endif
#if !defined(NACL_PRIoS)
#define NACL_PRIoS NACL___PRIuS_PREFIX "o"
#endif
#if !defined (NACL_PRIuS)
#define NACL_PRIuS NACL___PRIuS_PREFIX "u"
#endif
#if !defined(NACL_PRIxS)
#define NACL_PRIxS NACL___PRIuS_PREFIX "x"
#endif
#if !defined(NACL_PRIXS)
#define NACL_PRIXS NACL___PRIuS_PREFIX "X"
#endif

/*
 * printf macros for intptr_t and uintptr_t, int{8,16,32,64}
 */
#if NACL_WINDOWS
# if defined(_WIN64)
#  define NACL___PRIPTR_PREFIX "I64"
# else
#  define NACL___PRIPTR_PREFIX "l"
# endif
# define NACL_PRIdPTR NACL___PRIPTR_PREFIX "d"
# define NACL_PRIiPTR NACL___PRIPTR_PREFIX "i"
# define NACL_PRIoPTR NACL___PRIPTR_PREFIX "o"
# define NACL_PRIuPTR NACL___PRIPTR_PREFIX "u"
# define NACL_PRIxPTR NACL___PRIPTR_PREFIX "x"
# define NACL_PRIXPTR NACL___PRIPTR_PREFIX "X"

# define NACL_PRId8  "d"
# define NACL_PRIi8  "i"
# define NACL_PRIo8  "o"
# define NACL_PRIu8  "u"
# define NACL_PRIx8  "x"
# define NACL_PRIX8  "X"

# define NACL_PRId16 "d"
# define NACL_PRIi16 "i"
# define NACL_PRIo16 "o"
# define NACL_PRIu16 "u"
# define NACL_PRIx16 "x"
# define NACL_PRIX16 "X"

# define NACL___PRI32_PREFIX "I32"

# define NACL_PRId32 NACL___PRI32_PREFIX "d"
# define NACL_PRIi32 NACL___PRI32_PREFIX "i"
# define NACL_PRIo32 NACL___PRI32_PREFIX "o"
# define NACL_PRIu32 NACL___PRI32_PREFIX "u"
# define NACL_PRIx32 NACL___PRI32_PREFIX "x"
# define NACL_PRIX32 NACL___PRI32_PREFIX "X"

# define NACL___PRI64_PREFIX "I64"

#if !defined(NACL_PRId64)
# define NACL_PRId64 NACL___PRI64_PREFIX "d"
#endif
#if !defined(NACL_PRIi64)
# define NACL_PRIi64 NACL___PRI64_PREFIX "i"
#endif
#if !defined(NACL_PRIo64)
# define NACL_PRIo64 NACL___PRI64_PREFIX "o"
#endif
#if !defined(NACL_PRIu64)
# define NACL_PRIu64 NACL___PRI64_PREFIX "u"
#endif
#if !defined(NACL_PRIx64)
# define NACL_PRIx64 NACL___PRI64_PREFIX "x"
#endif
#if !defined(NACL_PRIX64)
# define NACL_PRIX64 NACL___PRI64_PREFIX "X"
#endif

#else  /* NACL_LINUX, NACL_OSX, __native_client__ */

# ifndef __STDC_FORMAT_MACROS
#  define __STDC_FORMAT_MACROS  /* C++ */
# endif

# include <inttypes.h>

# if defined(__native_client__)
#  define NACL_PRIdPTR PRId32
#  define NACL_PRIiPTR PRIi32
#  define NACL_PRIoPTR PRIo32
#  define NACL_PRIuPTR PRIu32
#  define NACL_PRIxPTR PRIx32
#  define NACL_PRIXPTR PRIX32
# else
#  define NACL_PRIdPTR PRIdPTR
#  define NACL_PRIiPTR PRIiPTR
#  define NACL_PRIoPTR PRIoPTR
#  define NACL_PRIuPTR PRIuPTR
#  define NACL_PRIxPTR PRIxPTR
#  define NACL_PRIXPTR PRIXPTR
# endif

# define NACL_PRId64 PRId64
# define NACL_PRIi64 PRIi64
# define NACL_PRIo64 PRIo64
# define NACL_PRIu64 PRIu64
# define NACL_PRIx64 PRIx64
# define NACL_PRIX64 PRIX64

# define NACL_PRId32 PRId32
# define NACL_PRIi32 PRIi32
# define NACL_PRIo32 PRIo32
# define NACL_PRIu32 PRIu32
# define NACL_PRIx32 PRIx32
# define NACL_PRIX32 PRIX32

# define NACL_PRId16 PRId16
# define NACL_PRIi16 PRIi16
# define NACL_PRIo16 PRIo16
# define NACL_PRIu16 PRIu16
# define NACL_PRIx16 PRIx16
# define NACL_PRIX16 PRIX16

# define NACL_PRId8 PRId8
# define NACL_PRIi8 PRIi8
# define NACL_PRIo8 PRIo8
# define NACL_PRIu8 PRIu8
# define NACL_PRIx8 PRIx8
# define NACL_PRIX8 PRIX8

# if NACL_OSX
/*
 * OSX defines "hh" prefix for int8_t etc, but that's not standards
 * compliant -- --std=c++98 -Wall -Werror rejects it.
 */
#  undef NACL_PRId8
#  undef NACL_PRIi8
#  undef NACL_PRIo8
#  undef NACL_PRIu8
#  undef NACL_PRIx8
#  undef NACL_PRIX8
#  define NACL_PRId8  "d"
#  define NACL_PRIi8  "i"
#  define NACL_PRIo8  "o"
#  define NACL_PRIu8  "u"
#  define NACL_PRIx8  "x"
#  define NACL_PRIX8  "X"
# endif  /* NACL_OSX */

#endif  /* NACL_LINUX, NACL_OSX, __native_client__ */

/*
 * macros for run-time error detectors (such as Valgrind/Memcheck).
 */
#if defined(_DEBUG) && NACL_LINUX
#include "native_client/src/third_party/valgrind/memcheck.h"
#define NACL_MAKE_MEM_UNDEFINED(a, b) (void) VALGRIND_MAKE_MEM_UNDEFINED(a, b)
#else
#define NACL_MAKE_MEM_UNDEFINED(a, b)
#endif

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_H_ */
