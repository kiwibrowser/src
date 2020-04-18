/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _WCHAR_H_
#define _WCHAR_H_

#include <sys/cdefs.h>
#include <stdio.h>

#include <stdarg.h>
#include <stddef.h>
#include <time.h>
#include <xlocale.h>

#include <bits/mbstate_t.h>
#include <bits/wchar_limits.h>
#include <bits/wctype.h>

__BEGIN_DECLS

wint_t btowc(int __ch);
int fwprintf(FILE* __fp, const wchar_t* __fmt, ...);
int fwscanf(FILE* __fp, const wchar_t* __fmt, ...);
wint_t fgetwc(FILE* __fp);
wchar_t* fgetws(wchar_t* __buf, int __size, FILE* __fp);
wint_t fputwc(wchar_t __wc, FILE* __fp);
int fputws(const wchar_t* __s, FILE* __fp);
int fwide(FILE* __fp, int __mode);
wint_t getwc(FILE* __fp);
wint_t getwchar(void);
int mbsinit(const mbstate_t* __ps);
size_t mbrlen(const char* __s, size_t __n, mbstate_t* __ps);
size_t mbrtowc(wchar_t* __buf, const char* __s, size_t __n, mbstate_t* __ps);
size_t mbsrtowcs(wchar_t* __dst, const char** __src, size_t __dst_n, mbstate_t* __ps);

#if __ANDROID_API__ >= 21
size_t mbsnrtowcs(wchar_t* __dst, const char** __src, size_t __src_n, size_t __dst_n, mbstate_t* __ps) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

wint_t putwc(wchar_t __wc, FILE* __fp);
wint_t putwchar(wchar_t __wc);
int swprintf(wchar_t* __buf, size_t __n, const wchar_t* __fmt, ...);
int swscanf(const wchar_t* __s, const wchar_t* __fmt, ...);
wint_t ungetwc(wint_t __wc, FILE* __fp);
int vfwprintf(FILE* __fp, const wchar_t* __fmt, va_list __args);

#if __ANDROID_API__ >= 21
int vfwscanf(FILE* __fp, const wchar_t* __fmt, va_list __args) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

int vswprintf(wchar_t* __buf, size_t __n, const wchar_t* __fmt, va_list __args);

#if __ANDROID_API__ >= 21
int vswscanf(const wchar_t* __s, const wchar_t* __fmt, va_list __args) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

int vwprintf(const wchar_t* __fmt, va_list __args);

#if __ANDROID_API__ >= 21
int vwscanf(const wchar_t* __fmt, va_list __args) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

wchar_t* wcpcpy(wchar_t* __dst, const wchar_t* __src);
wchar_t* wcpncpy(wchar_t* __dst, const wchar_t* __src, size_t __n);
size_t wcrtomb(char* __buf, wchar_t __wc, mbstate_t* __ps);
int wcscasecmp(const wchar_t* __lhs, const wchar_t* __rhs);

#if __ANDROID_API__ >= 23
int wcscasecmp_l(const wchar_t* __lhs, const wchar_t* __rhs, locale_t __l) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */

wchar_t* wcscat(wchar_t* __dst, const wchar_t* __src);
wchar_t* wcschr(const wchar_t* __s, wchar_t __wc);
int wcscmp(const wchar_t* __lhs, const wchar_t* __rhs);
int wcscoll(const wchar_t* __lhs, const wchar_t* __rhs);
wchar_t* wcscpy(wchar_t* __dst, const wchar_t* __src);
size_t wcscspn(const wchar_t* __s, const wchar_t* __accept);
size_t wcsftime(wchar_t* __buf, size_t __n, const wchar_t* __fmt, const struct tm* __tm);
size_t wcslen(const wchar_t* __s);
int wcsncasecmp(const wchar_t* __lhs, const wchar_t* __rhs, size_t __n);

#if __ANDROID_API__ >= 23
int wcsncasecmp_l(const wchar_t* __lhs, const wchar_t* __rhs, size_t __n, locale_t __l) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */

