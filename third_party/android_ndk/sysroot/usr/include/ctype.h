/*	$OpenBSD: ctype.h,v 1.19 2005/12/13 00:35:22 millert Exp $	*/
/*	$NetBSD: ctype.h,v 1.14 1994/10/26 00:55:47 cgd Exp $	*/

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ctype.h	5.3 (Berkeley) 4/3/91
 */

#ifndef _CTYPE_H_
#define _CTYPE_H_

#include <sys/cdefs.h>
#include <xlocale.h>

#define _CTYPE_U 0x01
#define _CTYPE_L 0x02
#define _CTYPE_D 0x04
#define _CTYPE_S 0x08
#define _CTYPE_P 0x10
#define _CTYPE_C 0x20
#define _CTYPE_X 0x40
#define _CTYPE_B 0x80
#define _CTYPE_R (_CTYPE_P|_CTYPE_U|_CTYPE_L|_CTYPE_D|_CTYPE_B)
#define _CTYPE_A (_CTYPE_L|_CTYPE_U)

/* _CTYPE_N was added to NDK r10 and is expected by gnu-libstdc++ */
#define _CTYPE_N _CTYPE_D

__BEGIN_DECLS

extern const char* _ctype_;

int isalnum(int __ch);
int isalpha(int __ch);
int isblank(int __ch);
int iscntrl(int __ch);
int isdigit(int __ch);
int isgraph(int __ch);
int islower(int __ch);
int isprint(int __ch);
int ispunct(int __ch);
int isspace(int __ch);
int isupper(int __ch);
int isxdigit(int __ch);
int tolower(int __ch);
int toupper(int __ch);

#if __ANDROID_API__ >= __ANDROID_API_L__
int isalnum_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int isalpha_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int isblank_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int iscntrl_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int isdigit_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int isgraph_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int islower_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int isprint_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int ispunct_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int isspace_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int isupper_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int isxdigit_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int tolower_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
int toupper_l(int __ch, locale_t __l) __INTRODUCED_IN(21);
#else
// Implemented as static inlines before 21.
#endif

int isascii(int __ch);
int toascii(int __ch);

#if __ANDROID_API__ >= 21
int _tolower(int __ch) __INTRODUCED_IN(21);
int _toupper(int __ch) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */


__END_DECLS

#endif
