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

#ifndef _STDLIB_H
#define _STDLIB_H

#include <sys/cdefs.h>
#include <xlocale.h>

#include <alloca.h>
#include <malloc.h>
#include <stddef.h>

__BEGIN_DECLS

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

__noreturn void abort(void);
__noreturn void exit(int __status);
#if __ANDROID_API__ >= __ANDROID_API_L__
__noreturn void _Exit(int __status) __INTRODUCED_IN(21);
#else
__noreturn void _Exit(int) __RENAME(_exit);
#endif

int atexit(void (*__fn)(void));


#if __ANDROID_API__ >= 21
int at_quick_exit(void (*__fn)(void)) __INTRODUCED_IN(21);
void quick_exit(int __status) __noreturn __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */


char* getenv(const char* __name);
int putenv(char* __assignment);
int setenv(const char* __name, const char* __value, int __overwrite);
int unsetenv(const char* __name);
int clearenv(void);

char* mkdtemp(char* __template);
char* mktemp(char* __template) __attribute__((deprecated("mktemp is unsafe, use mkstemp or tmpfile instead")));


#if __ANDROID_API__ >= 23
int mkostemp64(char* __template, int __flags) __INTRODUCED_IN(23);
int mkostemp(char* __template, int __flags) __INTRODUCED_IN(23);
int mkostemps64(char* __template, int __suffix_length, int __flags) __INTRODUCED_IN(23);
int mkostemps(char* __template, int __suffix_length, int __flags) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */


#if __ANDROID_API__ >= 21
int mkstemp64(char* __template) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

int mkstemp(char* __template);

#if __ANDROID_API__ >= 23
int mkstemps64(char* __template, int __flags) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */

int mkstemps(char* __template, int __flags);

long strtol(const char* __s, char** __end_ptr, int __base);
long long strtoll(const char* __s, char** __end_ptr, int __base);
unsigned long strtoul(const char* __s, char** __end_ptr, int __base);
unsigned long long strtoull(const char* __s, char** __end_ptr, int __base);


#if __ANDROID_API__ >= 16
int posix_memalign(void** __memptr, size_t __alignment, size_t __size) __INTRODUCED_IN(16);
#endif /* __ANDROID_API__ >= 16 */


double strtod(const char* __s, char** __end_ptr);
long double strtold(const char* __s, char** __end_ptr) __RENAME_LDBL(strtod, 3, 21);


#if __ANDROID_API__ >= 26
unsigned long strtoul_l(const char* __s, char** __end_ptr, int __base, locale_t __l) __INTRODUCED_IN(26);
#endif /* __ANDROID_API__ >= 26 */


int atoi(const char* __s) __attribute_pure__;
long atol(const char* __s) __attribute_pure__;
long long atoll(const char* __s) __attribute_pure__;

char* realpath(const char* __path, char* __resolved);
int system(const char* __command);

void* bsearch(const void* __key, const void* __base, size_t __nmemb, size_t __size, int (*__comparator)(const void* __lhs, const void* __rhs));

void qsort(void* __base, size_t __nmemb, size_t __size, int (*__comparator)(const void* __lhs, const void* __rhs));

uint32_t arc4random(void);
uint32_t arc4random_uniform(uint32_t __upper_bound);
void arc4random_buf(void* __buf, size_t __n);

#define RAND_MAX 0x7fffffff


#if __ANDROID_API__ >= 21
int rand_r(unsigned int* __seed_ptr) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */


double drand48(void);
double erand48(unsigned short __xsubi[3]);
long jrand48(unsigned short __xsubi[3]);

#if __ANDROID_API__ >= 23
void lcong48(unsigned short __param[7]) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */

long lrand48(void);
long mrand48(void);
long nrand48(unsigned short __xsubi[3]);
unsigned short* seed48(unsigned short __seed16v[3]);
void srand48(long __seed);


#if __ANDROID_API__ >= 21
char* initstate(unsigned int __seed, char* __state, size_t __n) __INTRODUCED_IN(21);
char* setstate(char* __state) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */


int getpt(void);

