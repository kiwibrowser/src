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

#ifndef _STRING_H
#error "Never include this file directly; instead, include <string.h>"
#endif


#if __ANDROID_API__ >= 23
void* __memchr_chk(const void*, int, size_t, size_t) __INTRODUCED_IN(23);
void* __memrchr_chk(const void*, int, size_t, size_t) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */


#if __ANDROID_API__ >= 21
char* __stpncpy_chk2(char*, const char*, size_t, size_t, size_t) __INTRODUCED_IN(21);
char* __strncpy_chk2(char*, const char*, size_t, size_t, size_t) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */


#if __ANDROID_API__ >= 17
size_t __strlcpy_chk(char*, const char*, size_t, size_t) __INTRODUCED_IN(17);
size_t __strlcat_chk(char*, const char*, size_t, size_t) __INTRODUCED_IN(17);
#endif /* __ANDROID_API__ >= 17 */


#if defined(__BIONIC_FORTIFY)
extern void* __memrchr_real(const void*, int, size_t) __RENAME(memrchr);

// These can share their implementation between gcc and clang with minimal
// trickery...
#if __ANDROID_API__ >= __ANDROID_API_J_MR1__
__BIONIC_FORTIFY_INLINE
void* memcpy(void* const dst __pass_object_size0, const void* src, size_t copy_amount)
        __overloadable
        __clang_error_if(__bos0(dst) != __BIONIC_FORTIFY_UNKNOWN_SIZE && __bos0(dst) < copy_amount,
                         "'memcpy' called with size bigger than buffer") {
    return __builtin___memcpy_chk(dst, src, copy_amount, __bos0(dst));
}

__BIONIC_FORTIFY_INLINE
void* memmove(void* const dst __pass_object_size0, const void* src, size_t len)
        __overloadable
        __clang_error_if(__bos0(dst) != __BIONIC_FORTIFY_UNKNOWN_SIZE && __bos0(dst) < len,
                         "'memmove' called with size bigger than buffer") {
    return __builtin___memmove_chk(dst, src, len, __bos0(dst));
}
#endif /* __ANDROID_API__ >= __ANDROID_API_J_MR1__ */

#if __ANDROID_API__ >= __ANDROID_API_L__
__BIONIC_FORTIFY_INLINE
char* stpcpy(char* const dst __pass_object_size, const char* src)
        __overloadable
        __clang_error_if(__bos(dst) != __BIONIC_FORTIFY_UNKNOWN_SIZE &&
                             __bos(dst) <= __builtin_strlen(src),
                         "'stpcpy' called with string bigger than buffer") {
    return __builtin___stpcpy_chk(dst, src, __bos(dst));
}
#endif /* __ANDROID_API__ >= __ANDROID_API_L__ */

#if __ANDROID_API__ >= __ANDROID_API_J_MR1__
__BIONIC_FORTIFY_INLINE
char* strcpy(char* const dst __pass_object_size, const char* src)
        __overloadable
        __clang_error_if(__bos(dst) != __BIONIC_FORTIFY_UNKNOWN_SIZE &&
                             __bos(dst) <= __builtin_strlen(src),
                         "'strcpy' called with string bigger than buffer") {
    return __builtin___strcpy_chk(dst, src, __bos(dst));
}

__BIONIC_FORTIFY_INLINE
char* strcat(char* const dst __pass_object_size, const char* src) __overloadable {
    return __builtin___strcat_chk(dst, src, __bos(dst));
}

__BIONIC_FORTIFY_INLINE
char* strncat(char* const dst __pass_object_size, const char* src, size_t n) __overloadable {
    return __builtin___strncat_chk(dst, src, n, __bos(dst));
}

__BIONIC_FORTIFY_INLINE
void* memset(void* const s __pass_object_size0, int c, size_t n)
        __overloadable
        __clang_error_if(__bos0(s) != __BIONIC_FORTIFY_UNKNOWN_SIZE && __bos0(s) < n,
                         "'memset' called with size bigger than buffer")
        /* If you're a user who wants this warning to go away: use `(&memset)(foo, bar, baz)`. */
        __clang_warning_if(c && !n, "'memset' will set 0 bytes; maybe the arguments got flipped?") {
    return __builtin___memset_chk(s, c, n, __bos0(s));
}
#endif /* __ANDROID_API__ >= __ANDROID_API_J_MR1__ */


#if defined(__clang__)

#if __ANDROID_API__ >= __ANDROID_API_M__
__BIONIC_FORTIFY_INLINE
void* memchr(const void* const s __pass_object_size, int c, size_t n) __overloadable {
    size_t bos = __bos(s);

    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __builtin_memchr(s, c, n);
    }

    return __memchr_chk(s, c, n, bos);
}

__BIONIC_FORTIFY_INLINE
void* __memrchr_fortify(const void* const __pass_object_size s, int c, size_t n) __overloadable {
    size_t bos = __bos(s);

    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __memrchr_real(s, c, n);
    }

    return __memrchr_chk(s, c, n, bos);
}
#endif /* __ANDROID_API__ >= __ANDROID_API_M__ */

