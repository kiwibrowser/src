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


/*
 * portability macros, etc. for string related stuff
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_STRING_H_
#define NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_STRING_H_ 1

#include <string.h>

#include "native_client/src/include/build_config.h"

typedef char utf8char_t;

#if NACL_OSX
/* NOTE:: Mac doesn't define strnlen in the headers. */
#if defined( __cplusplus)
extern "C"
#endif
size_t strnlen(const char* str, size_t max);
#endif

#if NACL_WINDOWS
/* disable warnings for deprecated strncpy */
#pragma warning(disable : 4996)

#define STRDUP _strdup
#define STRTOLL _strtoi64
#define STRTOULL _strtoui64

#else

#define STRDUP strdup
#define STRTOLL strtoll
#define STRTOULL strtoull

#endif

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_STRING_H_ */
