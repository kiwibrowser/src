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

#ifndef _SYS_STAT_H_
#error "Never include this file directly; instead, include <sys/stat.h>"
#endif


#if __ANDROID_API__ >= 18
mode_t __umask_chk(mode_t) __INTRODUCED_IN(18);
#endif /* __ANDROID_API__ >= 18 */


#if defined(__BIONIC_FORTIFY)
#define __umask_invalid_mode_str "'umask' called with invalid mode"

#if defined(__clang__)

#if __ANDROID_API__ >= __ANDROID_API_J_MR2__
/* Abuse enable_if to make this an overload of umask. */
__BIONIC_FORTIFY_INLINE
mode_t umask(mode_t mode)
    __overloadable
    __enable_if(1, "")
    __clang_error_if(mode & ~0777, __umask_invalid_mode_str) {
  return __umask_chk(mode);
}
#endif /* __ANDROID_API__ >= __ANDROID_API_J_MR2__ */

#else /* defined(__clang__) */
__errordecl(__umask_invalid_mode, __umask_invalid_mode_str);
extern mode_t __umask_real(mode_t) __RENAME(umask);

#if __ANDROID_API__ >= __ANDROID_API_J_MR2__
__BIONIC_FORTIFY_INLINE
mode_t umask(mode_t mode) {
  if (__builtin_constant_p(mode)) {
    if ((mode & 0777) != mode) {
      __umask_invalid_mode();
    }
    return __umask_real(mode);
  }
  return __umask_chk(mode);
}
#endif /* __ANDROID_API__ >= __ANDROID_API_J_MR2__ */

#endif /* defined(__clang__) */
#undef __umask_invalid_mode_str

#endif /* defined(__BIONIC_FORTIFY) */
