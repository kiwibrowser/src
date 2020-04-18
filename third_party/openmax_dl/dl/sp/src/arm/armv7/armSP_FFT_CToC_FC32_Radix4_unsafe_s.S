@//
@//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
@//
@//  Use of this source code is governed by a BSD-style license
@//  that can be found in the LICENSE file in the root of the source
@//  tree. An additional intellectual property rights grant can be found
@//  in the file PATENTS.  All contributing project authors may
@//  be found in the AUTHORS file in the root of the source tree.
@//
@//  This is a modification of armSP_FFT_CToC_SC32_Radix4_unsafe_s.S
@//  to support float instead of SC32.
@//

@//
@// Description:
@// Compute a Radix 4 FFT stage for a N point complex signal
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



@//Output Registers


@//Local Scratch Registers

#define grpCount        r12
#define step            r12                  /*@// Reuse grpCount*/
#define outPointStep    r3
#define setCount        r8
#define diff            r9
#define pointStep       r14

#define t1              r3                 /*@// Reuse outPointStep*/

@// Real and Imaginary parts used in the inner grp loop
#define x0r s0
#define x0i s1
#define x1r s2
#define x1i s3
#define x2r s4
#define x2i s5
#define x3r s6
#define x3i s7

@// Temporary reg to hold the twiddle multiplies

#define t0r s8
#define t0i s9
#define t2r s10
#define t2i s11
#define sr  s12
#define si  s13




        .macro FFTSTAGE scaled, inverse , name

        @// Define stack arguments


        @// Update grpCount and grpSize rightaway inorder to reuse
        @// pGrpCount and pGrpSize regs

        LSL     grpCount,subFFTSize,#2
        lsr     subFFTNum, subFFTNum, #2
        mov     subFFTSize, grpCount


        @// pT0+1 increments pT0 by 8 bytes
        @// pT0+pointStep = increment of 8*pointStep bytes = 2*grpSize bytes
        mov     pointStep, subFFTNum, lsl #1


        @// pOut0+1 increments pOut0 by 8 bytes
        @// pOut0+outPointStep == increment of 8*outPointStep bytes = 2*size
        @// bytes

        @// Use setCount as dummy.  It's set correctly below.
        smull   outPointStep, setCount, grpCount, pointStep

        LSL     pointStep,pointStep,#2                      @// 2*grpSize


        MOV     setCount,pointStep,LSR #3

        @// Interchange grpLoop and setLoop

setLoop\name:

        MOV     step,#0
        @// Set pSrc and pDst for the grpLoop

        SUB      diff,outPointStep,pointStep

        @// Save setCount on stack to reuse the reg

        ADD      pSrc,pSrc,diff,LSL #2  @// pSrc += (grpCount-1)*grpStep
        ADD      pDst,pDst,diff         @// pDst += (grpCount-1)*setCount
        ADD      step,step,diff         @// step += (grpCount-1)*setCount



        @// Loop on the grps

