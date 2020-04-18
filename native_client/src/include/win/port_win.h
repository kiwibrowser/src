/*
 * Copyright 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef NATIVE_CLIENT_SRC_INCLUDE_WIN_PORT_WIN_H_
#define NATIVE_CLIENT_SRC_INCLUDE_WIN_PORT_WIN_H_ 1

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_macros.h"

/* TODO: eliminated this file and move its contents to portability*.h */

/* wchar_t and unsigned short are not always equivalent*/
#pragma warning(disable : 4255)
/* padding added after struct member */
#pragma warning(disable: 4820)
/* sign extended conversion */
#pragma warning(disable: 4826)
/* conditional expression is constant */
#pragma warning(disable : 4127)

/* TODO: limit this include to files that really need it */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

/* types missing on Windows */
typedef signed char       __int8_t;
typedef unsigned char     __uint8_t;
typedef short             __int16_t;
typedef unsigned short    __uint16_t;
typedef int               __int32_t;
typedef unsigned int      __uint32_t;
typedef __int64           __int64_t;
typedef unsigned __int64  __uint64_t;

typedef __int8_t          int8_t;
typedef __uint8_t         uint8_t;
typedef __int16_t         int16_t;
typedef __uint16_t        uint16_t;
typedef __int32_t         int32_t;
typedef __uint32_t        uint32_t;
typedef __uint32_t        u_int32_t;
typedef __int64_t         int64_t;
typedef __uint64_t        uint64_t;

typedef long              off_t;
typedef int               mode_t;
typedef long              _off_t;
typedef long int          __loff_t;
typedef unsigned long     DWORD;
typedef long              clock_t;

#ifdef _WIN64
typedef int64_t           ssize_t;
#else
typedef int32_t           ssize_t;
#endif /* _WIN64 */


#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)
/*
 * These are conditionally defined because starting in VS2012 (11.0) stdint.h
 * includes definitions for them.
 *
 * Only including range values actually used in our codebase.
 */
#if _MSC_VER >= 1800
#include <stdint.h>
#else
# if !defined(UINT8_MAX)
#  define UINT8_MAX      NACL_UMAX_VAL(uint8_t)
# endif
# if !defined(INT8_MAX)
#  define INT8_MAX       NACL_MAX_VAL(int8_t)
# endif
# if !defined(INT8_MIN)
#  define INT8_MIN       NACL_MIN_VAL(int8_t)
# endif
# if !defined(UINT16_MAX)
#  define UINT16_MAX     NACL_UMAX_VAL(uint16_t)
# endif
# if !defined(INT16_MAX)
#  define INT16_MAX      NACL_MAX_VAL(int16_t)
# endif
# if !defined(INT16_MIN)
#  define INT16_MIN      NACL_MIN_VAL(int16_t)
# endif
# if !defined(UINT32_MAX)
#  define UINT32_MAX     NACL_UMAX_VAL(uint32_t)
# endif
# if !defined(INT32_MAX)
#  define INT32_MAX      NACL_MAX_VAL(int32_t)
# endif
# if !defined(INT32_MIN)
#  define INT32_MIN      NACL_MIN_VAL(int32_t)
# endif
# if !defined(UINT64_MAX)
#  define UINT64_MAX     NACL_UMAX_VAL(uint64_t)
# endif
# if !defined(INT64_MAX)
#  define INT64_MAX      NACL_MAX_VAL(int64_t)
# endif
# if !defined(INT64_MIN)
#  define INT64_MIN      NACL_MIN_VAL(int64_t)
# endif
#endif
#endif  /* _MSC_VER >= 1800 */

EXTERN_C_BEGIN

/* arguments processing */
int ffs(int x);
int getopt(int argc, char *argv[], const char *optstring);
extern char *optarg;  /* global argument pointer */
extern int optind;   /* global argv index */

EXTERN_C_END


/*************************** stdio.h  ********************/
#ifndef NULL
#if defined(__cplusplus)
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

/************************************************************/


/* from linux/limits.h, via sys/param.h */
#define PATH_MAX 4096

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_WIN_PORT_WIN_H_ */
