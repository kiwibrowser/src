/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef NDK_ANDROID_SUPPORT_STDLIB_H
#define NDK_ANDROID_SUPPORT_STDLIB_H

#include_next <stdlib.h>

__BEGIN_DECLS

#if __ANDROID_API__ < __ANDROID_API_J__
int posix_memalign(void** memptr, size_t alignment, size_t size);
#endif

#if __ANDROID_API__ < __ANDROID_API_L__
#undef MB_CUR_MAX
size_t __ctype_get_mb_cur_max(void);
#define MB_CUR_MAX __ctype_get_mb_cur_max()
long double strtold_l(const char*, char**, locale_t);
long long strtoll_l(const char*, char**, int, locale_t);
unsigned long long strtoull_l(const char*, char**, int, locale_t);
int mbtowc(wchar_t*, const char*, size_t);
int at_quick_exit(void (*)(void));
void quick_exit(int) __noreturn;
#endif

#if __ANDROID_API__ < __ANDROID_API_O__
double strtod_l(const char*, char**, locale_t);
float strtof_l(const char*, char**, locale_t);
long strtol_l(const char*, char**, int, locale_t);
unsigned long strtoul_l(const char*, char**, int, locale_t);
#endif

__END_DECLS

#endif
