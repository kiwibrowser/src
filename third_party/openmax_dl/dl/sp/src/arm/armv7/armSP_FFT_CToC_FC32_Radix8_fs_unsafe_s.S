@//
@//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
@//
@//  Use of this source code is governed by a BSD-style license
@//  that can be found in the LICENSE file in the root of the source
@//  tree. An additional intellectual property rights grant can be found
@//  in the file PATENTS.  All contributing project authors may
@//  be found in the AUTHORS file in the root of the source tree.
@//
@//  This is a modification of armSP_FFT_CToC_SC32_Radix8_fs_unsafe_s.S
@//  to support float instead of SC32.
@//

@//
@// Description:
@// Compute a first stage Radix 8 FFT stage for a N point complex signal
@//
@//


@// Include standard headers

#include "dl/api/arm/armCOMM_s.h"
#include "dl/api/arm/omxtypes_s.h"

@//        M_VARIANTS ARM1136JS

@// Import symbols required from other files
@// (For example tables)


@// Set debugging level
@//DEBUG_ON    SETL {TRUE}



@// Guarding implementation by the processor name

@//    IF  ARM1136JS

@//Input Registers

#define pSrc            r0
#define pDst            r2
#define pTwiddle        r1
#define subFFTNum       r6
#define subFFTSize      r7
#define pPingPongBuf    r5


@//Output Registers


@//Local Scratch Registers

#define grpSize         r14
#define step1           r3
#define step2           r8
#define setCount        r14             /*@// Reuse grpSize as setCount*/
#define pointStep       r12

#define t0              r4
@// Real and Imaginary parts

#define x0r             s0
#define x0i             s1
#define x1r             s2
#define x1i             s3
#define x2r             s4
#define x2i             s5
#define x3r             s6
#define x3i             s7
#define t3r             s8              /*@// Temporarily hold x3r and x3i*/
#define t3i             s9
#define t1r             s4
#define t1i             s5
#define sr              s10
#define si              s11
#define roothalf        s12

@// Define macros to load/store two float regs from/to the stack.
        .macro M_VSTM r0, r1, p
        .set    _Offset, _Workspace + \p\()_F
        add     t0, sp, #_Offset
        vstm.f32 t0, {\r0, \r1}
        .endm

        .macro M_VLDM r0, r1, p
        .set    _Offset, _Workspace + \p\()_F
        add     t0, sp, #_Offset
        vldm.f32 t0, {\r0, \r1}
        .endm

@// Define constants

        .macro FFTSTAGE scaled, inverse , name

        @// Define stack arguments


        @// Update grpCount and grpSize rightaway inorder to reuse
        @// pSubFFTSize and pSubFFTNum regs

        mov     subFFTSize, #8
        lsr     grpSize, subFFTNum, #3
        mov     subFFTNum, grpSize


        @// pT0+1 increments pT0 by 8 bytes
        @// pT0+pointStep = increment of 8*pointStep bytes = grpSize bytes
        @// Note: setCount = grpSize/8 (reuse the updated grpSize for
        @// setCount)
        MOV     pointStep,grpSize,LSL #3


        @// Calculate the step of input data for the next set
        MOV     step1,grpSize,LSL #4
        MOV     step2,pointStep,LSL #3
        SUB     step2,step2,pointStep           @// step2 = 7*pointStep


        @// grp = 0 a special case since all the twiddle factors are 1
        @// Loop on the sets

        movw    t0,#0x04f3
        movt    t0,#0x3f35
        vmov.f32 roothalf, t0                   @// roothalf = sqrt(1/2)