#if __ANDROID_API__ >= __ANDROID_API_L__
__BIONIC_FORTIFY_INLINE
char* stpncpy(char* const dst __pass_object_size, const char* const src __pass_object_size, size_t n)
        __overloadable {
    size_t bos_dst = __bos(dst);
    size_t bos_src = __bos(src);

    /* Ignore dst size checks; they're handled in strncpy_chk */
    if (bos_src == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __builtin___stpncpy_chk(dst, src, n, bos_dst);
    }

    return __stpncpy_chk2(dst, src, n, bos_dst, bos_src);
}

__BIONIC_FORTIFY_INLINE
char* strncpy(char* const dst __pass_object_size, const char* const src __pass_object_size, size_t n)
        __overloadable {
    size_t bos_dst = __bos(dst);
    size_t bos_src = __bos(src);

    /* Ignore dst size checks; they're handled in strncpy_chk */
    if (bos_src == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __builtin___strncpy_chk(dst, src, n, bos_dst);
    }

    return __strncpy_chk2(dst, src, n, bos_dst, bos_src);
}
#endif /* __ANDROID_API__ >= __ANDROID_API_L__ */

#if __ANDROID_API__ >= __ANDROID_API_J_MR1__
__BIONIC_FORTIFY_INLINE
size_t strlcpy(char* const dst __pass_object_size, const char* src, size_t size) __overloadable {
    size_t bos = __bos(dst);

    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __call_bypassing_fortify(strlcpy)(dst, src, size);
    }

    return __strlcpy_chk(dst, src, size, bos);
}

__BIONIC_FORTIFY_INLINE
size_t strlcat(char* const dst __pass_object_size, const char* src, size_t size) __overloadable {
    size_t bos = __bos(dst);

    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __call_bypassing_fortify(strlcat)(dst, src, size);
    }

    return __strlcat_chk(dst, src, size, bos);
}

/*
 * If we can evaluate the size of s at compile-time, just call __builtin_strlen
 * on it directly. This makes it way easier for compilers to fold things like
 * strlen("Foo") into a constant, as users would expect. -1ULL is chosen simply
 * because it's large.
 */
__BIONIC_FORTIFY_INLINE
size_t strlen(const char* const s __pass_object_size)
        __overloadable __enable_if(__builtin_strlen(s) != -1ULL,
                                   "enabled if s is a known good string.") {
    return __builtin_strlen(s);
}

__BIONIC_FORTIFY_INLINE
size_t strlen(const char* const s __pass_object_size0) __overloadable {
    size_t bos = __bos0(s);

    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __builtin_strlen(s);
    }

    return __strlen_chk(s, bos);
}
#endif /* __ANDROID_API__ >= __ANDROID_API_J_MR1__ */

#if  __ANDROID_API__ >= __ANDROID_API_J_MR2__
__BIONIC_FORTIFY_INLINE
char* strchr(const char* const s __pass_object_size, int c) __overloadable {
    size_t bos = __bos(s);

    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __builtin_strchr(s, c);
    }

    return __strchr_chk(s, c, bos);
}

__BIONIC_FORTIFY_INLINE
char* strrchr(const char* const s __pass_object_size, int c) __overloadable {
    size_t bos = __bos(s);

    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __builtin_strrchr(s, c);
    }

    return __strrchr_chk(s, c, bos);
}
#endif /* __ANDROID_API__ >= __ANDROID_API_J_MR2__ */

#else // defined(__clang__)
extern char* __strncpy_real(char*, const char*, size_t) __RENAME(strncpy);
extern size_t __strlcpy_real(char*, const char*, size_t)
    __RENAME(strlcpy);
extern size_t __strlcat_real(char*, const char*, size_t)
    __RENAME(strlcat);

__errordecl(__memchr_buf_size_error, "memchr called with size bigger than buffer");
__errordecl(__memrchr_buf_size_error, "memrchr called with size bigger than buffer");

#if __ANDROID_API__ >= __ANDROID_API_M__
__BIONIC_FORTIFY_INLINE
void* memchr(const void* s __pass_object_size, int c, size_t n) {
    size_t bos = __bos(s);

    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __builtin_memchr(s, c, n);
    }

    if (__builtin_constant_p(n) && (n > bos)) {
        __memchr_buf_size_error();
    }

    if (__builtin_constant_p(n) && (n <= bos)) {
        return __builtin_memchr(s, c, n);
    }

    return __memchr_chk(s, c, n, bos);
}

__BIONIC_FORTIFY_INLINE
void* __memrchr_fortify(const void* s, int c, size_t n) {
    size_t bos = __bos(s);

    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __memrchr_real(s, c, n);
    }

    if (__builtin_constant_p(n) && (n > bos)) {
        __memrchr_buf_size_error();
    }

    if (__builtin_constant_p(n) && (n <= bos)) {
        return __memrchr_real(s, c, n);
    }

    return __memrchr_chk(s, c, n, bos);
}
#endif /* __ANDROID_API__ >= __ANDROID_API_M__ */