grpLoop\name:



        @// butterfly loop
        add         pSrc, pointStep
        vldm.f32    pSrc, {x3r, x3i}                    @// data[1]
        add         pTwiddle, step
        vldm.f32    pTwiddle, {x1r, x1i}                @// coef[1]
        add         pTwiddle, step
        vldm.f32    pTwiddle, {x2r, x2i}                @// coef[2]
        add         pSrc, pointStep
        vldm.f32    pSrc, {x0r, x0i}                    @// data[2]

        @// do first complex multiply
        vmul.f32 t0r, x3r, x1r
        vmul.f32 t0i, x3i, x1r

        .ifeqs  "\inverse", "TRUE"
            vmla.f32 t0r, x3i, x1i
            vmls.f32 t0i, x3r, x1i
            vmov.f32 x1r, t0r
            vmov.f32 x1i, t0i
        .else
            vmls.f32 t0r, x3i, x1i
            vmla.f32 t0i, x3r, x1i
            vmov.f32 x1r, t0r
            vmov.f32 x1i, t0i
        .endif

        add     pTwiddle, pTwiddle, step
        vldm    pTwiddle, {x3r, x3i}                    @// coef[3]
        sub     pTwiddle, pTwiddle, step

        @// do second complex multiply
        vmul.f32 t0r, x0r, x2r
        vmul.f32 t0i, x0i, x2r

        .ifeqs  "\inverse", "TRUE"
            vmla.f32 t0r, x0i, x2i
            vmls.f32 t0i, x0r, x2i
            vmov.f32 x2r, t0r
            vmov.f32 x2i, t0i
        .else
            vmls.f32 t0r, x0i, x2i
            vmla.f32 t0i, x0r, x2i
            vmov.f32 x2r, t0r
            vmov.f32 x2i, t0i
        .endif

        add     pSrc, pointStep
        vldm    pSrc, {x0r, x0i}                @// data[3]
        sub     pSrc, pointStep

        SUB     pTwiddle,pTwiddle,step,LSL #1   @// reset pTwiddle
        SUBS    step,step,pointStep             @// decrement loop counter

        @// do third complex multiply
        SUB     pSrc,pSrc,pointStep,LSL #1      @// reset pSrc to data[0]
        vmul.f32 t0r, x0r, x3r
        vmul.f32 t0i, x0i, x3r

        .ifeqs  "\inverse", "TRUE"
            vmla.f32 t0r, x0i, x3i
            vmls.f32 t0i, x0r, x3i
            vmov.f32 x3r, t0r
            vmov.f32 x3i, t0i
        .else
            vmls.f32 t0r, x0i, x3i
            vmla.f32 t0i, x0r, x3i
            vmov.f32 x3r, t0r
            vmov.f32 x3i, t0i
        .endif

        vldm    pSrc, {x0r, x0i}                @// data[0]

        @// finish first stage of 4 point FFT
        vadd.f32     x0r,x0r,x2r                @// x0 = x0 + x2 (u0)
        vadd.f32     x0i,x0i,x2i

        vadd.f32     sr, x2r, x2r
        vadd.f32     si, x2i, x2i
        vsub.f32     x2r,x0r,sr                 @// x2 = x0 - x2 (u1)
        vsub.f32     x2i,x0i,si

        vadd.f32     x1r,x1r,x3r                @// x1 = x1/2 + x3/2 (u2/2)
        vadd.f32     x1i,x1i,x3i

        vadd.f32     sr, x3r, x3r
        vadd.f32     si, x3i, x3i
        vsub.f32     x3r,x1r,sr                 @// x3 = x1/2 - x3/2 (u3/2)
        vsub.f32     x3i,x1i,si


        @// finish second stage of 4 point FFT

        @// y0 = u1-u2 since twiddle's are stored as -ve values
        vsub.f32     x2r,x2r,x1r
        vsub.f32     x2i,x2i,x1i

        vadd.f32     sr, x1r, x1r
        vadd.f32     si, x1i, x1i
        vadd.f32     x1r,x2r,sr                 @// y2 = u1+u2
        vadd.f32     x1i,x2i,si
        vstm    pDst, {x2r, x2i}                @// store y0

        vsub.f32     x0r,x0r,x3i                @// y3 = u0+ju3
        vadd.f32     x0i,x0i,x3r

        vadd.f32     sr, x3r, x3r
        vadd.f32     si, x3i, x3i
        vadd.f32     t2r,x0r,si                 @// y1 = u0-ju3
        vsub.f32     t2i,x0i,sr                 @// t2 will be same as x2r reg

        .ifeqs  "\inverse", "TRUE"
            add     pDst, outPointStep
            vstm    pDst, {t2r, t2i}            @// store y1
            add     pDst, outPointStep
            vstm    pDst, {x1r, x1i}            @// store y2
            add     pDst, outPointStep
            vstm    pDst, {x0r, x0i}            @// store y3
            sub     pDst, outPointStep
        .else
            add     pDst, outPointStep
            vstm    pDst, {x0r, x0i}            @// store y1
            add     pDst, outPointStep
            vstm    pDst, {x1r, x1i}            @// store y2
            add     pDst, outPointStep
            vstm    pDst, {t2r, t2i}            @// store y3
            sub     pDst, outPointStep
        .endif

        SUB     pDst,pDst,outPointStep, LSL #1  @// reset pDst
        @// update the pDst for the next grp
        SUBGE   pDst,pDst,pointStep
        @// update the pSrc for the next grp
        SUBGE   pSrc,pSrc,pointStep,LSL #2


        BGE     grpLoop\name

        ADD     pSrc,pSrc,#8                    @// pSrc += 1; for the next set
        ADD     pDst,pDst,#8                    @// pDst += 1; for the next set

        SUBS    setCount,setCount,#1            @// decrement loop counter


        BGT     setLoop\name

        @// Reset and Swap pSrc and pDst for the next stage
        MOV     t1,pDst
        SUB     pDst,pSrc,subFFTNum,LSL #3
        SUB     pSrc,t1,subFFTNum,LSL #3

        .endm


        M_START armSP_FFTFwd_CToC_FC32_Radix4_OutOfPlace_unsafe_vfp,r4
        FFTSTAGE "FALSE","FALSE",FWD
        M_END

        M_START armSP_FFTInv_CToC_FC32_Radix4_OutOfPlace_unsafe_vfp,r4
        FFTSTAGE "FALSE","TRUE",INV
        M_END


@//    ENDIF                                                           @//ARM1136JS



@// Guarding implementation by the processor name

    .end
