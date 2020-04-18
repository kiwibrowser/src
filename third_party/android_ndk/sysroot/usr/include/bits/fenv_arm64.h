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
 * In ARMv8, AArch64 state, floating-point operation is controlled by:
 *
 *  * FPCR - 32Bit Floating-Point Control Register:
 *      * [31:27] - Reserved, Res0;
 *      * [26]    - AHP, Alternative half-precision control bit;
 *      * [25]    - DN, Default NaN mode control bit;
 *      * [24]    - FZ, Flush-to-zero mode control bit;
 *      * [23:22] - RMode, Rounding Mode control field:
 *            * 00  - Round to Nearest (RN) mode;
 *            * 01  - Round towards Plus Infinity (RP) mode;
 *            * 10  - Round towards Minus Infinity (RM) mode;
 *            * 11  - Round towards Zero (RZ) mode.
 *      * [21:20] - Stride, ignored during AArch64 execution;
 *      * [19]    - Reserved, Res0;
 *      * [18:16] - Len, ignored during AArch64 execution;
 *      * [15]    - IDE, Input Denormal exception trap;
 *      * [14:13] - Reserved, Res0;
 *      * [12]    - IXE, Inexact exception trap;
 *      * [11]    - UFE, Underflow exception trap;
 *      * [10]    - OFE, Overflow exception trap;
 *      * [9]     - DZE, Division by Zero exception;
 *      * [8]     - IOE, Invalid Operation exception;
 *      * [7:0]   - Reserved, Res0.
 *
 *  * FPSR - 32Bit Floating-Point Status Register:
 *      * [31]    - N, Negative condition flag for AArch32 (AArch64 sets PSTATE.N);
 *      * [30]    - Z, Zero condition flag for AArch32 (AArch64 sets PSTATE.Z);
 *      * [29]    - C, Carry conditon flag for AArch32 (AArch64 sets PSTATE.C);
 *      * [28]    - V, Overflow conditon flag for AArch32 (AArch64 sets PSTATE.V);
 *      * [27]    - QC, Cumulative saturation bit, Advanced SIMD only;
 *      * [26:8]  - Reserved, Res0;
 *      * [7]     - IDC, Input Denormal cumulative exception;
 *      * [6:5]   - Reserved, Res0;
 *      * [4]     - IXC, Inexact cumulative exception;
 *      * [3]     - UFC, Underflow cumulative exception;
 *      * [2]     - OFC, Overflow cumulative exception;
 *      * [1]     - DZC, Division by Zero cumulative exception;
 *      * [0]     - IOC, Invalid Operation cumulative exception.
 */

#ifndef _BITS_FENV_ARM64_H_
#define _BITS_FENV_ARM64_H_

#include <sys/types.h>

__BEGIN_DECLS

typedef struct {
  __uint32_t __control;     /* FPCR, Floating-point Control Register */
  __uint32_t __status;      /* FPSR, Floating-point Status Register */
} fenv_t;

typedef __uint32_t fexcept_t;

/* Exception flags. */
#define FE_INVALID    0x01
#define FE_DIVBYZERO  0x02
#define FE_OVERFLOW   0x04
#define FE_UNDERFLOW  0x08
#define FE_INEXACT    0x10
#define FE_DENORMAL   0x80
#define FE_ALL_EXCEPT (FE_DIVBYZERO | FE_INEXACT | FE_INVALID | \
                       FE_OVERFLOW | FE_UNDERFLOW | FE_DENORMAL)

/* Rounding modes. */
#define FE_TONEAREST  0x0
#define FE_UPWARD     0x1
#define FE_DOWNWARD   0x2
#define FE_TOWARDZERO 0x3

__END_DECLS

#endif
