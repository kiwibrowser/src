@//
@//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
@//
@//  Use of this source code is governed by a BSD-style license
@//  that can be found in the LICENSE file in the root of the source
@//  tree. An additional intellectual property rights grant can be found
@//  in the file PATENTS.  All contributing project authors may
@//  be found in the AUTHORS file in the root of the source tree.
@//
@//  This is a modification of armSP_FFT_CToC_SC32_Radix4_fs_unsafe_s.S
@//  to support float instead of SC32.
@//

@//
@// Description:
@// Compute a first stage Radix 4 FFT stage for a N point complex signal
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
#define pPingPongBuf    r5
#define subFFTNum       r6
#define subFFTSize      r7


@//Output Registers


@//Local Scratch Registers

#define grpSize         r14
#define outPointStep    r12
#define setStep         r3
#define setCount        r14                  /*@// Reuse grpSize as setCount*/
#define pointStep       r12

@// Real and Imaginary parts
#define x0r s0
#define x0i s1
#define x1r s2
#define x1i s3
#define x2r s4
#define x2i s5
#define x3r s6
#define x3i s7
#define t3r s0                 /*@// Temporarily hold x3r and x3i*/
#define t3i s1
#define sr  s8
#define si  s9



        .macro FFTSTAGE scaled, inverse, name

        @// Define stack arguments


        @// Update grpCount and grpSize rightaway inorder to reuse
        @// pSubFFTSize and pSubFFTNum regs
        mov     subFFTSize, #4
        lsr     grpSize, subFFTNum, #2
        mov     subFFTNum, grpSize


        @// pT0+1 increments pT0 by 8 bytes
        @// pT0+pointStep = increment of 8*pointStep bytes = 2*grpSize bytes
        @// Note: outPointStep = pointStep for firststage
        @// Note: setCount = grpSize/4 (reuse the updated grpSize for setCount)
        MOV     pointStep,grpSize,LSL #3


        @// Calculate the step of input data for the next set
        @//MOV     setStep,pointStep,LSL #1
        MOV     setStep,grpSize,LSL #4
        @// setStep = 3*pointStep
        ADD     setStep,setStep,pointStep
        @// setStep = - 3*pointStep+8
        RSB     setStep,setStep,#8

        @// grp = 0 a special case since all the twiddle factors are 1
        @// Loop on the sets

grpZeroSetLoop\name:

        vldm.f32 pSrc, {x0r, x0i}
        add     pSrc, pSrc, pointStep
        vldm.f32 pSrc, {x1r, x1i}
        add     pSrc, pSrc, pointStep
        vldm.f32 pSrc, {x2r, x2i}
        add     pSrc, pSrc, pointStep
        vldm.f32 pSrc, {x3r, x3i}
        add     pSrc, pSrc, setStep


        @// Decrement setcount
        SUBS    setCount,setCount,#1



        @// finish first stage of 4 point FFT

        vadd.f32     x0r,x0r,x2r                @// x0 = x0 + x2
        vadd.f32     x0i,x0i,x2i

        vadd.f32     sr, x2r, x2r
        vadd.f32     si, x2i, x2i
        vsub.f32     x2r,x0r,sr                 @// x2 = x0 - x2
        vsub.f32     x2i,x0i,si

        vadd.f32     x1r,x1r,x3r                @// x1 = x1 + x3
        vadd.f32     x1i,x1i,x3i

        vadd.f32     sr, x3r, x3r
        vadd.f32     si, x3i, x3i
        vsub.f32     x3r,x1r,sr                 @// x3 = x1 - x3
        vsub.f32     x3i,x1i,si


        @// finish second stage of 4 point FFT


        vadd.f32     x0r,x0r,x1r                @// x0 = x0 + x1
        vadd.f32     x0i,x0i,x1i

        vadd.f32     sr, x1r, x1r
        vadd.f32     si, x1i, x1i
        vsub.f32     x1r,x0r,sr                 @// x1 = x0 - x1
        vsub.f32     x1i,x0i,si

        vstm.f32 pDst, {x0r, x0i}
        add      pDst, pDst, outPointStep

        vadd.f32     x2r,x2r,x3i
        vsub.f32     x2i,x2i,x3r

        vadd.f32     sr, x3r, x3r
        vadd.f32     si, x3i, x3i
        vsub.f32     t3r, x2r, si
        vadd.f32     t3i, x2i, sr

        .ifeqs  "\inverse", "TRUE"
            vstm.f32 pDst, {t3r, t3i}
            add      pDst, pDst, outPointStep
            vstm.f32 pDst, {x1r, x1i}
            add      pDst, pDst, outPointStep
            vstm.f32 pDst, {x2r, x2i}
            add      pDst, pDst, setStep
        .else
            vstm.f32 pDst, {x2r, x2i}
            add      pDst, pDst, outPointStep
            vstm.f32 pDst, {x1r, x1i}
            add      pDst, pDst, outPointStep
            vstm.f32 pDst, {t3r, t3i}
            add      pDst, pDst, setStep
        .endif


        BGT     grpZeroSetLoop\name


        @// reset pSrc to pDst for the next stage
        SUB     pSrc,pDst,pointStep             @// pDst -= 2*grpSize
        mov     pDst, pPingPongBuf

        .endm


        M_START armSP_FFTFwd_CToC_FC32_Radix4_fs_OutOfPlace_unsafe_vfp,r4
        FFTSTAGE "FALSE","FALSE",FWD
        M_END


        M_START armSP_FFTInv_CToC_FC32_Radix4_fs_OutOfPlace_unsafe_vfp,r4
        FFTSTAGE "FALSE","TRUE",INV
        M_END


@//    ENDIF                                                           @//ARM1136JS


@// Guarding implementation by the processor name




    .end
