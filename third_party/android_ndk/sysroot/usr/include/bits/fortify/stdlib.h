/*
 * Copyright (C) 2017 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _STDLIB_H
#error "Never include this file directly; instead, include <stdlib.h>"
#endif

#if defined(__BIONIC_FORTIFY)
#define __realpath_buf_too_small_str \
    "'realpath' output parameter must be NULL or a pointer to a buffer with >= PATH_MAX bytes"

/* PATH_MAX is unavailable without polluting the namespace, but it's always 4096 on Linux */
#define __PATH_MAX 4096

#if defined(__clang__)
char* realpath(const char* path, char* resolved)
        __clang_error_if(__bos(resolved) != __BIONIC_FORTIFY_UNKNOWN_SIZE &&
                         __bos(resolved) < __PATH_MAX, __realpath_buf_too_small_str)
        __clang_error_if(!path, "'realpath': NULL path is never correct; flipped arguments?");
/* No need for a definition; the only issues we can catch are at compile-time. */

#else /* defined(__clang__) */

char* __realpath_real(const char*, char*) __RENAME(realpath);
__errordecl(__realpath_size_error, __realpath_buf_too_small_str);

__BIONIC_FORTIFY_INLINE
char* realpath(const char* path, char* resolved) {
    size_t bos = __bos(resolved);

    if (bos != __BIONIC_FORTIFY_UNKNOWN_SIZE && bos < __PATH_MAX) {
        __realpath_size_error();
    }

    return __realpath_real(path, resolved);
}

#endif /* defined(__clang__) */

#undef __PATH_MAX
#undef __realpath_buf_too_small_str
#endif /* defined(__BIONIC_FORTIFY) */
