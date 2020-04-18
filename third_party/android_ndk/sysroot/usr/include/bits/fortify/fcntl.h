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

#ifndef _FCNTL_H
#error "Never include this file directly; instead, include <fcntl.h>"
#endif


#if __ANDROID_API__ >= 17
int __open_2(const char*, int) __INTRODUCED_IN(17);
int __openat_2(int, const char*, int) __INTRODUCED_IN(17);
#endif /* __ANDROID_API__ >= 17 */

/*
 * These are the easiest way to call the real open even in clang FORTIFY.
 */
int __open_real(const char*, int, ...) __RENAME(open);
int __openat_real(int, const char*, int, ...) __RENAME(openat);

#if defined(__BIONIC_FORTIFY)
#define __open_too_many_args_error "too many arguments"
#define __open_too_few_args_error "called with O_CREAT or O_TMPFILE, but missing mode"
#define __open_useless_modes_warning "has superfluous mode bits; missing O_CREAT?"
/* O_TMPFILE shares bits with O_DIRECTORY. */
#define __open_modes_useful(flags) (((flags) & O_CREAT) || ((flags) & O_TMPFILE) == O_TMPFILE)
#if defined(__clang__)

#if __ANDROID_API__ >= __ANDROID_API_J_MR1__
__BIONIC_ERROR_FUNCTION_VISIBILITY
int open(const char* pathname, int flags, mode_t modes, ...) __overloadable
        __errorattr(__open_too_many_args_error);

/*
 * pass_object_size serves two purposes here, neither of which involve __bos: it
 * disqualifies this function from having its address taken (so &open works),
 * and it makes overload resolution prefer open(const char *, int) over
 * open(const char *, int, ...).
 */
__BIONIC_FORTIFY_INLINE
int open(const char* const __pass_object_size pathname, int flags)
        __overloadable
        __clang_error_if(__open_modes_useful(flags), "'open' " __open_too_few_args_error) {
    return __open_2(pathname, flags);
}

__BIONIC_FORTIFY_INLINE
int open(const char* const __pass_object_size pathname, int flags, mode_t modes)
        __overloadable
        __clang_warning_if(!__open_modes_useful(flags) && modes,
                           "'open' " __open_useless_modes_warning) {
    return __open_real(pathname, flags, modes);
}

__BIONIC_ERROR_FUNCTION_VISIBILITY
int openat(int dirfd, const char* pathname, int flags, mode_t modes, ...)
        __overloadable
        __errorattr(__open_too_many_args_error);

__BIONIC_FORTIFY_INLINE
int openat(int dirfd, const char* const __pass_object_size pathname, int flags)
        __overloadable
        __clang_error_if(__open_modes_useful(flags), "'openat' " __open_too_few_args_error) {
    return __openat_2(dirfd, pathname, flags);
}

__BIONIC_FORTIFY_INLINE
int openat(int dirfd, const char* const __pass_object_size pathname, int flags, mode_t modes)
        __overloadable
        __clang_warning_if(!__open_modes_useful(flags) && modes,
                           "'openat' " __open_useless_modes_warning) {
    return __openat_real(dirfd, pathname, flags, modes);
}
#endif /* __ANDROID_API__ >= __ANDROID_API_J_MR1__ */

#else /* defined(__clang__) */
__errordecl(__creat_missing_mode, __open_too_few_args_error);
__errordecl(__creat_too_many_args, __open_too_many_args_error);

#if __ANDROID_API__ >= __ANDROID_API_J_MR1__
__BIONIC_FORTIFY_INLINE
int open(const char* pathname, int flags, ...) {
    if (__builtin_constant_p(flags)) {
        if (__open_modes_useful(flags) && __builtin_va_arg_pack_len() == 0) {
            __creat_missing_mode();  /* Compile time error. */
        }
    }

    if (__builtin_va_arg_pack_len() > 1) {
        __creat_too_many_args();  /* Compile time error. */
    }

    if ((__builtin_va_arg_pack_len() == 0) && !__builtin_constant_p(flags)) {
        return __open_2(pathname, flags);
    }

    return __open_real(pathname, flags, __builtin_va_arg_pack());
}

__BIONIC_FORTIFY_INLINE
int openat(int dirfd, const char* pathname, int flags, ...) {
    if (__builtin_constant_p(flags)) {
        if (__open_modes_useful(flags) && __builtin_va_arg_pack_len() == 0) {
            __creat_missing_mode();  /* Compile time error. */
        }
    }

    if (__builtin_va_arg_pack_len() > 1) {
        __creat_too_many_args();  /* Compile time error. */
    }

    if ((__builtin_va_arg_pack_len() == 0) && !__builtin_constant_p(flags)) {
        return __openat_2(dirfd, pathname, flags);
    }

    return __openat_real(dirfd, pathname, flags, __builtin_va_arg_pack());
}
#endif /* __ANDROID_API__ >= __ANDROID_API_J_MR1__ */

#endif /* defined(__clang__) */

#undef __open_too_many_args_error
#undef __open_too_few_args_error
#undef __open_useless_modes_warning
#undef __open_modes_useful
#endif /* defined(__BIONIC_FORTIFY) */
