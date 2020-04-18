/*	$OpenBSD: setjmp.h,v 1.5 2005/12/13 00:35:22 millert Exp $	*/
/*	$NetBSD: setjmp.h,v 1.11 1994/12/20 10:35:44 cgd Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)setjmp.h	8.2 (Berkeley) 1/21/94
 */

#ifndef _SETJMP_H_
#define _SETJMP_H_

#include <sys/cdefs.h>

#if defined(__aarch64__)
#define _JBLEN 32
#elif defined(__arm__)
#define _JBLEN 64
#elif defined(__i386__)
#define _JBLEN 10
#elif defined(__mips__)
  #if defined(__LP64__)
  #define _JBLEN 25
  #else
  #define _JBLEN 157
  #endif
#elif defined(__x86_64__)
#define _JBLEN 11
#endif

typedef long sigjmp_buf[_JBLEN + 1];
typedef long jmp_buf[_JBLEN];

#undef _JBLEN

__BEGIN_DECLS

int _setjmp(jmp_buf __env);
void _longjmp(jmp_buf __env, int __value);

int setjmp(jmp_buf __env);
void longjmp(jmp_buf __env, int __value);


#if (defined(__LP64__)) || (defined(__arm__)) || (defined(__mips__) && !defined(__LP64__) && __ANDROID_API__ >= 12) || (defined(__i386__) && __ANDROID_API__ >= 12)
int sigsetjmp(sigjmp_buf __env, int __save_signal_mask)
    __INTRODUCED_IN_ARM(9) __INTRODUCED_IN_MIPS(12) __INTRODUCED_IN_X86(12);
void siglongjmp(sigjmp_buf __env, int __value)
    __INTRODUCED_IN_ARM(9) __INTRODUCED_IN_MIPS(12) __INTRODUCED_IN_X86(12);
#endif /* (defined(__LP64__)) || (defined(__arm__)) || (defined(__mips__) && !defined(__LP64__) && __ANDROID_API__ >= 12) || (defined(__i386__) && __ANDROID_API__ >= 12) */


__END_DECLS

#endif
