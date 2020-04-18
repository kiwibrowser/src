@//
@//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
@//
@//  Use of this source code is governed by a BSD-style license
@//  that can be found in the LICENSE file in the root of the source
@//  tree. An additional intellectual property rights grant can be found
@//  in the file PATENTS.  All contributing project authors may
@//  be found in the AUTHORS file in the root of the source tree.
@//
@//  This is a modification of 
@//  armSP_FFTInv_CCSToR_S32_preTwiddleRadix2_unsafe_s.S to support float
@//  instead of SC32.
@//

@//
@// Description:
@// Compute the "preTwiddleRadix2" stage prior to the call to the complexFFT
@// It does a Z(k) = Feven(k) + jW^(-k) FOdd(k); k=0,1,2,...N/2-1 computation
@// It implements the "scaled"(by 1/2) version of the above formula.
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

@/    IF  ARM1136JS

@//Input Registers

#define pSrc            r0
#define pDst            r1
#define pFFTSpec        r2


@// Output registers
#define result          r0

@//Local Scratch Registers


#define argTwiddle      r1
#define argDst          r2
#define argScale        r4
#define pTwiddle        r4
#define pOut            r5
#define subFFTSize      r7
#define subFFTNum       r6
#define N               r6
#define order           r14
#define diff            r9
#define count           r8
#define diffMinusOne    r2
#define round           r3

#define pOut1           r2
#define size            r7
#define step            r3
#define step1           r6
#define twStep          r12
#define pTwiddleTmp     r14
#define t0              r12