grpZeroSetLoop\name:

        vldm.f32 pSrc, {x0r, x0i}               @// x0
        add      pSrc, step1
        vldm.f32 pSrc, {x1r, x1i}               @// x2
        add      pSrc, step1
        vldm.f32 pSrc, {x2r, x2i}               @// x4
        add      pSrc, step1
        vldm.f32 pSrc, {x3r, x3i}               @// x6
        add      pSrc, step1

        SUB     pSrc, pSrc, step2

        @// finish first stage of 8 point FFT and save on stack

        vadd.f32     x0r,x0r,x2r                @// u0
        vadd.f32     x0i,x0i,x2i

        vadd.f32     sr, x2r, x2r
        vadd.f32     si, x2i, x2i
        vsub.f32     x2r,x0r,sr                 @// u1
        vsub.f32     x2i,x0i,si

        M_VSTM   x0r,x0i, pU0
        M_VSTM   x2r,x2i, pU1

        vadd.f32     x1r,x1r,x3r                @// u4
        vadd.f32     x1i,x1i,x3i

        vadd.f32     sr, x3r, x3r
        vadd.f32     si, x3i, x3i
        vsub.f32     x3r,x1r,sr                 @// u5
        vsub.f32     x3i,x1i,si

        M_VSTM   x1r,x1i, pU4
        M_VSTM   x3r,x3i, pU5


        vldm    pSrc, {x0r, x0i}                @// x1
        add     pSrc, step1
        vldm    pSrc, {x1r, x1i}                @// x3
        add     pSrc, step1
        vldm    pSrc, {x2r, x2i}                @// x5
        add     pSrc, step1
        vldm    pSrc, {x3r, x3i}                @// x7
        add     pSrc, #8

        SUB     pSrc, pSrc, step2

        vadd.f32     x0r,x0r,x2r                @// u2
        vadd.f32     x0i,x0i,x2i

        vadd.f32         sr, x2r, x2r
        vadd.f32         si, x2i, x2i
        vsub.f32     x2r,x0r,sr                 @// u3
        vsub.f32     x2i,x0i,si

        M_VSTM   x2r,x2i, pU3

        vadd.f32     x1r,x1r,x3r                @// u6
        vadd.f32     x1i,x1i,x3i

        vadd.f32         sr, x3r, x3r
        vadd.f32         si, x3i, x3i
        vsub.f32     x3r,x1r,sr                 @// u7
        vsub.f32     x3i,x1i,si

        @// finish second and third stage of 8 point FFT

        M_VSTM  x3r,x3i, pU7
        M_VLDM  x2r,x2i, pU0

        @// Decrement setcount
        SUBS    setCount,setCount,#1
        M_VLDM  x3r,x3i, pU4

        vadd.f32     x0r,x0r,x1r                @// v4
        vadd.f32     x0i,x0i,x1i

        vadd.f32     sr, x1r, x1r
        vadd.f32     si, x1i, x1i
        vsub.f32     x1r,x0r,sr                 @// v6
        vsub.f32     x1i,x0i,si

        vadd.f32     x2r,x2r,x3r                @// v0
        vadd.f32     x2i,x2i,x3i

        vadd.f32     sr, x3r, x3r
        vadd.f32     si, x3i, x3i
        vsub.f32     x3r,x2r,sr                 @// v2
        vsub.f32     x3i,x2i,si



        vadd.f32     x2r,x2r,x0r                @// y0
        vadd.f32     x2i,x2i,x0i

        vadd.f32     sr, x0r, x0r
        vadd.f32     si, x0i, x0i
        vsub.f32     x0r,x2r,sr                 @// y4
        vsub.f32     x0i,x2i,si

        vstm    pDst, {x2r, x2i}                @// store y0
        add     pDst, step1

        vadd.f32     x3r,x3r,x1i                @// y6
        vsub.f32     x3i,x3i,x1r

        vadd.f32     sr, x1r, x1r
        vadd.f32     si, x1i, x1i
        vsub.f32     t1r,x3r,si                 @// t1r=x2r reg;t1i=x2i reg
        vadd.f32     t1i,x3i,sr                 @// y2

        .ifeqs  "\inverse", "TRUE"
            vstm        pDst, {t1r, t1i}        @// store y2
            add pDst, step1
            vstm        pDst, {x0r, x0i}        @// store y4
            add pDst, step1
            vstm        pDst, {x3r, x3i}        @// store y6
            add pDst, step1
        .else
            vstm        pDst, {x3r, x3i}        @// store y2
            add pDst, step1
            vstm        pDst, {x0r, x0i}        @// store y4
            add pDst, step1
            vstm        pDst, {t1r, t1i}        @// store y6
            add pDst, step1
        .endif

        SUB     pDst, pDst, step2               @// set pDst to y1


        M_VLDM  x0r,x0i,pU1                     @// Load u1,u3,u5,u7
        M_VLDM  x1r,x1i,pU5
        M_VLDM  x3r,x3i,pU7

        vsub.f32     x0r,x0r,x1i                @// v1
        vadd.f32     x0i,x0i,x1r
        vadd.f32     sr, x1r, x1r
        vadd.f32     si, x1i, x1i
        vadd.f32     t1r,x0r,si                 @// t1r=x2r reg;t1i=x2i reg
        vsub.f32     t1i,x0i,sr                 @// v3

        M_VLDM  x1r,x1i,pU3

        vsub.f32     x1r,x1r,x3i                @// v5
        vadd.f32     x1i,x1i,x3r

        vadd.f32     sr, x3r, x3r
        vadd.f32     si, x3i, x3i
        vadd.f32     t3r,x1r,si                 @// t3i = x3i
        vsub.f32     t3i,x1i,sr                 @// v7

        @// store v5  as (v5.r - v5.i,v5.r + v5.i)
        @// store v7  as (v7.i + v7.r,v7.i - v7.r)

        vadd.f32     x3r,t3i,t3r                @// v7
        vsub.f32     x3i,t3i,t3r

        vsub.f32     x1r,x1r,x1i                @// v5
        vadd.f32     x1i, x1i
        vadd.f32     x1i,x1r,x1i

        vmul.f32  x3r, x3r, roothalf            @// (v7.i + v7.r)*(1/sqrt(2))
        vmul.f32  x3i, x3i, roothalf            @// (v7.i - v7.r)*(1/sqrt(2))
        vmul.f32  x1r, x1r, roothalf            @// (v5.r - v5.i)*(1/sqrt(2))
        vmul.f32  x1i, x1i, roothalf            @// (v5.r + v5.i)*(1/sqrt(2))

        vadd.f32     x2r,x2r,x3r                @// y7
        vadd.f32     x2i,x2i,x3i

        vadd.f32     sr, x3r, x3r
        vadd.f32     si, x3i, x3i
        vsub.f32     x3r,x2r,sr                 @// y3
        vsub.f32     x3i,x2i,si


        vsub.f32     x0r,x0r,x1r                @// y5
        vsub.f32     x0i,x0i,x1i

        vadd.f32     sr, x1r, x1r
        vadd.f32     si, x1i, x1i
        vadd.f32     x1r,x0r,sr                 @// y1
        vadd.f32     x1i,x0i,si

        .ifeqs  "\inverse", "TRUE"
            vstm    pDst, {x1r, x1i}            @// store y1
            add pDst, step1
            vstm    pDst, {x3r, x3i}            @// store y3
            add pDst, step1
            vstm    pDst, {x0r, x0i}            @// store y5
            add pDst, step1
            vstm    pDst, {x2r, x2i}            @// store y7
            add pDst, #8
        .else
            vstm    pDst, {x2r, x2i}            @// store y1
            add pDst, step1
            vstm    pDst, {x0r, x0i}            @// store y3
            add pDst, step1
            vstm    pDst, {x3r, x3i}            @// store y5
            add pDst, step1
            vstm    pDst, {x1r, x1i}            @// store y7
            add pDst, #8
        .endif

        SUB     pDst, pDst, step2               @// update pDst for the next set


        BGT     grpZeroSetLoop\name


        @// reset pSrc to pDst for the next stage
        SUB     pSrc,pDst,pointStep             @// pDst -= 2*grpSize
        mov     pDst, pPingPongBuf


        .endm





        @// Allocate stack memory required by the function

        @// Ensure 8 byte alignment to use M_VLDM
        M_ALLOC8    pU0, 8
        M_ALLOC8    pU1, 8
        M_ALLOC8    pU3, 8
        M_ALLOC8    pU4, 8
        M_ALLOC8    pU5, 8
        M_ALLOC8    pU7, 8

        M_START armSP_FFTFwd_CToC_FC32_Radix8_fs_OutOfPlace_unsafe_vfp,r4
            FFTSTAGE "FALSE","FALSE",FWD
        M_END

        @// Allocate stack memory required by the function

        @// Ensure 8 byte alignment to use M_VLDM
        M_ALLOC8    pU0, 8
        M_ALLOC8    pU1, 8
        M_ALLOC8    pU3, 8
        M_ALLOC8    pU4, 8
        M_ALLOC8    pU5, 8
        M_ALLOC8    pU7, 8

        M_START armSP_FFTInv_CToC_FC32_Radix8_fs_OutOfPlace_unsafe_vfp,r4
            FFTSTAGE "FALSE","TRUE",INV
        M_END

@//    ENDIF        @//ARM1136JS



@// Guarding implementation by the processor name


    .end