wchar_t* wcsncat(wchar_t* __dst, const wchar_t* __src, size_t __n);
int wcsncmp(const wchar_t* __lhs, const wchar_t* __rhs, size_t __n);
wchar_t* wcsncpy(wchar_t* __dst, const wchar_t* __src, size_t __n);

#if __ANDROID_API__ >= 21
size_t wcsnrtombs(char* __dst, const wchar_t** __src, size_t __src_n, size_t __dst_n, mbstate_t* __ps) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

wchar_t* wcspbrk(const wchar_t* __s, const wchar_t* __accept);
wchar_t* wcsrchr(const wchar_t* __s, wchar_t __wc);
size_t wcsrtombs(char* __dst, const wchar_t** __src, size_t __dst_n, mbstate_t* __ps);
size_t wcsspn(const wchar_t* __s, const wchar_t* __accept);
wchar_t* wcsstr(const wchar_t* __haystack, const wchar_t* __needle);
double wcstod(const wchar_t* __s, wchar_t** __end_ptr);

#if __ANDROID_API__ >= 21
float wcstof(const wchar_t* __s, wchar_t** __end_ptr) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

wchar_t* wcstok(wchar_t* __s, const wchar_t* __delimiter, wchar_t** __ptr);
long wcstol(const wchar_t* __s, wchar_t** __end_ptr, int __base);

#if __ANDROID_API__ >= 21
long long wcstoll(const wchar_t* __s, wchar_t** __end_ptr, int __base) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

long double wcstold(const wchar_t* __s, wchar_t** __end_ptr) __RENAME_LDBL(wcstod, 3, 21);
unsigned long wcstoul(const wchar_t* __s, wchar_t** __end_ptr, int __base);

#if __ANDROID_API__ >= 21
unsigned long long wcstoull(const wchar_t* __s, wchar_t** __end_ptr, int __base) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

int wcswidth(const wchar_t* __s, size_t __n);
size_t wcsxfrm(wchar_t* __dst, const wchar_t* __src, size_t __n);
int wctob(wint_t __wc);
int wcwidth(wchar_t __wc);
wchar_t* wmemchr(const wchar_t* __src, wchar_t __wc, size_t __n);
int wmemcmp(const wchar_t* __lhs, const wchar_t* __rhs, size_t __n);
wchar_t* wmemcpy(wchar_t* __dst, const wchar_t* __src, size_t __n);
#if defined(__USE_GNU)

#if __ANDROID_API__ >= 23
wchar_t* wmempcpy(wchar_t* __dst, const wchar_t* __src, size_t __n) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */

#endif
wchar_t* wmemmove(wchar_t* __dst, const wchar_t* __src, size_t __n);
wchar_t* wmemset(wchar_t* __dst, wchar_t __wc, size_t __n);
int wprintf(const wchar_t* __fmt, ...);
int wscanf(const wchar_t* __fmt, ...);

#if __ANDROID_API__ >= __ANDROID_API_L__
long long wcstoll_l(const wchar_t* __s, wchar_t** __end_ptr, int __base, locale_t __l) __INTRODUCED_IN(21);
unsigned long long wcstoull_l(const wchar_t* __s, wchar_t** __end_ptr, int __base, locale_t __l) __INTRODUCED_IN(21);
long double wcstold_l(const wchar_t* __s, wchar_t** __end_ptr, locale_t __l) __INTRODUCED_IN(21);

int wcscoll_l(const wchar_t* __lhs, const wchar_t* __rhs, locale_t __l) __attribute_pure__
    __INTRODUCED_IN(21);
size_t wcsxfrm_l(wchar_t* __dst, const wchar_t* __src, size_t __n, locale_t __l) __INTRODUCED_IN(21);
#else
// Implemented as static inlines before 21.
#endif

size_t wcslcat(wchar_t* __dst, const wchar_t* __src, size_t __n);
size_t wcslcpy(wchar_t* __dst, const wchar_t* __src, size_t __n);


#if __ANDROID_API__ >= 23
FILE* open_wmemstream(wchar_t** __ptr, size_t* __size_ptr) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */

wchar_t* wcsdup(const wchar_t* __s);
size_t wcsnlen(const wchar_t* __s, size_t __n);

__END_DECLS

#endif
