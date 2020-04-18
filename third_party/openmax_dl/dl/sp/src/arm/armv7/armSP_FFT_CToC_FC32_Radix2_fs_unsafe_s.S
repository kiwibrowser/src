@//
@//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
@//
@//  Use of this source code is governed by a BSD-style license
@//  that can be found in the LICENSE file in the root of the source
@//  tree. An additional intellectual property rights grant can be found
@//  in the file PATENTS.  All contributing project authors may
@//  be found in the AUTHORS file in the root of the source tree.
@//
@//  This is a modification of armSP_FFT_CToC_SC32_Radix2_fs_unsafe_s.S
@//  to support float instead of SC32.
@//

@//
@// Description:
@// Compute a Radix 2 FFT stage for a N point complex signal
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

#define pDstBuf         r3                   /*@// Temporarily hold pingpong buffer ptr*/
#define grpSize         r14
#define outPointStep    r12
#define setCount        r14
#define pointStep       r12

@// Real and Imaginary parts
#define x0r s0
#define x0i s1
#define x1r s2
#define x1i s3
#define y1r s4
#define y1i s5
#define y0r s6
#define y0i s7



        .macro FFTSTAGE scaled, inverse, name

        @// Update grpCount and grpSize rightaway inorder to reuse pGrpCount 
	@// and pGrpSize regs

        mov     subFFTSize, #2
        lsr     grpSize, subFFTNum, #1
        mov     subFFTNum, grpSize

        @// pT0+1 increments pT0 by 8 bytes
        @// pT0+pointStep = increment of 8*pointStep bytes = 4*grpSize bytes
        @// Note: outPointStep = pointStep for firststage
        @// Note: setCount = grpSize/2 (reuse the updated grpSize for setCount)
        MOV     pointStep,grpSize,LSL #3



        @// Loop on the sets for grp zero

grpZeroSetLoop\name:

        add      pSrc, pSrc, pointStep
        @// {x1r,x1i} = [pSrc, pointStep]
        vldm.f32 pSrc, {x1r, x1i}
        sub      pSrc, pSrc, pointStep
        vldm.f32 pSrc!, {x0r, x0i}

        SUBS    setCount,setCount,#1            @// decrement the loop counter



        vsub.f32     y1r,x0r,x1r
        vsub.f32     y1i,x0i,x1i

        vadd.f32     y0r,x0r,x1r
        vadd.f32     y0i,x0i,x1i

        add     pDst, pDst, outPointStep
        @// {y1r,y1i} -> [pDst, outPointStep]
        vstm    pDst, {y1r, y1i}
        sub     pDst, pDst, outPointStep
        vstm    pDst!, {y0r, y0i}

        BGT     grpZeroSetLoop\name


        @// reset pSrc to pDst for the next stage
        SUB     pSrc,pDst,pointStep             @// pDst -= 2*grpSize
        mov     pDst, pPingPongBuf

        .endm


        M_START armSP_FFTFwd_CToC_FC32_Radix2_fs_OutOfPlace_unsafe_vfp,r4
        FFTSTAGE "FALSE","FALSE",FWD
        M_END

        M_START armSP_FFTInv_CToC_FC32_Radix2_fs_OutOfPlace_unsafe_vfp,r4
        FFTSTAGE "FALSE","TRUE",INV
        M_END


@/    ENDIF                                                           @//ARM1136JS


@// Guarding implementation by the processor name



    .end
