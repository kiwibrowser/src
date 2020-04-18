/*-
 * Copyright (c) 2004-2005 David Schultz <das@FreeBSD.ORG>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/lib/msun/arm/fenv.h,v 1.5 2005/03/16 19:03:45 das Exp $
 */

/*
   Rewritten for Android.
*/

/* MIPS FPU floating point control register bits.
 *
 * 31-25  -> floating point conditions code bits set by FP compare
 *           instructions
 * 24     -> flush denormalized results to zero instead of
 *           causing unimplemented operation exception.
 * 23     -> Condition bit
 * 22     -> In conjunction with FS detects denormalized
 *           operands and replaces them internally with 0.
 * 21     -> In conjunction with FS forces denormalized operands
 *           to the closest normalized value.
 * 20-18  -> reserved (read as 0, write with 0)
 * 17     -> cause bit for unimplemented operation
 * 16     -> cause bit for invalid exception
 * 15     -> cause bit for division by zero exception
 * 14     -> cause bit for overflow exception
 * 13     -> cause bit for underflow exception
 * 12     -> cause bit for inexact exception
 * 11     -> enable exception for invalid exception
 * 10     -> enable exception for division by zero exception
 *  9     -> enable exception for overflow exception
 *  8     -> enable exception for underflow exception
 *  7     -> enable exception for inexact exception
 *  6     -> flag invalid exception
 *  5     -> flag division by zero exception
 *  4     -> flag overflow exception
 *  3     -> flag underflow exception
 *  2     -> flag inexact exception
 *  1-0   -> rounding control
 *
 *
 * Rounding Control:
 * 00 - rounding to nearest (RN)
 * 01 - rounding toward zero (RZ)
 * 10 - rounding (up) toward plus infinity (RP)
 * 11 - rounding (down)toward minus infinity (RM)
 */

#ifndef _BITS_FENV_MIPS_H_
#define _BITS_FENV_MIPS_H_

#include <sys/types.h>

__BEGIN_DECLS

typedef __uint32_t fenv_t;
typedef __uint32_t fexcept_t;

/* Exception flags */
#define FE_INVALID    0x40
#define FE_DIVBYZERO  0x20
#define FE_OVERFLOW   0x10
#define FE_UNDERFLOW  0x08
#define FE_INEXACT    0x04
#define FE_ALL_EXCEPT (FE_DIVBYZERO | FE_INEXACT | \
                       FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW)

/* Rounding modes */
#define FE_TONEAREST  0x0000
#define FE_TOWARDZERO 0x0001
#define FE_UPWARD     0x0002
#define FE_DOWNWARD   0x0003

__END_DECLS

#endif
