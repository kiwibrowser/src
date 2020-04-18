/*  $OpenBSD: fenv.h,v 1.2 2011/05/25 21:46:49 martynas Exp $ */
/*  $NetBSD: fenv.h,v 1.2.4.1 2011/02/08 16:18:55 bouyer Exp $  */

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FENV_H_
#define _FENV_H_

#include <sys/cdefs.h>

#if defined(__aarch64__)
#include <bits/fenv_arm64.h>
#elif defined(__arm__)
#include <bits/fenv_arm.h>
#elif defined(__i386__)
#include <bits/fenv_x86.h>
#elif defined(__mips__)
#include <bits/fenv_mips.h>
#elif defined(__x86_64__)
#include <bits/fenv_x86_64.h>
#endif

__BEGIN_DECLS

// fenv was always available on x86.
#if __ANDROID_API__ >= __ANDROID_API_L__ || defined(__i386__)
int feclearexcept(int __exceptions) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21) __INTRODUCED_IN_X86(9);
int fegetexceptflag(fexcept_t* __flag_ptr, int __exceptions) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21)
    __INTRODUCED_IN_X86(9);
int feraiseexcept(int __exceptions) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21) __INTRODUCED_IN_X86(9);
int fesetexceptflag(const fexcept_t* __flag_ptr, int __exceptions) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21)
    __INTRODUCED_IN_X86(9);
int fetestexcept(int __exceptions) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21) __INTRODUCED_IN_X86(9);

int fegetround(void) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21) __INTRODUCED_IN_X86(9);
int fesetround(int __rounding_mode) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21) __INTRODUCED_IN_X86(9);

int fegetenv(fenv_t* __env) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21) __INTRODUCED_IN_X86(9);
int feholdexcept(fenv_t* __env) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21) __INTRODUCED_IN_X86(9);
int fesetenv(const fenv_t* __env) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21) __INTRODUCED_IN_X86(9);
int feupdateenv(const fenv_t* __env) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21)
    __INTRODUCED_IN_X86(9);

int feenableexcept(int __exceptions) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21) __INTRODUCED_IN_X86(9);
int fedisableexcept(int __exceptions) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21) __INTRODUCED_IN_X86(9);
int fegetexcept(void) __INTRODUCED_IN_ARM(21) __INTRODUCED_IN_MIPS(21) __INTRODUCED_IN_X86(9);
#else
/* Defined as inlines for pre-21 ARM and MIPS. */
#endif

/*
 * The following constant represents the default floating-point environment
 * (that is, the one installed at program startup) and has type pointer to
 * const-qualified fenv_t.
 *
 * It can be used as an argument to the functions that manage the floating-point
 * environment, namely fesetenv() and feupdateenv().
 */
extern const fenv_t __fe_dfl_env;
#define FE_DFL_ENV  (&__fe_dfl_env)

__END_DECLS

#include <android/legacy_fenv_inlines_arm.h>
#include <android/legacy_fenv_inlines_mips.h>

#endif  /* ! _FENV_H_ */
