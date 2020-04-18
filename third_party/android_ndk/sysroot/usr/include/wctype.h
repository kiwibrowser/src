/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef _WCTYPE_H_
#define _WCTYPE_H_

#include <bits/wctype.h>
#include <sys/cdefs.h>
#include <xlocale.h>

__BEGIN_DECLS

#if __ANDROID_API__ >= __ANDROID_API_L__
int iswalnum_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
int iswalpha_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
int iswblank_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
int iswcntrl_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
int iswdigit_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
int iswgraph_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
int iswlower_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
int iswprint_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
int iswpunct_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
int iswspace_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
int iswupper_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
int iswxdigit_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);

wint_t towlower_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
wint_t towupper_l(wint_t __wc, locale_t __l) __INTRODUCED_IN(21);
#else
// Implemented as static inlines before 21.
#endif


#if __ANDROID_API__ >= 26
wint_t towctrans_l(wint_t __wc, wctrans_t __transform, locale_t __l) __INTRODUCED_IN(26);
wctrans_t wctrans_l(const char* __name, locale_t __l) __INTRODUCED_IN(26);
#endif /* __ANDROID_API__ >= 26 */



#if __ANDROID_API__ >= 21
wctype_t wctype_l(const char* __name, locale_t __l) __INTRODUCED_IN(21);
int iswctype_l(wint_t __wc, wctype_t __transform, locale_t __l) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */


__END_DECLS

#endif