#define x0r     s0
#define x0i     s1
#define x1r     s2
#define x1i     s3
#define w0r     s4
#define w0i     s5
#define y0r     s6
#define y0i     s7
#define w1r     s6
#define w1i     s7
#define y1r     s6              /*@// w1r,w1i*/
#define y1i     s7
#define st0     s8
#define st1     s9
#define st2     s10
#define st3     s11
#define st4     s12
#define st5     s13
//@ half = 0.5
#define half    s15





        .macro FFTSTAGE scaled, inverse,name

        @// Initialize half now.
        movw    N, #0x0000
        movt    N, #0x3f00
        vmov.f32 half, N                @// half = 0.5

        @// Read the size from structure and take log
        LDR     N, [pFFTSpec, #ARMsFFTSpec_N]

        @// Read other structure parameters
        LDR     pTwiddle, [pFFTSpec, #ARMsFFTSpec_pTwiddle]
        LDR     pOut, [pFFTSpec, #ARMsFFTSpec_pBuf]


        MOV     size,N,ASR #1           @// preserve the contents of N

        MOV     step,size,LSL #3        @// step = N/2 * 8 bytes
        ADD     pTwiddleTmp,pTwiddle,#8 @// W^2

        ADD     pOut1,pOut,step         @// pOut1 = pOut+ N/2*8 bytes
        @// twStep = 3N/8 * 8 bytes pointing to W^1
        SUB     twStep,step,size,LSL #1
        MOV     step1,size,LSL #2       @// step1 = N/4 * 8 = N/2*4 bytes
        SUB     step1,step1,#8          @// (N/4-1)*8 bytes
        ADD     argTwiddle,pTwiddle,twStep      @// W^1

        @// Z(k) = 1/2 {[F(k) +  F'(N/2-k)] +j*W^(-k) [F(k) -  F'(N/2-k)]}
        @// Note: W^(k) is stored as negated value and also need to
        @// conjugate the values from the table

        @// Z(0) : no need of twiddle multiply
        @// Z(0) = 1/2 { [F(0) +  F'(N/2)] +j [F(0) -  F'(N/2)] }


        add      pSrc, step             @// step = N/2*8 bytes
        vldm.f32 pSrc, {x1r, x1i}       @// {x1r, x1i} = [pSrc, step]
        sub      pSrc, step
        vldm.f32 pSrc!, {x0r, x0i}

        SUBS    size,size,#2

        vadd.f32 st0, x0r, x1r          @// a+c
        vsub.f32 st1, x0r, x1r          @// a-c
        vmov.f32 x0r, st0
        vmov.f32 x1r, st1
        vsub.f32 st0, x0i, x1i          @// b-d
        vadd.f32 x1i, x0i, x1i          @// b+d
        vmov.f32 x0i, st0


        vsub.f32     x0r,x0r,x1i        @// Z(0).r
        vadd.f32     x0i,x0i,x1r        @// Z(0).i

        vmul.f32 x0r, half
        vmul.f32 x0i, half
        vstm.f32 pOut1!, {x0r, x0i}     @// pOut1 = pOut+ N/2*8 bytes

        BLT     end\name
        BEQ     lastElement\name

        ASR     size,size,#1
evenOddButterflyLoop\name:

        SUB     step,step,#16           @// (N/2-2)*8 bytes

        add      pSrc, step             @// (N/2-1)*8 bytes
        vldm.f32 pSrc, {x1r, x1i}       @// {x1r, x1i} = [pSrc, step]
        sub      pSrc, step
        vldm.f32 pSrc!, {x0r, x0i}
        add      argTwiddle, step1
        vldm.f32 argTwiddle, {w1r, w1i} @// {w1r, w1i} = [argTwiddle, step]
        sub      argTwiddle, step1
        vldm.f32 argTwiddle!, {w0r, w0i}

        SUB     step1,step1,#8
        SUBS    size,size,#1


        vsub.f32     st2,x0r,x1r        @// a-c
        vadd.f32     st3,x0i,x1i        @// b+d
        vadd.f32     st0,x0r,x1r        @// a+c
        vsub.f32     st1,x0i,x1i        @// b-d

        vmul.f32  x1r,w1r,st2
        vmul.f32  x1i,w1r,st3
        vmls.f32  x1r,w1i,st3
        vmla.f32  x1i,w1i,st2

        vadd.f32     y1r,st0,x1i        @// F(N/2 -1)
        vsub.f32     y1i,x1r,st1        @// y1r,y1i same as w1r, w1i


        vmul.f32  x0r,w0r,st2
        vmul.f32  x0i,w0r,st3
        vmla.f32  x0r,w0i,st3
        vmls.f32  x0i,w0i,st2


        vadd.f32     st4,st0,x0i        @// F(1)
        vsub.f32     st5,st1,x0r


        vmul.f32 y1r, half
        vmul.f32 y1i, half
        vmul.f32 st4, half
        vmul.f32 st5, half
        add      pOut1, step            @// (N/2-1)*8 bytes
        vstm.f32 pOut1, {y1r, y1i}      @// {y1r,y1i} = [pOut1, step]
        sub      pOut1, step
        vstm.f32 pOut1!, {st4, st5}

        MOV     t0,argTwiddle           @// swap ptr for even and odd twiddles
        MOV     argTwiddle,pTwiddleTmp
        MOV     pTwiddleTmp,t0

        BGT     evenOddButterflyLoop\name


        @// Last element can be expanded as follows
        @// 1/2[Z(k) + Z'(k)] - j w^-k [Z(k) - Z'(k)]
        @// (since W^k is stored as -ve)
        @// 1/2[(a+jb) + (a-jb)] - j w^-k [(a+jb) - (a-jb)]
        @// 1/2[2a+j0] + j (c-jd) [0+j2b]
        @// (a+bc, -bd)
        @// Since (c,d) = (0,1) for the last element, result is just (a,-b)

lastElement\name:
        vldm.f32 pSrc, {x0r, x0i}

        vneg.f32 x0i, x0i
        vstm.f32 pOut1, {x0r, x0i}
end\name:


        .endm


@ Structure offsets for FFTSpec
        .set    ARMsFFTSpec_N, 0
        .set    ARMsFFTSpec_pBitRev, 4
        .set    ARMsFFTSpec_pTwiddle, 8
        .set    ARMsFFTSpec_pBuf, 12


        M_START armSP_FFTInv_CCSToR_F32_preTwiddleRadix2_unsafe_vfp,r4
             FFTSTAGE "FALSE","TRUE",Inv
        M_END

@//    ENDIF                                           @//ARM1136JS


      @// Guarding implementation by the processor name



    .end