#if __ANDROID_API__ >= 21
int posix_openpt(int __flags) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

char* ptsname(int __fd);
int ptsname_r(int __fd, char* __buf, size_t __n);
int unlockpt(int __fd);


#if __ANDROID_API__ >= 26
int getsubopt(char** __option, char* const* __tokens, char** __value_ptr) __INTRODUCED_IN(26);
#endif /* __ANDROID_API__ >= 26 */


typedef struct {
  int quot;
  int rem;
} div_t;

div_t div(int __numerator, int __denominator) __attribute_const__;

typedef struct {
  long int quot;
  long int rem;
} ldiv_t;

ldiv_t ldiv(long __numerator, long __denominator) __attribute_const__;

typedef struct {
  long long int quot;
  long long int rem;
} lldiv_t;

lldiv_t lldiv(long long __numerator, long long __denominator) __attribute_const__;

/* BSD compatibility. */

#if __ANDROID_API__ >= 21
const char* getprogname(void) __INTRODUCED_IN(21);
void setprogname(const char* __name) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */


int mblen(const char* __s, size_t __n) __INTRODUCED_IN(26) __VERSIONER_NO_GUARD;
size_t mbstowcs(wchar_t* __dst, const char* __src, size_t __n);
int mbtowc(wchar_t* __wc_ptr, const char* __s, size_t __n) __INTRODUCED_IN(21) __VERSIONER_NO_GUARD;
int wctomb(char* __dst, wchar_t __wc) __INTRODUCED_IN(21) __VERSIONER_NO_GUARD;

size_t wcstombs(char* __dst, const wchar_t* __src, size_t __n);

#if __ANDROID_API__ >= __ANDROID_API_L__
size_t __ctype_get_mb_cur_max(void) __INTRODUCED_IN(21);
#define MB_CUR_MAX __ctype_get_mb_cur_max()
#else
/*
 * Pre-L we didn't have any locale support and so we were always the POSIX
 * locale. POSIX specifies that MB_CUR_MAX for the POSIX locale is 1:
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/stdlib.h.html
 */
#define MB_CUR_MAX 1
#endif

#if defined(__BIONIC_INCLUDE_FORTIFY_HEADERS)
#include <bits/fortify/stdlib.h>
#endif

#if __ANDROID_API__ >= __ANDROID_API_L__
float strtof(const char* __s, char** __end_ptr) __INTRODUCED_IN(21);
double atof(const char* __s) __attribute_pure__ __INTRODUCED_IN(21);
int abs(int __x) __attribute_const__ __INTRODUCED_IN(21);
long labs(long __x) __attribute_const__ __INTRODUCED_IN(21);
long long llabs(long long __x) __attribute_const__ __INTRODUCED_IN(21);
int rand(void) __INTRODUCED_IN(21);
void srand(unsigned int __seed) __INTRODUCED_IN(21);
long random(void) __INTRODUCED_IN(21);
void srandom(unsigned int __seed) __INTRODUCED_IN(21);
int grantpt(int __fd) __INTRODUCED_IN(21);

long long strtoll_l(const char* __s, char** __end_ptr, int __base, locale_t __l) __INTRODUCED_IN(21);
unsigned long long strtoull_l(const char* __s, char** __end_ptr, int __base, locale_t __l) __INTRODUCED_IN(21);
long double strtold_l(const char* __s, char** __end_ptr, locale_t __l) __INTRODUCED_IN(21);
#else
// Implemented as static inlines before 21.
#endif

#if __ANDROID_API__ >= __ANDROID_API_O__
double strtod_l(const char* __s, char** __end_ptr, locale_t __l) __INTRODUCED_IN(26);
float strtof_l(const char* __s, char** __end_ptr, locale_t __l) __INTRODUCED_IN(26);
long strtol_l(const char* __s, char** __end_ptr, int, locale_t __l) __INTRODUCED_IN(26);
#else
// Implemented as static inlines before 26.
#endif

__END_DECLS

#include <android/legacy_stdlib_inlines.h>

#endif /* _STDLIB_H */