#if __ANDROID_API__ >= __ANDROID_API_L__
__BIONIC_FORTIFY_INLINE
char* stpncpy(char* dst, const char* src, size_t n) {
    size_t bos_dst = __bos(dst);
    size_t bos_src = __bos(src);

    if (bos_src == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __builtin___stpncpy_chk(dst, src, n, bos_dst);
    }

    if (__builtin_constant_p(n) && (n <= bos_src)) {
        return __builtin___stpncpy_chk(dst, src, n, bos_dst);
    }

    size_t slen = __builtin_strlen(src);
    if (__builtin_constant_p(slen)) {
        return __builtin___stpncpy_chk(dst, src, n, bos_dst);
    }

    return __stpncpy_chk2(dst, src, n, bos_dst, bos_src);
}

__BIONIC_FORTIFY_INLINE
char* strncpy(char* dst, const char* src, size_t n) {
    size_t bos_dst = __bos(dst);
    size_t bos_src = __bos(src);

    if (bos_src == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __strncpy_real(dst, src, n);
    }

    if (__builtin_constant_p(n) && (n <= bos_src)) {
        return __builtin___strncpy_chk(dst, src, n, bos_dst);
    }

    size_t slen = __builtin_strlen(src);
    if (__builtin_constant_p(slen)) {
        return __builtin___strncpy_chk(dst, src, n, bos_dst);
    }

    return __strncpy_chk2(dst, src, n, bos_dst, bos_src);
}
#endif /* __ANDROID_API__ >= __ANDROID_API_L__ */

#if __ANDROID_API__ >= __ANDROID_API_J_MR1__
__BIONIC_FORTIFY_INLINE
size_t strlcpy(char* dst __pass_object_size, const char* src, size_t size) {
    size_t bos = __bos(dst);

    // Compiler doesn't know destination size. Don't call __strlcpy_chk
    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __strlcpy_real(dst, src, size);
    }

    // Compiler can prove, at compile time, that the passed in size
    // is always <= the actual object size. Don't call __strlcpy_chk
    if (__builtin_constant_p(size) && (size <= bos)) {
        return __strlcpy_real(dst, src, size);
    }

    return __strlcpy_chk(dst, src, size, bos);
}

__BIONIC_FORTIFY_INLINE
size_t strlcat(char* dst, const char* src, size_t size) {
    size_t bos = __bos(dst);

    // Compiler doesn't know destination size. Don't call __strlcat_chk
    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __strlcat_real(dst, src, size);
    }

    // Compiler can prove, at compile time, that the passed in size
    // is always <= the actual object size. Don't call __strlcat_chk
    if (__builtin_constant_p(size) && (size <= bos)) {
        return __strlcat_real(dst, src, size);
    }

    return __strlcat_chk(dst, src, size, bos);
}

__BIONIC_FORTIFY_INLINE
size_t strlen(const char* s) __overloadable {
    size_t bos = __bos(s);

    // Compiler doesn't know destination size. Don't call __strlen_chk
    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __builtin_strlen(s);
    }

    size_t slen = __builtin_strlen(s);
    if (__builtin_constant_p(slen)) {
        return slen;
    }

    return __strlen_chk(s, bos);
}
#endif /* __ANDROID_API__ >= __ANDROID_API_J_MR1__ */

#if  __ANDROID_API__ >= __ANDROID_API_J_MR2__
__BIONIC_FORTIFY_INLINE
char* strchr(const char* s, int c) {
    size_t bos = __bos(s);

    // Compiler doesn't know destination size. Don't call __strchr_chk
    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __builtin_strchr(s, c);
    }

    size_t slen = __builtin_strlen(s);
    if (__builtin_constant_p(slen) && (slen < bos)) {
        return __builtin_strchr(s, c);
    }

    return __strchr_chk(s, c, bos);
}

__BIONIC_FORTIFY_INLINE
char* strrchr(const char* s, int c) {
    size_t bos = __bos(s);

    // Compiler doesn't know destination size. Don't call __strrchr_chk
    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __builtin_strrchr(s, c);
    }

    size_t slen = __builtin_strlen(s);
    if (__builtin_constant_p(slen) && (slen < bos)) {
        return __builtin_strrchr(s, c);
    }

    return __strrchr_chk(s, c, bos);
}
#endif /* __ANDROID_API__ >= __ANDROID_API_J_MR2__ */
#endif /* defined(__clang__) */

#if __ANDROID_API__ >= __ANDROID_API_M__
#if defined(__cplusplus)
extern "C++" {
__BIONIC_FORTIFY_INLINE
void* memrchr(void* const __pass_object_size s, int c, size_t n) {
    return __memrchr_fortify(s, c, n);
}

__BIONIC_FORTIFY_INLINE
const void* memrchr(const void* const __pass_object_size s, int c, size_t n) {
    return __memrchr_fortify(s, c, n);
}
}
#else
__BIONIC_FORTIFY_INLINE
void* memrchr(const void* const __pass_object_size s, int c, size_t n) __overloadable {
    return __memrchr_fortify(s, c, n);
}
#endif
#endif /* __ANDROID_API__ >= __ANDROID_API_M__ */

#endif /* defined(__BIONIC_FORTIFY) */
